#include "device_state.h"
#include <Preferences.h>
#include "audio_manager.h"

// Global state instance
DeviceState g_deviceState;

// Mutex for thread-safe access
SemaphoreHandle_t g_stateMutex = nullptr;
SemaphoreHandle_t g_spiMutex = nullptr;

static Preferences s_prefs;
static bool s_nvsPending = false;
static unsigned long s_lastStateChange = 0;
static unsigned long s_lastNVSSave = 0;

void initDeviceState() {
    if (g_stateMutex == nullptr) {
        g_stateMutex = xSemaphoreCreateMutex();
    }
    if (g_spiMutex == nullptr) {
        g_spiMutex = xSemaphoreCreateMutex();
    }

    if (g_stateMutex == nullptr) {
        DEBUG_PRINTLN("ERROR: Failed to allocate state mutex!");
        return;
    }

    g_deviceState.activeScreen = UI_SCREEN_BOOT_SPLASH;
    g_deviceState.needDisplayRedraw = true;

    // Populate hardware telemetry
    strncpy(g_deviceState.chipModel, ESP.getChipModel(), sizeof(g_deviceState.chipModel) - 1);
    g_deviceState.chipModel[sizeof(g_deviceState.chipModel) - 1] = '\0';
    g_deviceState.chipCores = ESP.getChipCores();
    g_deviceState.cpuFreqMHz = ESP.getCpuFreqMHz();
    strncpy(g_deviceState.sdkVersion, ESP.getSdkVersion(), sizeof(g_deviceState.sdkVersion) - 1);
    g_deviceState.sdkVersion[sizeof(g_deviceState.sdkVersion) - 1] = '\0';
    g_deviceState.flashChipSize = ESP.getFlashChipSize();

    // Load persisted state from NVS
    loadStateFromNVS();

    DEBUG_PRINTLN("[STATE] Device state initialized with NVS retention");
}

void loadStateFromNVS() {
    s_prefs.begin(NVS_NAMESPACE, true); // Read-only mode

    // 1. Last active input mode
    uint8_t src = s_prefs.getUChar(NVS_LAST_SOURCE, (uint8_t)SOURCE_BLUETOOTH);
    if (src >= 1 && src <= 4) {
        g_deviceState.currentSource = (AudioSource)src;
    } else {
        g_deviceState.currentSource = SOURCE_BLUETOOTH;
    }

    // 2. Volume level
    g_deviceState.volume = s_prefs.getUChar(NVS_VOLUME, 50);
    if (g_deviceState.volume > 100) g_deviceState.volume = 50;

    // 3. HW-322 FM frequency
    g_deviceState.fmFrequency = s_prefs.getFloat(NVS_FM_FREQUENCY, 88.0f);
    if (g_deviceState.fmFrequency < FM_MIN_FREQUENCY || g_deviceState.fmFrequency > FM_MAX_FREQUENCY) {
        g_deviceState.fmFrequency = 88.0f;
    }

    // 4. Last played SD card track index
    g_deviceState.sdCurrentTrack = s_prefs.getUShort(NVS_SD_TRACK, 0);

    // EQ Preset
    g_deviceState.eqPreset = s_prefs.getUChar(NVS_EQ_PRESET, 0);

    s_prefs.end();

    DEBUG_PRINTF("[NVS] Loaded: Source=%d, Vol=%d%%, FM=%.1fMHz, SD_Track=%d\n",
                 g_deviceState.currentSource, g_deviceState.volume,
                 g_deviceState.fmFrequency, g_deviceState.sdCurrentTrack);
}

void requestNVSSave() {
    s_nvsPending = true;
    s_lastStateChange = millis();
}

void saveStateToNVS(bool force) {
    unsigned long now = millis();

    // If not forced: wait until state has settled for 1200ms without changes,
    // and ensure at least 1500ms between flash writes.
    if (!force) {
        if (!s_nvsPending) return;
        if (now - s_lastStateChange < 1200) return; // Wait for knob rotation to stop
        if (now - s_lastNVSSave < 1500) return;     // Rate limit flash writes
    }

    s_nvsPending = false;
    s_lastNVSSave = now;

    // Safely capture state values under mutex without blocking NVS write
    uint8_t src = (uint8_t)SOURCE_BLUETOOTH;
    uint8_t vol = 50;
    float fm = 88.0f;
    uint16_t sd = 0;
    uint8_t eq = 0;

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        src = (uint8_t)g_deviceState.currentSource;
        vol = g_deviceState.volume;
        fm = g_deviceState.fmFrequency;
        sd = g_deviceState.sdCurrentTrack;
        eq = g_deviceState.eqPreset;
        xSemaphoreGive(g_stateMutex);
    } else {
        // Mutex contended; retry on next tick
        s_nvsPending = true;
        return;
    }

    // Write to NVS flash outside of the mutex to prevent task starvation
    s_prefs.begin(NVS_NAMESPACE, false); // Read-Write mode
    s_prefs.putUChar(NVS_LAST_SOURCE, src);
    s_prefs.putUChar(NVS_VOLUME, vol);
    s_prefs.putFloat(NVS_FM_FREQUENCY, fm);
    s_prefs.putUShort(NVS_SD_TRACK, sd);
    s_prefs.putUChar(NVS_EQ_PRESET, eq);
    s_prefs.end();

    DEBUG_PRINTLN("[NVS] State preserved to flash");
}

