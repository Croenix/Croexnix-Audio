#include "wifi_manager.h"
#include <Preferences.h>
#include "../core/device_state.h"

// Preferences for persistent storage
Preferences preferences;

// Wi-Fi state
static bool wifiInitialized = false;
static bool apModeActive = false;
static unsigned long lastReconnectAttempt = 0;
static const unsigned long RECONNECT_INTERVAL = 30000; // 30 seconds

void initWiFiManager() {
    DEBUG_PRINTLN("[WIFI] Initializing Wi-Fi manager...");

    // Set Wi-Fi mode
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    // Try to load saved credentials
    String ssid, password;
    if (loadWiFiCredentials(ssid, password)) {
        DEBUG_PRINTF("[WIFI] Found saved credentials for SSID: %s\n", ssid.c_str());
        connectToWiFi(ssid.c_str(), password.c_str());
    } else {
        DEBUG_PRINTLN("[WIFI] No saved credentials found");
        DEBUG_PRINTLN("[WIFI] Starting in Access Point mode for provisioning...");
        startAccessPoint();
    }

    wifiInitialized = true;
}

void updateWiFiManager() {
    if (!wifiInitialized) return;

    // Update Wi-Fi state
    bool connected = WiFi.status() == WL_CONNECTED;

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        g_deviceState.wifiConnected = connected;
        g_deviceState.apMode = apModeActive;

        if (connected) {
            strncpy(g_deviceState.wifiSSID, WiFi.SSID().c_str(), MAX_SSID_LENGTH - 1);
            g_deviceState.wifiRSSI = WiFi.RSSI();
            strncpy(g_deviceState.localIP, WiFi.localIP().toString().c_str(), 15);
        }

        xSemaphoreGive(g_stateMutex);
    }

    // Auto-reconnect logic for STA mode
    if (!apModeActive && !connected) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            DEBUG_PRINTLN("[WIFI] Connection lost, attempting to reconnect...");
            WiFi.reconnect();
        }
    }
}

bool connectToWiFi(const char* ssid, const char* password) {
    DEBUG_PRINTF("[WIFI] Connecting to: %s\n", ssid);

    // Stop AP mode if active
    if (apModeActive) {
        stopAccessPoint();
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // Wait for connection (max 10 seconds)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("\n[WIFI] Connected successfully!");
        DEBUG_PRINTF("[WIFI] IP Address: %s\n", WiFi.localIP().toString().c_str());
        DEBUG_PRINTF("[WIFI] RSSI: %d dBm\n", WiFi.RSSI());

        // Start mDNS responder
        if (MDNS.begin(MDNS_HOSTNAME)) {
            MDNS.addService(MDNS_SERVICE_NAME, MDNS_SERVICE_PROTOCOL, MDNS_SERVICE_PORT);
            DEBUG_PRINTF("[WIFI] mDNS responder started: %s.local\n", MDNS_HOSTNAME);
        }

        // Save credentials
        saveWiFiCredentials(ssid, password);

        return true;
    } else {
        DEBUG_PRINTLN("\n[WIFI] Connection failed!");
        return false;
    }
}

void disconnectWiFi() {
    DEBUG_PRINTLN("[WIFI] Disconnecting...");
    WiFi.disconnect(true);
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void startAccessPoint() {
    DEBUG_PRINTLN("[WIFI] Starting Access Point mode...");

    WiFi.mode(WIFI_AP);

    bool success = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CONNECTIONS);

    if (success) {
        IPAddress IP = WiFi.softAPIP();
        DEBUG_PRINTF("[WIFI] AP started successfully!\n");
        DEBUG_PRINTF("[WIFI] SSID: %s\n", AP_SSID);
        DEBUG_PRINTF("[WIFI] IP Address: %s\n", IP.toString().c_str());

        apModeActive = true;

        // Start mDNS in AP mode
        if (MDNS.begin(MDNS_HOSTNAME)) {
            MDNS.addService(MDNS_SERVICE_NAME, MDNS_SERVICE_PROTOCOL, MDNS_SERVICE_PORT);
            DEBUG_PRINTF("[WIFI] mDNS responder started: %s.local\n", MDNS_HOSTNAME);
        }
    } else {
        DEBUG_PRINTLN("[WIFI] Failed to start Access Point!");
    }
}

void stopAccessPoint() {
    if (apModeActive) {
        DEBUG_PRINTLN("[WIFI] Stopping Access Point...");
        WiFi.softAPdisconnect(true);
        apModeActive = false;
    }
}

bool isAPMode() {
    return apModeActive;
}

int scanNetworks() {
    DEBUG_PRINTLN("[WIFI] Scanning for networks...");
    int n = WiFi.scanNetworks();
    DEBUG_PRINTF("[WIFI] Found %d networks\n", n);
    return n;
}

String getScannedNetwork(int index) {
    return WiFi.SSID(index);
}

int8_t getScannedRSSI(int index) {
    return WiFi.RSSI(index);
}

String getLocalIP() {
    if (apModeActive) {
        return WiFi.softAPIP().toString();
    } else {
        return WiFi.localIP().toString();
    }
}

String getSSID() {
    return WiFi.SSID();
}

int8_t getRSSI() {
    return WiFi.RSSI();
}

String getMACAddress() {
    return WiFi.macAddress();
}

void saveWiFiCredentials(const char* ssid, const char* password) {
    preferences.begin(NVS_NAMESPACE, false);
    preferences.putString(NVS_WIFI_SSID, ssid);
    preferences.putString(NVS_WIFI_PASS, password);
    preferences.end();

    DEBUG_PRINTLN("[WIFI] Credentials saved to NVS");
}

bool loadWiFiCredentials(String& ssid, String& password) {
    preferences.begin(NVS_NAMESPACE, true);
    ssid = preferences.getString(NVS_WIFI_SSID, "");
    password = preferences.getString(NVS_WIFI_PASS, "");
    preferences.end();

    return (ssid.length() > 0);
}

void clearWiFiCredentials() {
    preferences.begin(NVS_NAMESPACE, false);
    preferences.remove(NVS_WIFI_SSID);
    preferences.remove(NVS_WIFI_PASS);
    preferences.end();

    DEBUG_PRINTLN("[WIFI] Credentials cleared from NVS");
}
