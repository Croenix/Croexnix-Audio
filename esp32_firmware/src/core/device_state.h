#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#include <Arduino.h>
#include "config.h"

// Global device state structure
struct DeviceState {
    // UI State Machine
    UIScreenState activeScreen = UI_SCREEN_BOOT_SPLASH;
    uint8_t selectedDiagModule = 0;       // 0: DAC, 1: FM, 2: SD Card, 3: System
    bool needDisplayRedraw = true;

    // Audio state
    AudioSource currentSource = SOURCE_FM_RADIO;
    uint8_t volume = 50;                  // 0-100
    bool isPlaying = false;
    bool isMuted = false;

    // Current track metadata (ID3 tags)
    char trackTitle[MAX_METADATA_LENGTH] = "";
    char trackArtist[MAX_METADATA_LENGTH] = "";
    char trackAlbum[MAX_METADATA_LENGTH] = "";
    uint32_t trackDuration = 0;           // seconds
    uint32_t trackPosition = 0;           // seconds
    uint16_t trackBitrate = 0;            // kbps

    // WCMCU DAC state
    bool dacOperational = true;
    uint32_t audioUnderruns = 0;
    uint32_t audioSampleRate = AUDIO_SAMPLE_RATE;
    uint8_t audioBits = AUDIO_BITS_PER_SAMPLE;

    // HW-322 FM Radio state
    bool fmOperational = false;
    float fmFrequency = 88.0;             // MHz
    uint8_t fmSignalStrength = 75;        // 0-100
    char fmStationName[MAX_METADATA_LENGTH] = "Croenix FM";
    bool fmAutoSeek = false;

    // Bluetooth state
    BluetoothState bluetoothState = BT_STATE_DISCONNECTED;
    bool bluetoothConnected = false;
    char bluetoothDevice[MAX_METADATA_LENGTH] = BT_DEVICE_NAME;

    // SD Card state
    bool sdCardMounted = false;
    uint64_t sdCardSize = 0;              // bytes
    uint64_t sdCardUsed = 0;              // bytes
    uint16_t sdTotalTracks = 0;
    uint16_t sdCurrentTrack = 0;
    char currentFolder[64] = "/MUSIC";

    // Network state
    bool wifiConnected = false;
    bool apMode = true;                   // Boot default: SoftAP mode
    char wifiSSID[MAX_SSID_LENGTH] = "";
    int8_t wifiRSSI = 0;                  // dBm
    char localIP[16] = "";
    char macAddress[18] = "";
    uint8_t connectedClients = 0;

    // System & Hardware state
    char deviceName[32] = DEVICE_NAME;
    char firmwareVersion[16] = FIRMWARE_VERSION;
    char compileDate[20] = __DATE__;
    char compileTime[16] = __TIME__;
    char chipModel[24] = "ESP32";
    uint8_t chipCores = 2;
    uint32_t cpuFreqMHz = 240;
    char sdkVersion[24] = "";
    uint32_t uptime = 0;                  // seconds
    uint8_t cpuUsageCore0 = 0;            // 0-100
    uint8_t cpuUsageCore1 = 0;            // 0-100
    uint32_t freeHeap = 0;                // bytes
    uint32_t totalHeap = 0;               // bytes
    uint32_t minFreeHeap = 0;             // bytes
    uint8_t heapUsagePercent = 0;         // 0-100%
    uint32_t flashChipSize = 0;           // bytes

    // EQ Preset
    uint8_t eqPreset = 0;                 // 0=Flat, 1=Rock, 2=Pop, etc.
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
void cycleAudioSource();
void setPlaying(bool playing);
void setBluetoothState(BluetoothState state, const char* deviceName = nullptr);
void setUIScreen(UIScreenState screen);
void nextDiagModule();
void prevDiagModule();

#endif // DEVICE_STATE_H
