#include "device_state.h"
#include "audio_manager.h"

// Global state instance
DeviceState g_deviceState;

// Mutex for thread-safe access
SemaphoreHandle_t g_stateMutex = nullptr;

void initDeviceState() {
    g_stateMutex = xSemaphoreCreateMutex();

    if (g_stateMutex == nullptr) {
        DEBUG_PRINTLN("ERROR: Failed to create state mutex!");
        return;
    }

    // Populate hardware & firmware identification
    strncpy(g_deviceState.deviceName, DEVICE_NAME, sizeof(g_deviceState.deviceName) - 1);
    strncpy(g_deviceState.firmwareVersion, FIRMWARE_VERSION, sizeof(g_deviceState.firmwareVersion) - 1);
    strncpy(g_deviceState.bluetoothDevice, BT_DEVICE_NAME, sizeof(g_deviceState.bluetoothDevice) - 1);
    strncpy(g_deviceState.chipModel, ESP.getChipModel(), sizeof(g_deviceState.chipModel) - 1);
    g_deviceState.chipCores = ESP.getChipCores();
    g_deviceState.cpuFreqMHz = ESP.getCpuFreqMHz();
    strncpy(g_deviceState.sdkVersion, ESP.getSdkVersion(), sizeof(g_deviceState.sdkVersion) - 1);
    g_deviceState.totalHeap = ESP.getHeapSize();
    g_deviceState.freeHeap = ESP.getFreeHeap();
    g_deviceState.minFreeHeap = ESP.getMinFreeHeap();
    g_deviceState.flashChipSize = ESP.getFlashChipSize();

    g_deviceState.activeScreen = UI_SCREEN_BOOT_SPLASH;
    g_deviceState.currentSource = SOURCE_FM_RADIO;
    g_deviceState.bluetoothState = BT_STATE_DISCONNECTED;
    g_deviceState.needDisplayRedraw = true;

    DEBUG_PRINTLN("[STATE] Device state initialized (Croenix)");
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
}

void setAudioSource(AudioSource source) {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
        g_deviceState.currentSource = source;
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
}

void cycleAudioSource() {
    AudioSource nextSrc = SOURCE_FM_RADIO;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
        switch (g_deviceState.currentSource) {
            case SOURCE_FM_RADIO:    nextSrc = SOURCE_WEB_STREAM; break; // Music Player
            case SOURCE_WEB_STREAM:  nextSrc = SOURCE_BLUETOOTH;  break;
            case SOURCE_BLUETOOTH:   nextSrc = SOURCE_SD_CARD;    break;
            case SOURCE_SD_CARD:     nextSrc = SOURCE_FM_RADIO;   break;
            default:                 nextSrc = SOURCE_FM_RADIO;   break;
        }
        g_deviceState.currentSource = nextSrc;
        g_deviceState.needDisplayRedraw = true;
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
        }
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
}

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
