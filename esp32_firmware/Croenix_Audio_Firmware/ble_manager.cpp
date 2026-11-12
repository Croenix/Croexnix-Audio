#include "ble_manager.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "config.h"
#include "device_state.h"
#include "audio_manager.h"
#include "wifi_manager.h"
#include "sd_manager.h"

static BLEServer* pServer = nullptr;
static BLECharacteristic* pCmdChar = nullptr;
static BLECharacteristic* pStatusChar = nullptr;
static BLECharacteristic* pInfoChar = nullptr;

static bool s_bleConnected = false;
static bool s_oldBleConnected = false;
static unsigned long s_lastBleBroadcast = 0;

// Forward declaration
static void handleBLECommand(const String& cmd);

class CroenixServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        s_bleConnected = true;
        DEBUG_PRINTLN("[BLE] Mobile application connected via BLE!");
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            g_deviceState.bluetoothState = BT_STATE_CONNECTED;
            strncpy(g_deviceState.bluetoothDevice, "Mobile Companion (BLE)", sizeof(g_deviceState.bluetoothDevice) - 1);
            g_deviceState.needDisplayRedraw = true;
            xSemaphoreGive(g_stateMutex);
        }
        broadcastBLEStatus();
    }

    void onDisconnect(BLEServer* pServer) override {
        s_bleConnected = false;
        DEBUG_PRINTLN("[BLE] Mobile application disconnected. Resuming BLE advertising...");
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            g_deviceState.bluetoothState = BT_STATE_DISCONNECTED;
            g_deviceState.bluetoothDevice[0] = '\0';
            g_deviceState.needDisplayRedraw = true;
            xSemaphoreGive(g_stateMutex);
        }
        pServer->startAdvertising();
    }
};

class CroenixCommandCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        String rxValue = pCharacteristic->getValue().c_str();
        if (rxValue.length() > 0) {
            DEBUG_PRINTF("[BLE:CMD] Received: %s\n", rxValue.c_str());
            handleBLECommand(rxValue);
        }
    }
};

static void handleBLECommand(const String& cmd) {
    // 1. Try JSON command first
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, cmd);

    if (!error) {
        const char* command = doc["command"];
        if (!command) return;

        if (strcmp(command, "play") == 0) {
            playAudio();
        } else if (strcmp(command, "pause") == 0) {
            pauseAudio();
        } else if (strcmp(command, "stop") == 0) {
            stopAudio();
        } else if (strcmp(command, "next_track") == 0) {
            nextTrack();
        } else if (strcmp(command, "prev_track") == 0) {
            previousTrack();
        } else if (strcmp(command, "volume") == 0) {
            int val = doc["value"] | 50;
            setMasterVolume(val);
        } else if (strcmp(command, "source") == 0) {
            int src = doc["value"] | 2;
            switchAudioSource((AudioSource)src);
        } else if (strcmp(command, "set_screen") == 0) {
            int sc = doc["value"] | 1;
            setUIScreen((UIScreenState)sc);
        } else if (strcmp(command, "set_tone") == 0) {
            int8_t b = doc["bass"] | 0;
            int8_t m = doc["mid"] | 0;
            int8_t t = doc["treble"] | 0;
            setHardwareTone(b, m, t);
        } else if (strcmp(command, "wifi_config") == 0) {
            const char* ssid = doc["ssid"];
            const char* pass = doc["password"];
            if (ssid && strlen(ssid) > 0) {
                DEBUG_PRINTF("[BLE] Received Wi-Fi credentials for: %s\n", ssid);
                saveWiFiCredentials(ssid, pass ? pass : "");
                connectToWiFi(ssid, pass ? pass : "");
            }
        } else if (strcmp(command, "play_index") == 0) {
            int idx = doc["index"] | 0;
            switchAudioSource(SOURCE_SD_CARD);
            setUIScreen(UI_SCREEN_NOW_PLAYING);
            playSDTrack(idx);
        } else if (strcmp(command, "reboot") == 0) {
            delay(200);
            ESP.restart();
        }
    } else {
        // 2. Fallback: Short text commands (PLAY, PAUSE, NEXT, PREV, VOL:xx, MUTE)
        String s = cmd;
        s.trim();
        if (s.equalsIgnoreCase("PLAY")) {
            playAudio();
        } else if (s.equalsIgnoreCase("PAUSE")) {
            pauseAudio();
        } else if (s.equalsIgnoreCase("NEXT")) {
            nextTrack();
        } else if (s.equalsIgnoreCase("PREV") || s.equalsIgnoreCase("PREVIOUS")) {
            previousTrack();
        } else if (s.startsWith("VOL:") || s.startsWith("VOLUME:")) {
            int idx = s.indexOf(':');
            int vol = s.substring(idx + 1).toInt();
            setMasterVolume(vol);
        } else if (s.equalsIgnoreCase("MUTE")) {
            setMasterVolume(0);
        } else if (s.startsWith("SCREEN:")) {
            int sc = s.substring(7).toInt();
            setUIScreen((UIScreenState)sc);
        }
    }

    // Always broadcast back immediate state telemetry after executing command
    broadcastBLEStatus();
}

void initBLEManager() {
    DEBUG_PRINTLN("[BLE] Initializing Croenix BLE GATT Server...");

    BLEDevice::init(CROENIX_DEVICE_NAME);
    BLEDevice::setMTU(512);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new CroenixServerCallbacks());

    BLEService* pService = pServer->createService(CROENIX_BLE_SERVICE_UUID);

    // Command Characteristic (Write)
    pCmdChar = pService->createCharacteristic(
        CROENIX_BLE_CHAR_CMD_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    pCmdChar->setCallbacks(new CroenixCommandCallbacks());

    // Status Characteristic (Read + Notify)
    pStatusChar = pService->createCharacteristic(
        CROENIX_BLE_CHAR_STATUS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pStatusChar->addDescriptor(new BLE2902());

    // Device Info Characteristic (Read)
    pInfoChar = pService->createCharacteristic(
        CROENIX_BLE_CHAR_INFO_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    pInfoChar->setValue("CROENIX|3.0.0-OPTIMIZED|ESP32");

    pService->start();

    // Start Advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(CROENIX_BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // Functions that help with iPhone connections
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    DEBUG_PRINTLN("[BLE] ✓ BLE Server active, advertising as 'CROENIX'");
}

void broadcastBLEStatus() {
    if (!s_bleConnected || !pStatusChar) return;

    StaticJsonDocument<256> doc;
    doc["type"] = "state";
    doc["source"] = (int)g_deviceState.currentSource;
    doc["vol"] = g_deviceState.volume;
    doc["play"] = isAudioPlaying();
    doc["screen"] = (int)g_deviceState.activeScreen;
    doc["sd"] = g_deviceState.sdCardMounted;
    doc["wifi"] = g_deviceState.wifiConnected;
    if (g_deviceState.wifiConnected) {
        doc["ip"] = g_deviceState.localIP;
    }
    doc["title"] = g_deviceState.trackTitle;
    doc["artist"] = g_deviceState.trackArtist;

    String jsonStr;
    serializeJson(doc, jsonStr);

    pStatusChar->setValue(jsonStr.c_str());
    pStatusChar->notify();
}

void updateBLEManager() {
    if (!s_bleConnected) return;

    unsigned long now = millis();
    if (now - s_lastBleBroadcast >= 1500) {
        s_lastBleBroadcast = now;
        broadcastBLEStatus();
    }
}

bool isBLEConnected() {
    return s_bleConnected;
}

