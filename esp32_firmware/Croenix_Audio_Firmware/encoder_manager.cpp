#include "encoder_manager.h"
#include "device_state.h"
#include "audio_manager.h"
#include "fm_radio.h"
#include "sd_manager.h"
#include "websocket_server.h"

// -------------------------------------------------------------
// Production-Grade Quadrature State Machine for KY-040 Encoder
// -------------------------------------------------------------

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
static volatile bool     s_longPressFired = false;
static volatile uint32_t s_lastSwEventTime = 0;

// Volume HUD overlay tracking
static unsigned long s_volumeOverlayUntil = 0;
static uint8_t s_currentOverlayVol = 50;

// -------------------------------------------------------------
// Interrupt Service Routines
// -------------------------------------------------------------

void IRAM_ATTR encoderISR() {
    uint8_t a = digitalRead(ENCODER_CLK);
    uint8_t b = digitalRead(ENCODER_DT);

    s_encState = ((s_encState << 2) | (a << 1) | b) & 0x0F;
    int8_t step = Q_TABLE[s_encState];

    if (step != 0) {
        s_detentAcc += step;

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
        if (!s_swIsDown) {
            s_swIsDown = true;
            s_swPressStartTime = now;
            s_longPressFired = false;
            s_lastSwEventTime = now;
        }
    } else {
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
    DEBUG_PRINTLN("[ENCODER] Initializing interrupt state machine...");

    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);

    uint8_t a = digitalRead(ENCODER_CLK);
    uint8_t b = digitalRead(ENCODER_DT);
    s_encState = ((a << 1) | b) & 0x03;
    s_detentAcc = 0;
    s_encDelta = 0;

    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT), encoderISR, CHANGE);
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
            DEBUG_PRINTLN("[ENCODER] Entered HARDWARE STATUS Screen");
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

    // 1. Long-Press Detection (>750ms returns to Home Screen; >8000ms triggers Diagnostics)
    if (s_swIsDown && !s_longPressFired) {
        uint32_t holdTime = now - s_swPressStartTime;
        if (holdTime >= 8000) {
            s_longPressFired = true;
            toggleHardwareDiagnostics();
        } else if (holdTime >= 750 && g_deviceState.activeScreen != UI_SCREEN_HOME) {
            s_longPressFired = true;
            g_deviceState.activeScreen = UI_SCREEN_HOME;
            g_deviceState.needDisplayRedraw = true;
            DEBUG_PRINTLN("[ENCODER] Long-press: Returned to HOME screen");
            broadcastDeviceState();
        }
    }

    // 2. Button Release (Single Click Actions)
    if (s_swReleasedEvent) {
        s_swReleasedEvent = false;

        if (!s_longPressFired) {
            uint32_t duration = s_swReleaseTime - s_swPressStartTime;
            if (duration >= ENCODER_SHORT_PRESS_MIN && duration <= ENCODER_SHORT_PRESS_MAX) {
                switch (g_deviceState.activeScreen) {
                    case UI_SCREEN_HOME:
                        // Enter the highlighted mode
                        switch (g_deviceState.homeGridCursor) {
                            case 0: // SD Card
                                switchAudioSource(SOURCE_SD_CARD);
                                g_deviceState.activeScreen = UI_SCREEN_NOW_PLAYING;
                                break;
                            case 1: // USB
                                g_deviceState.activeScreen = UI_SCREEN_USB;
                                break;
                            case 2: // Bluetooth
                                switchAudioSource(SOURCE_BLUETOOTH);
                                g_deviceState.activeScreen = UI_SCREEN_BLUETOOTH;
                                break;
                            case 3: // FM Radio
                                switchAudioSource(SOURCE_FM_RADIO);
                                g_deviceState.activeScreen = UI_SCREEN_FM_RADIO;
                                break;
                            case 4: // Web FM
                                switchAudioSource(SOURCE_WEB_STREAM);
                                g_deviceState.activeScreen = UI_SCREEN_WEB_FM;
                                break;
                            case 5: // SoundCloud
                                switchAudioSource(SOURCE_WEB_STREAM);
                                g_deviceState.activeScreen = UI_SCREEN_SOUNDCLOUD;
                                break;
                            case 6: // AUX
                                g_deviceState.activeScreen = UI_SCREEN_AUX;
                                break;
                            case 7: // More -> Equalizer
                                g_deviceState.activeScreen = UI_SCREEN_EQUALIZER;
                                break;
                        }
                        g_deviceState.needDisplayRedraw = true;
                        break;

                    case UI_SCREEN_NOW_PLAYING:
                    case UI_SCREEN_BLUETOOTH:
                        if (isAudioPlaying()) {
                            pauseAudio();
                        } else {
                            playAudio();
                        }
                        break;

                    case UI_SCREEN_FM_RADIO: {
                        // Cycle FM Presets: P1(91.1) -> P2(98.3) -> P3(100.8) -> P4(104.8)
                        g_deviceState.selectedPresetFM = (g_deviceState.selectedPresetFM + 1) % 4;
                        const float fms[4] = {91.10f, 98.30f, 100.80f, 104.80f};
                        setFMFrequency(fms[g_deviceState.selectedPresetFM]);
                        g_deviceState.needDisplayRedraw = true;
                        break;
                    }

                    case UI_SCREEN_EQUALIZER: {
                        // Cycle EQ Presets: Flat -> Rock -> Pop -> Jazz
                        g_deviceState.selectedEQPreset = (g_deviceState.selectedEQPreset + 1) % 4;
                        setEQPreset(g_deviceState.selectedEQPreset);
                        g_deviceState.needDisplayRedraw = true;
                        break;
                    }

                    case UI_SCREEN_HARDWARE_DIAGNOSTICS:
                        if (g_deviceState.selectedDiagModule == 0) {
                            playBootChime();
                        } else {
                            toggleHardwareDiagnostics();
                        }
                        break;

                    default:
                        // Return to Home
                        g_deviceState.activeScreen = UI_SCREEN_HOME;
                        g_deviceState.needDisplayRedraw = true;
                        break;
                }
                broadcastDeviceState();
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

    for (int i = 0; i < abs(delta); i++) {
        int dir = (delta > 0) ? 1 : -1;

        switch (g_deviceState.activeScreen) {
            case UI_SCREEN_HOME: {
                // Navigate 8-Card Grid (0 to 7)
                int cur = (int)g_deviceState.homeGridCursor + dir;
                if (cur < 0) cur = 7;
                if (cur > 7) cur = 0;
                g_deviceState.homeGridCursor = (uint8_t)cur;
                g_deviceState.needDisplayRedraw = true;
                break;
            }

            case UI_SCREEN_NOW_PLAYING: {
                // Next / Previous Track
                if (dir > 0) nextTrack();
                else previousTrack();
                break;
            }

            case UI_SCREEN_FM_RADIO: {
                // Precision 0.1 MHz Frequency Tuning
                float curF = g_deviceState.fmFrequency;
                float newF = curF + (dir * FM_STEP_FREQUENCY);
                if (newF < FM_MIN_FREQUENCY) newF = FM_MAX_FREQUENCY;
                if (newF > FM_MAX_FREQUENCY) newF = FM_MIN_FREQUENCY;
                setFMFrequency(newF);
                break;
            }

            case UI_SCREEN_EQUALIZER: {
                // Cycle EQ Presets
                int p = (int)g_deviceState.selectedEQPreset + dir;
                if (p < 0) p = 3;
                if (p > 3) p = 0;
                g_deviceState.selectedEQPreset = (uint8_t)p;
                setEQPreset(g_deviceState.selectedEQPreset);
                g_deviceState.needDisplayRedraw = true;
                break;
            }

            case UI_SCREEN_HARDWARE_DIAGNOSTICS:
                if (dir > 0) nextDiagModule();
                else prevDiagModule();
                break;

            default: {
                // Standard Volume Adjustment
                int currentVol = (int)getMasterVolume();
                currentVol = constrain(currentVol + (dir * 3), 0, 100);
                setMasterVolume((uint8_t)currentVol);

                s_currentOverlayVol = (uint8_t)currentVol;
                s_volumeOverlayUntil = now + 1200;
                break;
            }
        }
    }
    broadcastDeviceState();
}

bool isVolumeOverlayActive() {
    return millis() < s_volumeOverlayUntil;
}

uint8_t getVolumeOverlayValue() {
    return s_currentOverlayVol;
}
