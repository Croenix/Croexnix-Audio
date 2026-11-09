#include "WiFiManager.h"
#include "Config.h"

WiFiManager::WiFiManager()
  : _staState(WiFiConnectionState::DISCONNECTED),
    _lastConnectAttempt(0),
    _reconnectInterval(WIFI_RECONNECT_INTERVAL_MS),
    _reconnectAttempts(0),
    _apActive(false) {
}

void WiFiManager::begin() {
  DEBUG_PRINTLN("[WiFiManager] Initializing...");

  // Set WiFi mode to AP+STA
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(DEVICE_HOSTNAME);

  // Start Access Point
  startAP();

  // Load and attempt saved STA credentials
  _loadSavedCredentials();
  if (hasSavedCredentials()) {
    DEBUG_PRINT("[WiFiManager] Found saved credentials for: ");
    DEBUG_PRINTLN(_savedSSID);
    _attemptSTAConnection();
  } else {
    DEBUG_PRINTLN("[WiFiManager] No saved router credentials");
  }
}

bool WiFiManager::startAP() {
  DEBUG_PRINTLN("[WiFiManager] Starting Access Point...");
  DEBUG_PRINT("[WiFiManager] SSID: ");
  DEBUG_PRINTLN(AP_SSID);

  // Configure AP
  WiFi.softAPConfig(AP_IP_ADDRESS, AP_GATEWAY, AP_SUBNET);
  bool success = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, false, AP_MAX_CONNECTIONS);

  if (success) {
    _apActive = true;
    IPAddress IP = WiFi.softAPIP();
    DEBUG_PRINT("[WiFiManager] AP started successfully. IP: ");
    DEBUG_PRINTLN(IP);
  } else {
    _apActive = false;
    DEBUG_PRINTLN("[WiFiManager] AP start failed!");
  }

  return success;
}

bool WiFiManager::connectToRouter(const String& ssid, const String& password, bool save) {
  if (ssid.isEmpty()) {
    DEBUG_PRINTLN("[WiFiManager] Cannot connect: SSID is empty");
    return false;
  }

  DEBUG_PRINT("[WiFiManager] Connecting to router: ");
  DEBUG_PRINTLN(ssid);

  _savedSSID = ssid;
  _savedPassword = password;

  if (save) {
    // Save credentials to NVS
    _prefs.begin(NVS_NAMESPACE, false);
    _prefs.putString(NVS_KEY_WIFI_SSID, ssid);
    _prefs.putString(NVS_KEY_WIFI_PASSWORD, password);
    _prefs.end();
    DEBUG_PRINTLN("[WiFiManager] Credentials saved to NVS");
  }

  // Reset reconnect state
  _reconnectAttempts = 0;
  _reconnectInterval = WIFI_RECONNECT_INTERVAL_MS;

  // Attempt connection
  _attemptSTAConnection();

  return true;
}

void WiFiManager::disconnect() {
  DEBUG_PRINTLN("[WiFiManager] Disconnecting from router...");
  WiFi.disconnect(false); // Don't turn off WiFi, keep AP active
  _staState = WiFiConnectionState::DISCONNECTED;
}

void WiFiManager::update() {
  _handleSTAStateChange();

  // Check if we need to reconnect
  if (_staState == WiFiConnectionState::FAILED ||
      (_staState == WiFiConnectionState::DISCONNECTED && hasSavedCredentials())) {

    unsigned long now = millis();
    if (now - _lastConnectAttempt >= _reconnectInterval) {
      DEBUG_PRINT("[WiFiManager] Attempting reconnect (");
      DEBUG_PRINT(_reconnectAttempts + 1);
      DEBUG_PRINT(") in ");
      DEBUG_PRINTLN(_getReconnectIntervalString());
      _attemptSTAConnection();
    }
  }
}

WiFiStatus WiFiManager::getStatus() {
  WiFiStatus status;

  // AP status
  status.ap_active = _apActive;
  status.ap_clients = WiFi.softAPgetStationNum();
  status.ap_ip = WiFi.softAPIP();

  // STA status
  status.sta_state = _staState;
  status.sta_connected = WiFi.isConnected();
  status.sta_ip = WiFi.localIP();
  status.sta_ssid = WiFi.SSID();
  status.sta_rssi = WiFi.RSSI();

  return status;
}

