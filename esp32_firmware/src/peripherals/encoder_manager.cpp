#include "encoder_manager.h"
#include "device_state.h"
#include "audio_manager.h"
#include "fm_radio.h"
#include "sd_manager.h"
#include "websocket_server.h"

// -------------------------------------------------------------
// Interrupt-driven Quadrature State Machine for KY-040 Encoder
// -------------------------------------------------------------

// Quadrature Gray-code transition lookup table
// Maps 4-bit state (prev_A, prev_B, curr_A, curr_B) to rotation direction:
//  0: Invalid / No step
// +1: Clockwise step
// -1: Counter-clockwise step
static const int8_t Q_TABLE[16] = {
    0, -1,  1,  0,
    1,  0,  0, -1,
   -1,  0,  0,  1,
    0,  1, -1,  0
};

static volatile uint8_t s_encState = 0;
static volatile int8_t  s_detentAcc = 0;
static volatile int16_t s_encDelta = 0;

// Button state tracking (SW pin)
static volatile bool     s_swIsDown = false;
static volatile uint32_t s_swPressStartTime = 0;
static volatile uint32_t s_swReleaseTime = 0;
static volatile bool     s_swReleasedEvent = false;
static volatile bool     s_longPress10sFired = false;
static volatile uint32_t s_lastSwEventTime = 0;

// Volume HUD overlay tracking
static unsigned long s_volumeOverlayUntil = 0;
static uint8_t s_currentOverlayVol = 50;

// -------------------------------------------------------------
// Interrupt Service Routines (IRAM for zero latency)
// -------------------------------------------------------------

void IRAM_ATTR encoderISR() {
    uint8_t a = digitalRead(ENCODER_CLK);
    uint8_t b = digitalRead(ENCODER_DT);

    s_encState = ((s_encState << 2) | (a << 1) | b) & 0x0F;
    int8_t step = Q_TABLE[s_encState];

    if (step != 0) {
        s_detentAcc += step;

        // Mechanical detent rest state is usually CLK=1 and DT=1 (pulled high)
        if (a == 1 && b == 1) {
            if (s_detentAcc >= 2) {
                s_encDelta++;
                s_detentAcc = 0;
            } else if (s_detentAcc <= -2) {
                s_encDelta--;
                s_detentAcc = 0;
            } else {
                s_detentAcc = 0;
            }
        }
    }
}

void IRAM_ATTR buttonISR() {
    uint32_t now = millis();
    if (now - s_lastSwEventTime < 25) return; // Debounce guard

    int pinVal = digitalRead(ENCODER_SW);
    if (pinVal == LOW) {
        // Button pressed down
        if (!s_swIsDown) {
            s_swIsDown = true;
            s_swPressStartTime = now;
            s_longPress10sFired = false;
            s_lastSwEventTime = now;
        }
    } else {
        // Button released
        if (s_swIsDown) {
            s_swIsDown = false;
            s_swReleaseTime = now;
            s_swReleasedEvent = true;
            s_lastSwEventTime = now;
        }
    }
}

// -------------------------------------------------------------
// Initialization
// -------------------------------------------------------------

void initEncoderManager() {
    DEBUG_PRINTLN("[ENCODER] Initializing robust interrupt state machine...");

    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);

    // Seed state with current pin readings
    uint8_t a = digitalRead(ENCODER_CLK);
    uint8_t b = digitalRead(ENCODER_DT);
    s_encState = ((a << 1) | b) & 0x03;
    s_detentAcc = 0;
    s_encDelta = 0;

    // Attach hardware interrupts on CHANGE for quadrature decoding
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT), encoderISR, CHANGE);

    // Attach hardware interrupt for push button
    attachInterrupt(digitalPinToInterrupt(ENCODER_SW), buttonISR, CHANGE);

    DEBUG_PRINTF("[ENCODER] Pins attached: CLK=%d, DT=%d, SW=%d (10s Long-Press Diagnostics Enabled)\n",
                 ENCODER_CLK, ENCODER_DT, ENCODER_SW);
}

// -------------------------------------------------------------
// Diagnostics Toggle Helper
// -------------------------------------------------------------

void toggleHardwareDiagnostics() {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (g_deviceState.activeScreen == UI_SCREEN_HARDWARE_DIAGNOSTICS) {
            g_deviceState.activeScreen = UI_SCREEN_HOME;
            DEBUG_PRINTLN("[ENCODER] Exiting Hardware Diagnostics -> Returning to Home Screen");
        } else {
            g_deviceState.activeScreen = UI_SCREEN_HARDWARE_DIAGNOSTICS;
            g_deviceState.selectedDiagModule = 0;
            DEBUG_PRINTLN("[ENCODER] 10-Second Long-Press Triggered: Entered HARDWARE DIAGNOSTICS Mode");
        }
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
    broadcastDeviceState();
}

