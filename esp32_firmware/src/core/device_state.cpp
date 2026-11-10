#include "device_state.h"

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

    DEBUG_PRINTLN("Device state initialized");
}

void updateMetadata(const char* title, const char* artist, const char* album) {
    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
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

        xSemaphoreGive(g_stateMutex);
    }
}

void updateTrackProgress(uint32_t position, uint32_t duration) {
    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.trackPosition = position;
        g_deviceState.trackDuration = duration;
        xSemaphoreGive(g_stateMutex);
    }
}

void setVolume(uint8_t vol) {
    if (vol > 100) vol = 100;

    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.volume = vol;
        xSemaphoreGive(g_stateMutex);
    }
}

void setAudioSource(AudioSource source) {
    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.currentSource = source;
        xSemaphoreGive(g_stateMutex);
    }
}

void setPlaying(bool playing) {
    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.isPlaying = playing;
        xSemaphoreGive(g_stateMutex);
    }
}
