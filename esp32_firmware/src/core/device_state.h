#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#include <Arduino.h>
#include "config.h"

// Global device state structure
struct DeviceState {
    // Audio state
    AudioSource currentSource = SOURCE_NONE;
    uint8_t volume = 50;              // 0-100
    bool isPlaying = false;
    bool isMuted = false;

    // Current track metadata
    char trackTitle[MAX_METADATA_LENGTH] = "";
    char trackArtist[MAX_METADATA_LENGTH] = "";
    char trackAlbum[MAX_METADATA_LENGTH] = "";
    uint32_t trackDuration = 0;       // seconds
    uint32_t trackPosition = 0;       // seconds
    uint16_t trackBitrate = 0;        // kbps

    // FM Radio state
    float fmFrequency = 88.0;         // MHz
    uint8_t fmSignalStrength = 0;     // 0-100
    char fmStationName[MAX_METADATA_LENGTH] = "";

    // Network state
    bool wifiConnected = false;
    bool apMode = false;
    char wifiSSID[MAX_SSID_LENGTH] = "";
    int8_t wifiRSSI = 0;              // dBm
    char localIP[16] = "";
    uint8_t connectedClients = 0;

    // SD Card state
    bool sdCardMounted = false;
    uint64_t sdCardSize = 0;          // bytes
    uint64_t sdCardUsed = 0;          // bytes

    // Bluetooth state
    bool bluetoothConnected = false;
    char bluetoothDevice[MAX_METADATA_LENGTH] = "";

    // System state
    uint32_t uptime = 0;              // seconds
    uint8_t cpuUsageCore0 = 0;        // 0-100
    uint8_t cpuUsageCore1 = 0;        // 0-100
    uint32_t freeHeap = 0;            // bytes

    // EQ Preset
    uint8_t eqPreset = 0;             // 0=Flat, 1=Rock, 2=Pop, etc.
};

// Global state instance (extern declaration)
extern DeviceState g_deviceState;

// Thread-safe state access mutex
extern SemaphoreHandle_t g_stateMutex;

// Helper functions for thread-safe state access
void initDeviceState();
void updateMetadata(const char* title, const char* artist, const char* album);
void updateTrackProgress(uint32_t position, uint32_t duration);
void setVolume(uint8_t vol);
void setAudioSource(AudioSource source);
void setPlaying(bool playing);

#endif // DEVICE_STATE_H