// -------------------------------------------------------------
// Update Loop (Task Processing)
// -------------------------------------------------------------

void updateEncoderManager() {
    uint32_t now = millis();

    // 1. Check for 10-second long press interrupt while held down
    if (s_swIsDown && !s_longPress10sFired) {
        if (now - s_swPressStartTime >= ENCODER_LONG_PRESS_MS) {
            s_longPress10sFired = true;
            toggleHardwareDiagnostics();
        }
    }

    // 2. Handle button release (Single Press)
    if (s_swReleasedEvent) {
        s_swReleasedEvent = false;

        if (!s_longPress10sFired) {
            uint32_t duration = s_swReleaseTime - s_swPressStartTime;
            if (duration >= ENCODER_SHORT_PRESS_MIN && duration <= ENCODER_SHORT_PRESS_MAX) {
                // Check active screen
                if (g_deviceState.activeScreen == UI_SCREEN_HARDWARE_DIAGNOSTICS) {
                    // Click in Diagnostics exits back to Home screen
                    toggleHardwareDiagnostics();
                } else {
                    // Single press cycles input modes: FM -> Music Player -> Bluetooth -> SD card -> FM
                    DEBUG_PRINTLN("[ENCODER] Single press detected: Cycling input mode");
                    cycleAudioSource();
                    broadcastDeviceState();
                }
            }
        }
    }

    // 3. Process Rotary Turns (Detent State Machine)
    int16_t delta = 0;
    if (s_encDelta != 0) {
        noInterrupts();
        delta = s_encDelta;
        s_encDelta = 0;
        interrupts();
    }

    if (delta == 0) return;

    // Process each detent step
    for (int i = 0; i < abs(delta); i++) {
        int dir = (delta > 0) ? 1 : -1;

        if (g_deviceState.activeScreen == UI_SCREEN_HARDWARE_DIAGNOSTICS) {
            // Scroll through connected modules in Hardware Status
            if (dir > 0) {
                nextDiagModule();
            } else {
                prevDiagModule();
            }
            DEBUG_PRINTF("[ENCODER] Hardware Status Scroll: Module %d\n", g_deviceState.selectedDiagModule);
        } else {
            // Screen is Home: Mode-specific Controls
            switch (g_deviceState.currentSource) {
                case SOURCE_FM_RADIO: {
                    // FM Channel Tuning: 0.1 MHz per detent step
                    float currentFreq = g_deviceState.fmFrequency;
                    float newFreq = currentFreq + (dir * 0.1f);
                    if (newFreq < FM_MIN_FREQUENCY) newFreq = FM_MAX_FREQUENCY;
                    if (newFreq > FM_MAX_FREQUENCY) newFreq = FM_MIN_FREQUENCY;
                    setFMFrequency(newFreq);
                    DEBUG_PRINTF("[ENCODER] FM Tuned to: %.1f MHz\n", newFreq);
                    break;
                }

                case SOURCE_SD_CARD: {
                    // SD Folder / Track browsing
                    if (dir > 0) {
                        nextTrack();
                        DEBUG_PRINTLN("[ENCODER] SD Browse: Next Track");
                    } else {
                        previousTrack();
                        DEBUG_PRINTLN("[ENCODER] SD Browse: Previous Track");
                    }
                    break;
                }

                case SOURCE_WEB_STREAM: // Music Player
                case SOURCE_BLUETOOTH:
                default: {
                    // Volume adjustment
                    int currentVol = (int)getMasterVolume();
                    currentVol = constrain(currentVol + (dir * 3), 0, 100);
                    setMasterVolume((uint8_t)currentVol);

                    // Show temporary Volume HUD overlay
                    s_currentOverlayVol = (uint8_t)currentVol;
                    s_volumeOverlayUntil = now + 1200;
                    DEBUG_PRINTF("[ENCODER] Volume Detent: %d%%\n", currentVol);
                    break;
                }
            }

            broadcastDeviceState();
        }
    }
}

bool isVolumeOverlayActive() {
    return millis() < s_volumeOverlayUntil;
}

uint8_t getVolumeOverlayValue() {
    return s_currentOverlayVol;
}
