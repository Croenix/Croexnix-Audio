#include "fm_radio.h"
#include <SI470X.h>
#include <Wire.h>
#include <Preferences.h>
#include "device_state.h"

// Si4703 FM radio instance (using PU2CLR_SI470X library)
static SI470X radio;
static bool fmInitialized = false;

void initFMRadio() {
    DEBUG_PRINTLN("[FM] Initializing Si4703 FM radio...");

    Wire.begin(FM_SDA, FM_SCL);

    // Initialize Si4703 with Reset and SDA pins
    radio.setup(FM_RST, FM_SDA);
    radio.setVolume(15);
    radio.setBand(0);    // 0 = 87.5–108 MHz (US / Europe)
    radio.setSpace(1);   // 1 = 100 kHz spacing

    // Set to default frequency: 88.0 MHz (8800 in 10 kHz units)
    radio.setFrequency(8800);

    fmInitialized = true;
    DEBUG_PRINTLN("[FM] Si4703 initialized successfully");
}

void setFMFrequency(float frequency) {
    if (!fmInitialized) return;

    if (frequency < FM_MIN_FREQUENCY) frequency = FM_MIN_FREQUENCY;
    if (frequency > FM_MAX_FREQUENCY) frequency = FM_MAX_FREQUENCY;

    // Convert MHz to 10 kHz units (e.g., 88.0 MHz -> 8800)
    uint16_t freqInt = (uint16_t)(frequency * 100.0f + 0.5f);
    radio.setFrequency(freqInt);

    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.fmFrequency = frequency;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTF("[FM] Frequency set to: %.1f MHz\n", frequency);
}

float getFMFrequency() {
    if (!fmInitialized) return 0.0f;

    uint16_t freq = radio.getFrequency();
    return (float)freq / 100.0f;
}

void seekFMUp() {
    if (!fmInitialized) return;

    DEBUG_PRINTLN("[FM] Seeking up...");
    radio.seek(0, 1); // seek_mode=0 (wrap), direction=1 (up)

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
    radio.seek(0, 0); // seek_mode=0 (wrap), direction=0 (down)

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
    int rssi = radio.getRssi();
    if (rssi < 0) rssi = 0;
    if (rssi > 75) rssi = 75;

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

    // RDS Station Name if available
    if (radio.getRdsReady()) {
        char* name = radio.getRdsStationName();
        if (name && strlen(name) > 0) {
            return String(name);
        }
    }

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
