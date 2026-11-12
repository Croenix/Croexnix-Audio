#include "wifi_manager.h"
#include <Preferences.h>
#include "device_state.h"

// Preferences for persistent storage
static Preferences preferences;

// Wi-Fi state (Station-Only, NO SoftAP Hotspot)
static bool wifiInitialized = false;
static unsigned long lastReconnectAttempt = 0;
static const unsigned long RECONNECT_INTERVAL = 30000; // 30 seconds
static String s_savedSSID = "";
static String s_savedPass = "";

void initWiFiManager() {
    DEBUG_PRINTLN("[WIFI] Initializing Station-Only Wi-Fi (No Hotspot Mode)...");

    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_STA);

    // Try to load saved credentials
    bool hasSaved = loadWiFiCredentials(s_savedSSID, s_savedPass);

    if (hasSaved && s_savedSSID.length() > 0) {
        DEBUG_PRINTF("[WIFI] Found saved home network credentials: %s\n", s_savedSSID.c_str());
        // Non-blocking attempt - if offline, device continues without hanging
        connectToWiFi(s_savedSSID.c_str(), s_savedPass.c_str());
    } else {
        DEBUG_PRINTLN("[WIFI] No saved Wi-Fi credentials. Operating in Pure Bluetooth/Offline mode.");
        WiFi.disconnect(true);
    }

    wifiInitialized = true;
}

void updateWiFiManager() {
    if (!wifiInitialized) return;

    bool connected = (WiFi.status() == WL_CONNECTED);

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        g_deviceState.wifiConnected = connected;
        g_deviceState.apMode = false; // Hotspot is disabled
        strncpy(g_deviceState.macAddress, WiFi.macAddress().c_str(), sizeof(g_deviceState.macAddress) - 1);

        if (connected) {
            strncpy(g_deviceState.wifiSSID, WiFi.SSID().c_str(), MAX_SSID_LENGTH - 1);
            g_deviceState.wifiRSSI = WiFi.RSSI();
            strncpy(g_deviceState.localIP, WiFi.localIP().toString().c_str(), 15);
        } else {
            g_deviceState.localIP[0] = '\0';
            g_deviceState.wifiSSID[0] = '\0';
            g_deviceState.wifiRSSI = 0;
        }

        xSemaphoreGive(g_stateMutex);
    }

    // Auto-reconnect logic for saved STA network
    if (!connected && s_savedSSID.length() > 0) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            DEBUG_PRINTLN("[WIFI] Connection lost, retrying home network...");
            WiFi.begin(s_savedSSID.c_str(), s_savedPass.c_str());
        }
    }
}

bool connectToWiFi(const char* ssid, const char* password) {
    if (!ssid || strlen(ssid) == 0) return false;

    DEBUG_PRINTF("[WIFI] Connecting to home Wi-Fi: %s\n", ssid);

    WiFi.disconnect(true);
    delay(50);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // Wait up to 8 seconds for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 16) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("\n[WIFI] ✓ Connected to home Wi-Fi network!");
        DEBUG_PRINTF("[WIFI] Local IP: %s\n", WiFi.localIP().toString().c_str());
        DEBUG_PRINTF("[WIFI] Signal: %d dBm\n", WiFi.RSSI());

        s_savedSSID = ssid;
        s_savedPass = password ? password : "";

        // Start mDNS responder
        if (MDNS.begin(MDNS_HOSTNAME)) {
            MDNS.addService(MDNS_SERVICE_NAME, MDNS_SERVICE_PROTOCOL, MDNS_SERVICE_PORT);
            DEBUG_PRINTF("[WIFI] mDNS responder active: %s.local\n", MDNS_HOSTNAME);
        }

        // Save valid credentials to NVS
        saveWiFiCredentials(ssid, password ? password : "");
        return true;
    } else {
        DEBUG_PRINTLN("\n[WIFI] ✗ Wi-Fi connection timed out. Remaining in Bluetooth mode.");
        return false;
    }
}

void disconnectWiFi() {
    DEBUG_PRINTLN("[WIFI] Disconnecting Wi-Fi...");
    WiFi.disconnect(true);
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void startAccessPoint() {
    // Disabled by design: User requested no hotspot mode to prevent phone data disconnection
    DEBUG_PRINTLN("[WIFI] SoftAP/Hotspot is disabled. Device uses Bluetooth Low Energy for configuration.");
}

void stopAccessPoint() {
    WiFi.softAPdisconnect(true);
}

bool isAPMode() {
    return false; // SoftAP is never active
}

String getLocalIP() {
    return isWiFiConnected() ? WiFi.localIP().toString() : "";
}

String getSSID() {
    return isWiFiConnected() ? WiFi.SSID() : "";
}

int getRSSI() {
    return isWiFiConnected() ? WiFi.RSSI() : 0;
}

String getMACAddress() {
    return WiFi.macAddress();
}

bool saveWiFiCredentials(const char* ssid, const char* password) {
    if (!ssid || strlen(ssid) == 0) return false;

    preferences.begin("croenix_wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password ? password : "");
    preferences.end();

    DEBUG_PRINTLN("[WIFI] Credentials safely saved to NVS");
    return true;
}

bool loadWiFiCredentials(String& ssid, String& password) {
    preferences.begin("croenix_wifi", true);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    preferences.end();

    return (ssid.length() > 0);
}

void clearWiFiCredentials() {
    preferences.begin("croenix_wifi", false);
    preferences.clear();
    preferences.end();
    s_savedSSID = "";
    s_savedPass = "";
    DEBUG_PRINTLN("[WIFI] Credentials cleared from NVS");
}