void commitSDTrackIndexToNVS(uint16_t trackIdx) {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_deviceState.sdCurrentTrack = trackIdx;
        xSemaphoreGive(g_stateMutex);
    }
    s_prefs.begin(NVS_NAMESPACE, false);
    s_prefs.putUShort(NVS_SD_TRACK, trackIdx);
    s_prefs.end();
}

void updateMetadata(const char* title, const char* artist, const char* album) {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
        if (title) {
            strncpy(g_deviceState.trackTitle, title, MAX_METADATA_LENGTH - 1);
            g_deviceState.trackTitle[MAX_METADATA_LENGTH - 1] = '\0';
        }
        if (artist) {
            strncpy(g_deviceState.trackArtist, artist, MAX_METADATA_LENGTH - 1);
            g_deviceState.trackArtist[MAX_METADATA_LENGTH - 1] = '\0';
        }
        if (album) {
            strncpy(g_deviceState.trackAlbum, album, MAX_METADATA_LENGTH - 1);
            g_deviceState.trackAlbum[MAX_METADATA_LENGTH - 1] = '\0';
        }
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
}

void updateTrackProgress(uint32_t position, uint32_t duration) {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_deviceState.trackPosition = position;
        g_deviceState.trackDuration = duration;
        xSemaphoreGive(g_stateMutex);
    }
}

void setVolume(uint8_t vol) {
    if (vol > 100) vol = 100;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_deviceState.volume = vol;
        xSemaphoreGive(g_stateMutex);
    }
    requestNVSSave();
}

void setAudioSource(AudioSource source) {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
        g_deviceState.currentSource = source;
        g_deviceState.fmTuneWindowActive = false; // Reset FM tune override
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
    requestNVSSave();
}

// Single-Press Input Mode Cycling Order: Bluetooth -> SD Card -> FM -> Web FM
void cycleAudioSource() {
    AudioSource nextSrc = SOURCE_BLUETOOTH;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
        switch (g_deviceState.currentSource) {
            case SOURCE_BLUETOOTH:   nextSrc = SOURCE_SD_CARD;    break;
            case SOURCE_SD_CARD:     nextSrc = SOURCE_FM_RADIO;   break;
            case SOURCE_FM_RADIO:    nextSrc = SOURCE_WEB_STREAM; break; // Web FM
            case SOURCE_WEB_STREAM:  nextSrc = SOURCE_BLUETOOTH;  break;
            default:                 nextSrc = SOURCE_BLUETOOTH;  break;
        }
        xSemaphoreGive(g_stateMutex);
    }
    switchAudioSource(nextSrc);
}

void setPlaying(bool playing) {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_deviceState.isPlaying = playing;
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
}

void setBluetoothState(BluetoothState state, const char* deviceName) {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
        g_deviceState.bluetoothState = state;
        g_deviceState.bluetoothConnected = (state == BT_STATE_CONNECTED);
        if (deviceName && strlen(deviceName) > 0) {
            strncpy(g_deviceState.bluetoothDevice, deviceName, MAX_METADATA_LENGTH - 1);
            g_deviceState.bluetoothDevice[MAX_METADATA_LENGTH - 1] = '\0';
        }
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
}

// -------------------------------------------------------------
// FM Tuning Override State Machine (5-second auto-revert window)
// -------------------------------------------------------------

void triggerFMTuneWindow() {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_deviceState.fmTuneWindowActive = true;
        g_deviceState.fmTuneExpiresAt = millis() + FM_TUNE_WINDOW_MS;
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
    DEBUG_PRINTLN("[STATE] FM Tune Mode Activated (5-Second Window)");
}

bool isFMTuneWindowActive() {
    return g_deviceState.fmTuneWindowActive && (millis() < g_deviceState.fmTuneExpiresAt);
}

void checkFMTuneExpiration() {
    if (g_deviceState.fmTuneWindowActive && (millis() >= g_deviceState.fmTuneExpiresAt)) {
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            g_deviceState.fmTuneWindowActive = false;
            g_deviceState.needDisplayRedraw = true;
            xSemaphoreGive(g_stateMutex);
        }
        DEBUG_PRINTLN("[STATE] FM Tune Window Expired -> Reverted to Volume Control");
    }
}

// -------------------------------------------------------------
// UI Screen Navigation
// -------------------------------------------------------------

void setUIScreen(UIScreenState screen) {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
        g_deviceState.activeScreen = screen;
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
}

void nextDiagModule() {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_deviceState.selectedDiagModule = (g_deviceState.selectedDiagModule + 1) % 4;
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
}

void prevDiagModule() {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_deviceState.selectedDiagModule = (g_deviceState.selectedDiagModule + 3) % 4;
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
}
