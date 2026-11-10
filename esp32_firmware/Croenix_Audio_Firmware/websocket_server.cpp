#include "websocket_server.h"
#include "wifi_manager.h"
#include "device_state.h"
#include "audio_manager.h"
#include "fm_radio.h"
#include "sd_manager.h"

// WebSocket server instance
WebSocketsServer webSocket = WebSocketsServer(WS_PORT);

// Broadcast timer
static unsigned long lastBroadcast = 0;
static const unsigned long BROADCAST_INTERVAL = 1000; // 1 second

void initWebSocketServer() {
    DEBUG_PRINTLN("[WEBSOCKET] Initializing WebSocket server...");

    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);

    DEBUG_PRINTF("[WEBSOCKET] Server started on port %d\n", WS_PORT);
}

void updateWebSocketServer() {
    webSocket.loop();

    // Periodic state broadcast
    unsigned long now = millis();
    if (now - lastBroadcast >= BROADCAST_INTERVAL) {
        lastBroadcast = now;
        broadcastDeviceState();
    }
}

void broadcastDeviceState() {
    if (webSocket.connectedClients() == 0) return;

    StaticJsonDocument<1024> doc;

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        doc["type"] = "state_update";
        doc["source"] = g_deviceState.currentSource;
        doc["volume"] = g_deviceState.volume;
        doc["playing"] = g_deviceState.isPlaying;
        doc["position"] = g_deviceState.trackPosition;
        doc["duration"] = g_deviceState.trackDuration;
        doc["bitrate"] = g_deviceState.trackBitrate;
        doc["eq_preset"] = g_deviceState.eqPreset;

        // Device & Firmware identification
        doc["device_name"] = g_deviceState.deviceName;
        doc["firmware_version"] = g_deviceState.firmwareVersion;
        doc["compile_date"] = g_deviceState.compileDate;
        doc["compile_time"] = g_deviceState.compileTime;
        doc["chip_model"] = g_deviceState.chipModel;
        doc["chip_cores"] = g_deviceState.chipCores;
        doc["cpu_freq"] = g_deviceState.cpuFreqMHz;
        doc["sdk_version"] = g_deviceState.sdkVersion;

        // System & Performance metrics
        doc["uptime"] = g_deviceState.uptime;
        doc["free_heap"] = g_deviceState.freeHeap;
        doc["total_heap"] = g_deviceState.totalHeap;
        doc["min_free_heap"] = g_deviceState.minFreeHeap;
        doc["heap_usage"] = g_deviceState.heapUsagePercent;
        doc["flash_size"] = g_deviceState.flashChipSize;
        doc["cpu0"] = g_deviceState.cpuUsageCore0;
        doc["cpu1"] = g_deviceState.cpuUsageCore1;

        // Network metrics
        doc["wifi_connected"] = isWiFiConnected();
        doc["ap_mode"] = isAPMode();
        doc["ssid"] = getSSID();
        doc["rssi"] = getRSSI();
        doc["local_ip"] = getLocalIP();
        doc["mac"] = getMACAddress();
        doc["clients"] = webSocket.connectedClients();

        // Bluetooth & SD & FM
        doc["bt_connected"] = g_deviceState.bluetoothConnected;
        doc["bt_device"] = g_deviceState.bluetoothDevice;
        doc["sd_mounted"] = g_deviceState.sdCardMounted;
        doc["sd_size"] = (uint32_t)(g_deviceState.sdCardSize / 1024);
        doc["sd_used"] = (uint32_t)(g_deviceState.sdCardUsed / 1024);
        doc["fm_frequency"] = g_deviceState.fmFrequency;
        doc["fm_rssi"] = g_deviceState.fmSignalStrength;

        xSemaphoreGive(g_stateMutex);
    }

    String output;
    serializeJson(doc, output);
    webSocket.broadcastTXT(output);
}

void broadcastMetadata() {
    if (webSocket.connectedClients() == 0) return;

    StaticJsonDocument<512> doc;

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        doc["type"] = "metadata";
        doc["title"] = g_deviceState.trackTitle;
        doc["artist"] = g_deviceState.trackArtist;
        doc["album"] = g_deviceState.trackAlbum;

        xSemaphoreGive(g_stateMutex);
    }

    String output;
    serializeJson(doc, output);
    webSocket.broadcastTXT(output);
}

