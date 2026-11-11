#include "encoder_manager.h"
#include "device_state.h"
#include "audio_manager.h"
#include "websocket_server.h"

// Encoder pin state tracking
static volatile int lastCLK = HIGH;
static unsigned long lastTurnTime = 0;

// Button state tracking
static unsigned long buttonPressTime = 0;
static bool buttonPressed = false;
static const unsigned long LONG_PRESS_MS = 750;

// Volume HUD overlay timer
static unsigned long volumeOverlayUntil = 0;
static uint8_t currentOverlayVol = 50;

void initEncoderManager() {
    DEBUG_PRINTLN("[ENCODER] Initializing rotary encoder (KY-040)...");

    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);

    lastCLK = digitalRead(ENCODER_CLK);

    DEBUG_PRINTF("[ENCODER] Pins configured: CLK=%d, DT=%d, SW=%d\n",
                 ENCODER_CLK, ENCODER_DT, ENCODER_SW);
}

void updateEncoderManager() {
    unsigned long now = millis();

    // 1. Rotary Turn Detection
    int currentCLK = digitalRead(ENCODER_CLK);
    if (currentCLK != lastCLK && currentCLK == LOW) {
        // Debounce minimum 25ms between steps
        if (now - lastTurnTime > 25) {
            int currentDT = digitalRead(ENCODER_DT);
            int currentVol = (int)getMasterVolume();

            if (currentDT != currentCLK) {
                // Clockwise -> Volume UP
                currentVol = min(100, currentVol + 3);
            } else {
                // Counter-clockwise -> Volume DOWN
                currentVol = max(0, currentVol - 3);
            }

            setMasterVolume((uint8_t)currentVol);

            // Activate Volume HUD overlay on TFT for 1.5 seconds
            currentOverlayVol = (uint8_t)currentVol;
            volumeOverlayUntil = now + 1500;

            // Immediately broadcast state to sync with mobile app
            broadcastDeviceState();

            lastTurnTime = now;
            DEBUG_PRINTF("[ENCODER] Rotated. New Volume: %d%%\n", currentVol);
        }
    }
    lastCLK = currentCLK;

    // 2. Push Button Detection (Active LOW)
    int btnState = digitalRead(ENCODER_SW);

    if (btnState == LOW) {
        if (!buttonPressed) {
            buttonPressed = true;
            buttonPressTime = now;
        }
    } else {
        if (buttonPressed) {
            unsigned long pressDuration = now - buttonPressTime;
            buttonPressed = false;

            if (pressDuration >= LONG_PRESS_MS) {
                // Long press: Cycle Audio Source
                AudioSource nextSrc = (AudioSource)((g_deviceState.currentSource + 1) % 5);
                if (nextSrc == SOURCE_NONE) nextSrc = SOURCE_BLUETOOTH;
                switchAudioSource(nextSrc);
                DEBUG_PRINTF("[ENCODER] Long press: Switched source to %d\n", nextSrc);
            } else if (pressDuration > 40) {
                // Short click: Toggle Play / Pause
                if (g_deviceState.isPlaying) {
                    pauseAudio();
                    DEBUG_PRINTLN("[ENCODER] Short click: Paused");
                } else {
                    playAudio();
                    DEBUG_PRINTLN("[ENCODER] Short click: Play");
                }
            }

            // Immediately broadcast state to sync with mobile app
            broadcastDeviceState();
        }
    }
}

bool isVolumeOverlayActive() {
    return millis() < volumeOverlayUntil;
}

uint8_t getVolumeOverlayValue() {
    return currentOverlayVol;
}

