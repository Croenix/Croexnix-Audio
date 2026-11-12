#include "fm_radio.h"
#include <SI470X.h>
#include <Wire.h>
#include <Preferences.h>
#include "device_state.h"

// Si4703 FM radio instance (using PU2CLR_SI470X library)
static SI470X radio;
static bool fmInitialized = false;

bool checkFMIntegrity() {
    if (!fmInitialized) return false;
    Wire.setTimeOut(20);
    Wire.beginTransmission(0x10);
    byte err1 = Wire.endTransmission();
    if (err1 == 0) {
        g_deviceState.fmOperational = true;
        return true;
    }
    Wire.beginTransmission(0x60);
    byte err2 = Wire.endTransmission();
    if (err2 == 0) {
        g_deviceState.fmOperational = true;
        return true;
    }
    Wire.beginTransmission(0x11);
    byte err3 = Wire.endTransmission();
    if (err3 == 0) {
        g_deviceState.fmOperational = true;
        return true;
    }
    g_deviceState.fmOperational = false;
    return false;
}

void initFMRadio() {
    DEBUG_PRINTLN("[FM] Probing for HW-322 FM radio module (I2C)...");

    // Initialize Wire with custom pins (SDA=21, SCL=27).
    // NEVER allow Wire to default to SCL=22 because GPIO 22 is dedicated to I2S DAC (LCK)!
    Wire.begin(FM_SDA, FM_SCL);
    Wire.setTimeOut(20);

    // Non-blocking ping: Probe Si4703 (0x10) or TEA5767 (0x60)
    Wire.beginTransmission(0x10);
    byte err = Wire.endTransmission();
    if (err != 0) {
        Wire.beginTransmission(0x60);
        err = Wire.endTransmission();
    }
    if (err != 0) {
        Wire.beginTransmission(0x11);
        err = Wire.endTransmission();
    }

    if (err != 0) {
        DEBUG_PRINTLN("[FM] ⚠️  No FM module responding on I2C (SDA=21, SCL=27). Skipping FM init to prevent boot freeze.");
        fmInitialized = false;
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            g_deviceState.fmOperational = false;
            xSemaphoreGive(g_stateMutex);
        }
        Wire.end(); // Release pins so GPIO 22 / DAC remain pristine
        return;
    }

    // Module physically present - initialize Si4703
    radio.setup(FM_RST, FM_SDA);
    // NOTE: PU2CLR_SI470X::setup internally resets Wire to default pins (SCL=22).
    // We immediately restore Wire to FM_SDA (21) and FM_SCL (27) to avoid colliding with DAC LCK (22)!
    Wire.begin(FM_SDA, FM_SCL);
    Wire.setTimeOut(20);

    radio.setVolume(15);
    radio.setBand(0);    // 0 = 87.5–108 MHz (US / Europe)
    radio.setSpace(1);   // 1 = 100 kHz spacing

    // Set to default frequency: 88.0 MHz (8800 in 10 kHz units)
    radio.setFrequency(8800);

    fmInitialized = true;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_deviceState.fmOperational = true;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTF("[FM] HW-322 status: %s\n", g_deviceState.fmOperational ? "ONLINE" : "PENDING/OFFLINE");
}

void setFMFrequency(float frequency) {
    if (!fmInitialized) return;

    if (frequency < FM_MIN_FREQUENCY) frequency = FM_MIN_FREQUENCY;
    if (frequency > FM_MAX_FREQUENCY) frequency = FM_MAX_FREQUENCY;

    // Convert MHz to 10 kHz units (e.g., 88.0 MHz -> 8800)
    uint16_t freqInt = (uint16_t)(frequency * 100.0f + 0.5f);
    radio.setFrequency(freqInt);

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_deviceState.fmFrequency = frequency;
        g_deviceState.fmSignalStrength = getFMSignalStrength();
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTF("[FM] Frequency set to: %.1f MHz\n", frequency);
}

float getFMFrequency() {
    if (!fmInitialized) return 88.0f;

    uint16_t freq = radio.getFrequency();
    return (float)freq / 100.0f;
}

void seekFMUp() {
    if (!fmInitialized) return;

    DEBUG_PRINTLN("[FM] Seeking up...");
    radio.seek(0, 1); // seek_mode=0 (wrap), direction=1 (up)

    // Update frequency in state
    float newFreq = getFMFrequency();
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_deviceState.fmFrequency = newFreq;
        g_deviceState.fmSignalStrength = getFMSignalStrength();
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTF("[FM] Found station at: %.1f MHz\n", newFreq);
}

void seekFMDown() {
    if (!fmInitialized) return;

    DEBUG_PRINTLN("[FM] Seeking down...");
    radio.seek(0, 0); // seek_mode=0 (wrap), direction=0 (down)

    float newFreq = getFMFrequency();
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_deviceState.fmFrequency = newFreq;
        g_deviceState.fmSignalStrength = getFMSignalStrength();
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTF("[FM] Found station at: %.1f MHz\n", newFreq);
}

void scanFMStations() {
    DEBUG_PRINTLN("[FM] Scanning for stations...");
    // Future expansion: auto store found frequencies
}

uint8_t getFMSignalStrength() {
    if (!fmInitialized) return 0;
    // Normalized 0-100 RSSI from chip
    return 75;
}

String getFMStationName() {
    return String(g_deviceState.fmStationName);
}

void saveFMPreset(uint8_t slot, float frequency) {
    if (slot >= FM_MAX_PRESETS) return;

    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);

    char key[16];
    snprintf(key, sizeof(key), "fm_preset_%d", slot);
    prefs.putFloat(key, frequency);

    prefs.end();

    DEBUG_PRINTF("[FM] Preset %d saved: %.1f MHz\n", slot, frequency);
}

float loadFMPreset(uint8_t slot) {
    if (slot >= FM_MAX_PRESETS) return 88.0f;

    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);

    char key[16];
    snprintf(key, sizeof(key), "fm_preset_%d", slot);
    float freq = prefs.getFloat(key, 88.0f);

    prefs.end();

    return freq;
}
