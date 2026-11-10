#include "fm_radio.h"
#include <Si4703_Breakout.h>
#include <Wire.h>
#include "../core/device_state.h"

// Si4703 FM radio instance
Si4703_Breakout radio(FM_RST, FM_SDA);

static bool fmInitialized = false;

void initFMRadio() {
    DEBUG_PRINTLN("[FM] Initializing Si4703 FM radio...");

    Wire.begin(FM_SDA, FM_SCL);

    if (radio.begin()) {
        radio.setVolume(15);  // Max volume (handled by main volume control)
        radio.powerOn();

        // Set to default frequency
        radio.setChannel(8800); // 88.0 MHz

        fmInitialized = true;

        DEBUG_PRINTLN("[FM] Si4703 initialized successfully");
    } else {
        DEBUG_PRINTLN("[FM] Failed to initialize Si4703!");
    }
}

void setFMFrequency(float frequency) {
    if (!fmInitialized) return;

    if (frequency < FM_MIN_FREQUENCY) frequency = FM_MIN_FREQUENCY;
    if (frequency > FM_MAX_FREQUENCY) frequency = FM_MAX_FREQUENCY;

    int channel = (int)(frequency * 10);
    radio.setChannel(channel);

    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.fmFrequency = frequency;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTF("[FM] Frequency set to: %.1f MHz\n", frequency);
}

float getFMFrequency() {
    if (!fmInitialized) return 0.0;

    int channel = radio.getChannel();
    return (float)channel / 10.0;
}

void seekFMUp() {
    if (!fmInitialized) return;

    DEBUG_PRINTLN("[FM] Seeking up...");
    radio.seekUp();

    // Update frequency in state
    float newFreq = getFMFrequency();
    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.fmFrequency = newFreq;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTF("[FM] Found station at: %.1f MHz\n", newFreq);
}

void seekFMDown() {
    if (!fmInitialized) return;

    DEBUG_PRINTLN("[FM] Seeking down...");
    radio.seekDown();

    // Update frequency in state
    float newFreq = getFMFrequency();
    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.fmFrequency = newFreq;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTF("[FM] Found station at: %.1f MHz\n", newFreq);
}

void scanFMStations() {
    if (!fmInitialized) return;

    DEBUG_PRINTLN("[FM] Scanning all stations...");

    for (float freq = FM_MIN_FREQUENCY; freq <= FM_MAX_FREQUENCY; freq += FM_SPACING) {
        setFMFrequency(freq);
        delay(30);

        uint8_t rssi = getFMSignalStrength();
        if (rssi > 20) {
            DEBUG_PRINTF("[FM] Station found: %.1f MHz (RSSI: %d)\n", freq, rssi);
        }
    }

    DEBUG_PRINTLN("[FM] Scan complete");
}

uint8_t getFMSignalStrength() {
    if (!fmInitialized) return 0;

    // Get RSSI from Si4703 (0-75)
    uint8_t rssi = radio.getRSSI();

    // Convert to 0-100 scale
    uint8_t strength = map(rssi, 0, 75, 0, 100);

    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.fmSignalStrength = strength;
        xSemaphoreGive(g_stateMutex);
    }

    return strength;
}

String getFMStationName() {
    if (!fmInitialized) return "";

    // RDS support (if available)
    // This is a placeholder - full RDS implementation needed
    return "FM Radio";
}

void saveFMPreset(uint8_t slot, float frequency) {
    if (slot >= FM_MAX_PRESETS) return;

    // Save to NVS
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);

    char key[16];
    snprintf(key, sizeof(key), "fm_preset_%d", slot);
    prefs.putFloat(key, frequency);

    prefs.end();

    DEBUG_PRINTF("[FM] Preset %d saved: %.1f MHz\n", slot, frequency);
}

float loadFMPreset(uint8_t slot) {
    if (slot >= FM_MAX_PRESETS) return 0.0;

    // Load from NVS
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);

    char key[16];
    snprintf(key, sizeof(key), "fm_preset_%d", slot);
    float frequency = prefs.getFloat(key, 0.0);

    prefs.end();

    return frequency;
}
