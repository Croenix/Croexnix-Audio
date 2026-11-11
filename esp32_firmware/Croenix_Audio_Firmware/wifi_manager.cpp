#include "wifi_manager.h"
#include <Preferences.h>
#include "device_state.h"

// Preferences for persistent storage
static Preferences preferences;

// Wi-Fi state
static bool wifiInitialized = false;
static bool apModeActive = false;
static unsigned long lastReconnectAttempt = 0;
static const unsigned long RECONNECT_INTERVAL = 30000; // 30 seconds

void initWiFiManager() {
    DEBUG_PRINTLN("[WIFI] Initializing dual-mode Wi-Fi manager...");

    WiFi.persistent(false); // Clean memory control
    WiFi.setAutoReconnect(true);

    // Try to load saved credentials
    String ssid, password;
    bool hasSaved = loadWiFiCredentials(ssid, password);

    if (hasSaved && ssid.length() > 0) {
        DEBUG_PRINTF("[WIFI] Found saved home network credentials: %s\n", ssid.c_str());
        bool connected = connectToWiFi(ssid.c_str(), password.c_str());
        if (!connected) {
            DEBUG_PRINTLN("[WIFI] Could not connect to saved network. Starting secure AP mode for setup...");
            startAccessPoint();
        }
    } else {
        DEBUG_PRINTLN("[WIFI] No saved credentials. Starting secure AP mode for initial Flutter setup...");
        startAccessPoint();
    }

    wifiInitialized = true;
}

void updateWiFiManager() {
    if (!wifiInitialized) return;

    bool connected = (WiFi.status() == WL_CONNECTED);

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        g_deviceState.wifiConnected = connected;
        g_deviceState.apMode = apModeActive;
        strncpy(g_deviceState.macAddress, WiFi.macAddress().c_str(), sizeof(g_deviceState.macAddress) - 1);

        if (connected) {
            strncpy(g_deviceState.wifiSSID, WiFi.SSID().c_str(), MAX_SSID_LENGTH - 1);
            g_deviceState.wifiRSSI = WiFi.RSSI();
            strncpy(g_deviceState.localIP, WiFi.localIP().toString().c_str(), 15);
        } else if (apModeActive) {
            strncpy(g_deviceState.wifiSSID, AP_SSID, MAX_SSID_LENGTH - 1);
            g_deviceState.wifiRSSI = 0;
            strncpy(g_deviceState.localIP, WiFi.softAPIP().toString().c_str(), 15);
        } else {
            g_deviceState.localIP[0] = '\0';
            g_deviceState.wifiSSID[0] = '\0';
            g_deviceState.wifiRSSI = 0;
        }

        xSemaphoreGive(g_stateMutex);
    }

    // Auto-reconnect logic for STA mode
    if (!apModeActive && !connected) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            DEBUG_PRINTLN("[WIFI] Connection lost, attempting reconnection...");
            WiFi.reconnect();
        }
    }
}

bool connectToWiFi(const char* ssid, const char* password) {
    DEBUG_PRINTF("[WIFI] Connecting to home network: %s\n", ssid);

    // Stop AP mode to lock out unauthorized devices
    if (apModeActive) {
        stopAccessPoint();
    }

    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // Wait for connection with non-blocking delays (max 12 seconds)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 24) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("\n[WIFI] Connected to home network successfully!");
        DEBUG_PRINTF("[WIFI] IP Address: %s\n", WiFi.localIP().toString().c_str());
        DEBUG_PRINTF("[WIFI] RSSI: %d dBm\n", WiFi.RSSI());

        apModeActive = false;

        // Start mDNS responder
        if (MDNS.begin(MDNS_HOSTNAME)) {
            MDNS.addService(MDNS_SERVICE_NAME, MDNS_SERVICE_PROTOCOL, MDNS_SERVICE_PORT);
            DEBUG_PRINTF("[WIFI] mDNS responder active: %s.local\n", MDNS_HOSTNAME);
        }

        // Save valid credentials to NVS
        saveWiFiCredentials(ssid, password);
        return true;
    } else {
        DEBUG_PRINTLN("\n[WIFI] Failed to connect to home network!");
        // Re-open secure AP mode so device remains accessible for configuration
        startAccessPoint();
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
    DEBUG_PRINTLN("[WIFI] Starting Secure SoftAP Mode...");

    // Disconnect STA
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);

    // Secure WPA2 hotspot with password
    bool success = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CONNECTIONS);

    if (success) {
        IPAddress IP = WiFi.softAPIP();
        DEBUG_PRINTF("[WIFI] Secure AP active!\n");
        DEBUG_PRINTF("[WIFI] SSID: %s (WPA2)\n", AP_SSID);
        DEBUG_PRINTF("[WIFI] IP: %s\n", IP.toString().c_str());

        apModeActive = true;

        if (MDNS.begin(MDNS_HOSTNAME)) {
            MDNS.addService(MDNS_SERVICE_NAME, MDNS_SERVICE_PROTOCOL, MDNS_SERVICE_PORT);
        }
    } else {
        DEBUG_PRINTLN("[WIFI] Failed to start SoftAP!");
    }
}

void stopAccessPoint() {
    if (apModeActive) {
        DEBUG_PRINTLN("[WIFI] Shutting down SoftAP to secure device from unauthorized access...");
        WiFi.softAPdisconnect(true);
        apModeActive = false;
    }
}

bool isAPMode() {
    return apModeActive;
}

int scanNetworks() {
    DEBUG_PRINTLN("[WIFI] Scanning for available networks...");
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

    DEBUG_PRINTLN("[WIFI] Credentials safely persisted in NVS");
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