void broadcastNetworkInfo() {
    if (webSocket.connectedClients() == 0) return;

    StaticJsonDocument<512> doc;

    doc["type"] = "network_info";
    doc["wifi_connected"] = isWiFiConnected();
    doc["ap_mode"] = isAPMode();
    doc["ssid"] = getSSID();
    doc["rssi"] = getRSSI();
    doc["local_ip"] = getLocalIP();
    doc["mac"] = getMACAddress();

    String output;
    serializeJson(doc, output);
    webSocket.broadcastTXT(output);
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            DEBUG_PRINTF("[WEBSOCKET] Client %u disconnected\n", num);
            break;

        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            DEBUG_PRINTF("[WEBSOCKET] Client %u connected from %s\n", num, ip.toString().c_str());

            // Send initial state to new client
            broadcastDeviceState();
            broadcastMetadata();
            broadcastNetworkInfo();
            break;
        }

        case WStype_TEXT: {
            DEBUG_PRINTF("[WEBSOCKET] Received from client %u: %s\n", num, payload);

            // Parse JSON command
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (error) {
                DEBUG_PRINTLN("[WEBSOCKET] Failed to parse JSON");
                return;
            }

            // Handle command
            const char* command = doc["command"];

            if (strcmp(command, "play") == 0) handlePlayCommand(doc);
            else if (strcmp(command, "pause") == 0) handlePauseCommand(doc);
            else if (strcmp(command, "stop") == 0) handleStopCommand(doc);
            else if (strcmp(command, "volume") == 0) handleVolumeCommand(doc);
            else if (strcmp(command, "source") == 0) handleSourceCommand(doc);
            else if (strcmp(command, "eq") == 0) handleEQCommand(doc);
            else if (strcmp(command, "wifi_config") == 0) handleWiFiConfigCommand(doc);
            else if (strcmp(command, "scan_networks") == 0) handleScanNetworksCommand(doc);
            else if (strcmp(command, "file_list") == 0) handleFileListCommand(doc);
            else if (strcmp(command, "load_file") == 0) handleLoadFileCommand(doc);
            else if (strcmp(command, "fm_frequency") == 0) handleFMFrequencyCommand(doc);
            else if (strcmp(command, "stream_url") == 0) handleStreamURLCommand(doc);
            else if (strcmp(command, "get_status") == 0 || strcmp(command, "get_info") == 0) {
                broadcastDeviceState();
                broadcastMetadata();
                broadcastNetworkInfo();
            }
            else if (strcmp(command, "reboot") == 0) {
                DEBUG_PRINTLN("[WEBSOCKET] Reboot requested by client");
                delay(200);
                ESP.restart();
            }
            else {
                DEBUG_PRINTF("[WEBSOCKET] Unknown command: %s\n", command);
            }

            break;
        }

        default:
            break;
    }
}

// ============================================
// Command Handlers
// ============================================

void handlePlayCommand(JsonDocument& doc) {
    playAudio();
    broadcastDeviceState();
}

void handlePauseCommand(JsonDocument& doc) {
    pauseAudio();
    broadcastDeviceState();
}

void handleStopCommand(JsonDocument& doc) {
    stopAudio();
    broadcastDeviceState();
}

void handleVolumeCommand(JsonDocument& doc) {
    uint8_t volume = doc["value"];
    setMasterVolume(volume);
    broadcastDeviceState();
}

void handleSourceCommand(JsonDocument& doc) {
    int source = doc["value"];
    switchAudioSource((AudioSource)source);
    broadcastDeviceState();
}

void handleEQCommand(JsonDocument& doc) {
    uint8_t preset = doc["value"];
    setEQPreset(preset);
    broadcastDeviceState();
}

void handleWiFiConfigCommand(JsonDocument& doc) {
    const char* ssid = doc["ssid"];
    const char* password = doc["password"];

    bool success = connectToWiFi(ssid, password);

    StaticJsonDocument<256> response;
    response["type"] = "wifi_config_result";
    response["success"] = success;

    String output;
    serializeJson(response, output);
    webSocket.broadcastTXT(output);
}

void handleScanNetworksCommand(JsonDocument& doc) {
    int n = scanNetworks();

    StaticJsonDocument<1024> response;
    response["type"] = "scan_result";
    response["count"] = n;

    JsonArray networks = response.createNestedArray("networks");
    for (int i = 0; i < n && i < 20; i++) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = getScannedNetwork(i);
        network["rssi"] = getScannedRSSI(i);
    }

    String output;
    serializeJson(response, output);
    webSocket.broadcastTXT(output);
}

void handleFileListCommand(JsonDocument& doc) {
    const char* path = doc["path"] | "/";

    StaticJsonDocument<2048> response;
    response["type"] = "file_list";

    JsonArray files = response.createNestedArray("files");
    listSDFiles(path, files);

    String output;
    serializeJson(response, output);
    webSocket.broadcastTXT(output);
}

void handleLoadFileCommand(JsonDocument& doc) {
    const char* filepath = doc["path"];

    bool success = false;
    String ext = String(filepath);
    ext.toLowerCase();

    if (ext.endsWith(".mp3")) {
        success = loadMP3File(filepath);
    } else if (ext.endsWith(".flac")) {
        success = loadFLACFile(filepath);
    } else if (ext.endsWith(".wav")) {
        success = loadWAVFile(filepath);
    }

    StaticJsonDocument<256> response;
    response["type"] = "load_result";
    response["success"] = success;
    response["file"] = filepath;

    String output;
    serializeJson(response, output);
    webSocket.broadcastTXT(output);
}

void handleFMFrequencyCommand(JsonDocument& doc) {
    float frequency = doc["value"];
    setFMFrequency(frequency);
    broadcastDeviceState();
}

void handleStreamURLCommand(JsonDocument& doc) {
    const char* url = doc["url"];
    bool success = connectToWebStream(url);

    StaticJsonDocument<256> response;
    response["type"] = "stream_result";
    response["success"] = success;
    response["url"] = url;

    String output;
    serializeJson(response, output);
    webSocket.broadcastTXT(output);
}