bool WiFiManager::isConnectedToRouter() {
  return WiFi.isConnected();
}

bool WiFiManager::isAPActive() {
  return _apActive;
}

int WiFiManager::getAPClientCount() {
  return WiFi.softAPgetStationNum();
}

bool WiFiManager::hasSavedCredentials() {
  return !_savedSSID.isEmpty();
}

void WiFiManager::clearSavedCredentials() {
  DEBUG_PRINTLN("[WiFiManager] Clearing saved credentials");
  _prefs.begin(NVS_NAMESPACE, false);
  _prefs.remove(NVS_KEY_WIFI_SSID);
  _prefs.remove(NVS_KEY_WIFI_PASSWORD);
  _prefs.end();

  _savedSSID = "";
  _savedPassword = "";
}

// Private methods

void WiFiManager::_loadSavedCredentials() {
  _prefs.begin(NVS_NAMESPACE, true); // Read-only
  _savedSSID = _prefs.getString(NVS_KEY_WIFI_SSID, "");
  _savedPassword = _prefs.getString(NVS_KEY_WIFI_PASSWORD, "");
  _prefs.end();
}

void WiFiManager::_attemptSTAConnection() {
  if (_savedSSID.isEmpty()) {
    return;
  }

  _staState = WiFiConnectionState::CONNECTING;
  _lastConnectAttempt = millis();

  DEBUG_PRINT("[WiFiManager] Attempting to connect to: ");
  DEBUG_PRINTLN(_savedSSID);

  WiFi.begin(_savedSSID.c_str(), _savedPassword.c_str());

  // Wait for connection with timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < WIFI_CONNECT_TIMEOUT_MS) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    _staState = WiFiConnectionState::CONNECTED;
    _reconnectAttempts = 0;
    _reconnectInterval = WIFI_RECONNECT_INTERVAL_MS;

    DEBUG_PRINTLN("[WiFiManager] Connected to router!");
    DEBUG_PRINT("[WiFiManager] IP Address: ");
    DEBUG_PRINTLN(WiFi.localIP());
    DEBUG_PRINT("[WiFiManager] RSSI: ");
    DEBUG_PRINT(WiFi.RSSI());
    DEBUG_PRINTLN(" dBm");
  } else {
    _staState = WiFiConnectionState::FAILED;
    _reconnectAttempts++;

    // Exponential backoff
    _reconnectInterval = min(
      (unsigned long)(WIFI_RECONNECT_INTERVAL_MS * pow(2, _reconnectAttempts - 1)),
      (unsigned long)WIFI_MAX_RECONNECT_INTERVAL_MS
    );

    DEBUG_PRINT("[WiFiManager] Connection failed. Status: ");
    DEBUG_PRINTLN(WiFi.status());
    DEBUG_PRINT("[WiFiManager] Next retry in: ");
    DEBUG_PRINTLN(_getReconnectIntervalString());
  }
}

void WiFiManager::_handleSTAStateChange() {
  static WiFiConnectionState lastState = WiFiConnectionState::DISCONNECTED;

  if (_staState == WiFiConnectionState::CONNECTED && !WiFi.isConnected()) {
    // Connection lost
    DEBUG_PRINTLN("[WiFiManager] Connection to router lost!");
    _staState = WiFiConnectionState::DISCONNECTED;
  }

  if (_staState != lastState) {
    lastState = _staState;

    // State changed - could trigger events here in future
    switch (_staState) {
      case WiFiConnectionState::CONNECTING:
        DEBUG_PRINTLN("[WiFiManager] State: CONNECTING");
        break;
      case WiFiConnectionState::CONNECTED:
        DEBUG_PRINTLN("[WiFiManager] State: CONNECTED");
        break;
      case WiFiConnectionState::FAILED:
        DEBUG_PRINTLN("[WiFiManager] State: FAILED");
        break;
      case WiFiConnectionState::DISCONNECTED:
        DEBUG_PRINTLN("[WiFiManager] State: DISCONNECTED");
        break;
    }
  }
}

String WiFiManager::_getReconnectIntervalString() {
  if (_reconnectInterval < 1000) {
    return String(_reconnectInterval) + "ms";
  } else {
    return String(_reconnectInterval / 1000) + "s";
  }
}
