#include "encoder_manager.h"
#include "device_state.h"
#include "audio_manager.h"
#include "fm_radio.h"
#include "sd_manager.h"
#include "websocket_server.h"

// -------------------------------------------------------------
// Production-Grade Quadrature State Machine for KY-040 Encoder
// Eliminates contact bounce, removes latency, and guarantees
// exactly 1:1 mechanical detent-to-state transitions.
// -------------------------------------------------------------

// Gray-code transition lookup table
// State index = (prev_CLK << 3) | (prev_DT << 2) | (curr_CLK << 1) | curr_DT
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
// Interrupt Service Routines (IRAM_ATTR for zero-latency execution)
// -------------------------------------------------------------

void IRAM_ATTR encoderISR() {
    uint8_t a = digitalRead(ENCODER_CLK);
    uint8_t b = digitalRead(ENCODER_DT);

    s_encState = ((s_encState << 2) | (a << 1) | b) & 0x0F;
    int8_t step = Q_TABLE[s_encState];

    if (step != 0) {
        s_detentAcc += step;

        // Mechanical detent resting position is both pins pulled HIGH (CLK=1, DT=1)
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
        // Switch pressed down
        if (!s_swIsDown) {
            s_swIsDown = true;
            s_swPressStartTime = now;
            s_longPress10sFired = false;
            s_lastSwEventTime = now;
        }
    } else {
        // Switch released
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
    DEBUG_PRINTLN("[ENCODER] Initializing lean interrupt state machine...");

    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);

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

    DEBUG_PRINTF("[ENCODER] Interrupts configured on CLK=%d, DT=%d, SW=%d\n",
                 ENCODER_CLK, ENCODER_DT, ENCODER_SW);
}

void toggleHardwareDiagnostics() {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (g_deviceState.activeScreen == UI_SCREEN_HARDWARE_DIAGNOSTICS) {
            g_deviceState.activeScreen = UI_SCREEN_HOME;
            DEBUG_PRINTLN("[ENCODER] Diagnostics Exited -> Home Screen");
        } else {
            g_deviceState.activeScreen = UI_SCREEN_HARDWARE_DIAGNOSTICS;
            g_deviceState.selectedDiagModule = 0;
            DEBUG_PRINTLN("[ENCODER] 10s Long-Press: Entered HARDWARE STATUS Screen");
        }
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
    broadcastDeviceState();
}

// -------------------------------------------------------------
// Task Processing Loop (Runs on Core 0)
// -------------------------------------------------------------

void updateEncoderManager() {
    uint32_t now = millis();

    // Check expiration of the 5-second FM Tune window
    checkFMTuneExpiration();

    // 1. Detect 10-Second Long-Press from ANY screen while button is held
    if (s_swIsDown && !s_longPress10sFired) {
        if (now - s_swPressStartTime >= ENCODER_LONG_PRESS_MS) {
            s_longPress10sFired = true;
            toggleHardwareDiagnostics();
        }
    }

    // 2. Handle Button Release (Single Press Actions)
    if (s_swReleasedEvent) {
        s_swReleasedEvent = false;

        if (!s_longPress10sFired) {
            uint32_t duration = s_swReleaseTime - s_swPressStartTime;
            if (duration >= ENCODER_SHORT_PRESS_MIN && duration <= ENCODER_SHORT_PRESS_MAX) {
                // If in Hardware Diagnostics screen: Click exits to Home Screen
                if (g_deviceState.activeScreen == UI_SCREEN_HARDWARE_DIAGNOSTICS) {
                    toggleHardwareDiagnostics();
                } else {
                    // Home Screen Behavior:
                    // If in FM mode and Tune window is NOT active -> Activate 5-second Tune window!
                    if (g_deviceState.currentSource == SOURCE_FM_RADIO && !isFMTuneWindowActive()) {
                        triggerFMTuneWindow();
                    } else {
                        // Single press cycles input modes: Bluetooth -> SD Card -> FM -> Web FM
                        DEBUG_PRINTLN("[ENCODER] Single press: Cycling input mode");
                        cycleAudioSource();
                    }
                    broadcastDeviceState();
                }
            }
        }
    }

    // 3. Process Rotary Encoder Detent Steps
    int16_t delta = 0;
    if (s_encDelta != 0) {
        noInterrupts();
        delta = s_encDelta;
        s_encDelta = 0;
        interrupts();
    }

    if (delta == 0) return;

    // Process each mechanical detent step
    for (int i = 0; i < abs(delta); i++) {
        int dir = (delta > 0) ? 1 : -1;

        if (g_deviceState.activeScreen == UI_SCREEN_HARDWARE_DIAGNOSTICS) {
            // Scroll through connected modules in Hardware Status
            if (dir > 0) nextDiagModule();
            else prevDiagModule();
            DEBUG_PRINTF("[ENCODER] Diag Scroll: Module %d\n", g_deviceState.selectedDiagModule);
        } else {
            // Home Screen Contextual Controls
            if (g_deviceState.currentSource == SOURCE_FM_RADIO && isFMTuneWindowActive()) {
                // FM Tuning Override: Adjust frequency during 5-second Tune window
                float curFreq = g_deviceState.fmFrequency;
                float newFreq = curFreq + (dir * FM_STEP_FREQUENCY);
                if (newFreq < FM_MIN_FREQUENCY) newFreq = FM_MAX_FREQUENCY;
                if (newFreq > FM_MAX_FREQUENCY) newFreq = FM_MIN_FREQUENCY;
                setFMFrequency(newFreq);
                // Extend the tune window upon each turn
                triggerFMTuneWindow();
                DEBUG_PRINTF("[ENCODER] FM Tuned: %.1f MHz\n", newFreq);
            } else if (g_deviceState.currentSource == SOURCE_SD_CARD) {
                // SD Card Mode: Next/Previous track browsing
                if (dir > 0) nextTrack();
                else previousTrack();
            } else {
                // Standard Volume Adjustment (Default & Reverted after FM 5s window)
                int currentVol = (int)getMasterVolume();
                currentVol = constrain(currentVol + (dir * 3), 0, 100);
                setMasterVolume((uint8_t)currentVol);

                s_currentOverlayVol = (uint8_t)currentVol;
                s_volumeOverlayUntil = now + 1200;
                DEBUG_PRINTF("[ENCODER] Volume: %d%%\n", currentVol);
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
