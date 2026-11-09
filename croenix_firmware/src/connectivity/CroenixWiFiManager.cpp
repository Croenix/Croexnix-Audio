#include "CroenixWiFiManager.h"
#include "../storage/SettingsManager.h"
#include "../core/PlayerStateManager.h"
#include "../Config.h"

CroenixWiFiManager::CroenixWiFiManager(SettingsManager* settingsManager, PlayerStateManager* stateManager)
  : _settingsManager(settingsManager),
    _stateManager(stateManager),
    _networkState(CroenixNetworkState::WIFI_OFFLINE),
    _apActive(false),
    _staConnecting(false),
    _staConnectStartTime(0),
    _lastReconnectAttemptMs(0),
    _currentBackoffMs(WIFI_RECONNECT_INTERVAL_BASE_MS),
    _reconnectAttempts(0) {
}

void CroenixWiFiManager::begin() {
  CROENIX_LOG("WiFiManager", "Starting Dual-Band Coexistence Engine (AP + STA)...");

  _setupWiFiEvents();

  // Dual mode
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(DEVICE_HOSTNAME);

  // Start Local Access Point
  startAP();

  // Load router credentials from NVS
  if (_settingsManager && _settingsManager->hasWifiCredentials()) {
    _staTargetSsid = _settingsManager->getWifiSSID();
    _staTargetPassword = _settingsManager->getWifiPassword();
    CROENIX_LOG("WiFiManager", "Found saved router credentials for: %s", _staTargetSsid.c_str());
    _startSTAConnection();
  } else {
    CROENIX_LOG("WiFiManager", "No router credentials saved. Standing by in AP-only mode.");
  }

  _syncStateWithManager();
}

void CroenixWiFiManager::_setupWiFiEvents() {
  WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
      case ARDUINO_EVENT_WIFI_AP_START:
        _apActive = true;
        CROENIX_LOG("WiFiManager", "[EVENT] AP Started. IP: %s", WiFi.softAPIP().toString().c_str());
        break;

      case ARDUINO_EVENT_WIFI_AP_STOP:
        _apActive = false;
        CROENIX_LOG("WiFiManager", "[EVENT] AP Stopped.");
        break;

      case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        CROENIX_LOG("WiFiManager", "[EVENT] Client joined Hotspot. Total: %d", WiFi.softAPgetStationNum());
        _syncStateWithManager();
        break;

      case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        CROENIX_LOG("WiFiManager", "[EVENT] Client left Hotspot. Total: %d", WiFi.softAPgetStationNum());
        _syncStateWithManager();
        break;

      case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        CROENIX_LOG("WiFiManager", "[EVENT] Associated with router: %s", _staTargetSsid.c_str());
        break;

      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        _staConnecting = false;
        _reconnectAttempts = 0;
        _currentBackoffMs = WIFI_RECONNECT_INTERVAL_BASE_MS;
        CROENIX_LOG("WiFiManager", "[EVENT] Connected to Router! IP: %s, RSSI: %d dBm",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
        _syncStateWithManager();
        break;

      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        CROENIX_LOG("WiFiManager", "[EVENT] Router disconnected (reason: %d)", info.wifi_sta_disconnected.reason);
        if (_staConnecting) {
          _staConnecting = false;
        }
        if (_stateManager) {
          _stateManager->incrementWifiReconnectCount();
        }
        _calculateNextBackoffWithJitter();
        _syncStateWithManager();
        break;

      default:
        break;
    }
  });
}

bool CroenixWiFiManager::startAP() {
  _networkState = CroenixNetworkState::AP_STARTING;
  WiFi.softAPConfig(AP_IP_ADDRESS, AP_GATEWAY, AP_SUBNET);
  bool res = WiFi.softAP(AP_SSID_DEFAULT, AP_PASSWORD_DEFAULT, AP_CHANNEL_DEFAULT, false, AP_MAX_CONNECTIONS);
  if (res) {
    _apActive = true;
    _networkState = isSTAConnected() ? CroenixNetworkState::AP_STA_ONLINE : CroenixNetworkState::AP_ONLINE;
    CROENIX_LOG("WiFiManager", "AP Active: SSID='%s', IP='%s'", AP_SSID_DEFAULT, WiFi.softAPIP().toString().c_str());
  } else {
    _apActive = false;
    _networkState = CroenixNetworkState::NETWORK_ERROR;
    CROENIX_LOG("WiFiManager", "AP initialization FAILED!");
  }
  return res;
}

bool CroenixWiFiManager::connectToRouterAsync(const String& ssid, const String& password, bool saveCredentials) {
  if (ssid.isEmpty()) return false;

  CROENIX_LOG("WiFiManager", "Initiating async connection to router: %s", ssid.c_str());
  _staTargetSsid = ssid;
  _staTargetPassword = password;

  if (saveCredentials && _settingsManager) {
    _settingsManager->saveWifiCredentials(ssid, password);
  }

  _reconnectAttempts = 0;
  _currentBackoffMs = WIFI_RECONNECT_INTERVAL_BASE_MS;
  _startSTAConnection();
  return true;
}

void CroenixWiFiManager::_startSTAConnection() {
  if (_staTargetSsid.isEmpty()) return;

  _staConnecting = true;
  _staConnectStartTime = millis();
  _lastReconnectAttemptMs = millis();
  _networkState = _reconnectAttempts == 0 ? CroenixNetworkState::STA_CONNECTING : CroenixNetworkState::STA_RECONNECTING;

  WiFi.begin(_staTargetSsid.c_str(), _staTargetPassword.c_str());
  CROENIX_LOG("WiFiManager", "Attempting connection to '%s' (Attempt #%d)", _staTargetSsid.c_str(), _reconnectAttempts + 1);
}

void CroenixWiFiManager::disconnectSTA() {
  WiFi.disconnect(false);
  _staConnecting = false;
  _staTargetSsid = "";
  _staTargetPassword = "";
  _networkState = _apActive ? CroenixNetworkState::AP_ONLINE : CroenixNetworkState::WIFI_OFFLINE;
  _syncStateWithManager();
  CROENIX_LOG("WiFiManager", "STA interface disconnected by request.");
}

void CroenixWiFiManager::update() {
  // Ensure Hotspot is always ON for direct app connection
  if (!_apActive) {
    static unsigned long lastApRetry = 0;
    if (millis() - lastApRetry > 5000) {
      lastApRetry = millis();
      CROENIX_LOG("WiFiManager", "Hotspot dropped or inactive. Ensuring AP is active...");
      startAP();
      _syncStateWithManager();
    }
  }

  // Check connection timeout during connecting
  if (_staConnecting) {
    if (millis() - _staConnectStartTime > WIFI_CONNECT_TIMEOUT_MS) {
      CROENIX_LOG("WiFiManager", "Connection attempt to '%s' timed out after %d ms", _staTargetSsid.c_str(), WIFI_CONNECT_TIMEOUT_MS);
      _staConnecting = false;
      _calculateNextBackoffWithJitter();
      _syncStateWithManager();
    }
  }

  // Handle automatic reconnect backoff
  if (!isSTAConnected() && !_staConnecting && !_staTargetSsid.isEmpty()) {
    if (millis() - _lastReconnectAttemptMs >= _currentBackoffMs) {
      CROENIX_LOG("WiFiManager", "Reconnection interval reached. Retrying router...");
      _startSTAConnection();
    }
  }
}

void CroenixWiFiManager::_calculateNextBackoffWithJitter() {
  _reconnectAttempts++;
  // Exponential backoff
  unsigned long backoff = WIFI_RECONNECT_INTERVAL_BASE_MS * (1 << min(_reconnectAttempts, 4));
  if (backoff > WIFI_RECONNECT_INTERVAL_MAX_MS) {
    backoff = WIFI_RECONNECT_INTERVAL_MAX_MS;
  }
  // Random jitter (+/- 20%)
  long jitter = (long)(esp_random() % (backoff / 5)) - (long)(backoff / 10);
  _currentBackoffMs = (unsigned long)((long)backoff + jitter);
  _lastReconnectAttemptMs = millis();
  CROENIX_LOG("WiFiManager", "Next reconnection attempt scheduled in %lu ms", _currentBackoffMs);
}

void CroenixWiFiManager::_syncStateWithManager() {
  bool staConn = isSTAConnected();
  if (_apActive && staConn) {
    _networkState = CroenixNetworkState::AP_STA_ONLINE;
  } else if (staConn) {
    _networkState = CroenixNetworkState::STA_ONLINE;
  } else if (_apActive) {
    _networkState = _staConnecting ? CroenixNetworkState::STA_CONNECTING : CroenixNetworkState::AP_ONLINE;
  } else {
    _networkState = CroenixNetworkState::WIFI_OFFLINE;
  }

  if (_stateManager) {
    _stateManager->updateConnection(
      staConn,
      staConn ? WiFi.localIP().toString() : "",
      staConn ? WiFi.SSID() : "",
      staConn ? WiFi.RSSI() : 0,
      _apActive,
      _apActive ? WiFi.softAPgetStationNum() : 0
    );
  }
}

CroenixNetworkState CroenixWiFiManager::getNetworkState() {
  return _networkState;
}

const char* CroenixWiFiManager::getNetworkStateString() {
  switch (_networkState) {
    case CroenixNetworkState::WIFI_OFFLINE:     return "WIFI_OFFLINE";
    case CroenixNetworkState::AP_STARTING:      return "AP_STARTING";
    case CroenixNetworkState::AP_ONLINE:        return "AP_ONLINE";
    case CroenixNetworkState::STA_CONNECTING:   return "STA_CONNECTING";
    case CroenixNetworkState::STA_ONLINE:       return "STA_ONLINE";
    case CroenixNetworkState::AP_STA_ONLINE:    return "AP_STA_ONLINE";
    case CroenixNetworkState::STA_RECONNECTING: return "STA_RECONNECTING";
    case CroenixNetworkState::NETWORK_ERROR:    return "NETWORK_ERROR";
    default:                                    return "UNKNOWN";
  }
}

bool CroenixWiFiManager::isAPActive() {
  return _apActive;
}

bool CroenixWiFiManager::isSTAConnected() {
  return WiFi.status() == WL_CONNECTED;
}

int CroenixWiFiManager::getAPClientCount() {
  return _apActive ? WiFi.softAPgetStationNum() : 0;
}

String CroenixWiFiManager::getSTAIP() {
  return isSTAConnected() ? WiFi.localIP().toString() : "";
}

String CroenixWiFiManager::getAPIP() {
  return _apActive ? WiFi.softAPIP().toString() : "";
}

String CroenixWiFiManager::getSTASSID() {
  return isSTAConnected() ? WiFi.SSID() : "";
}

int CroenixWiFiManager::getSTARSSI() {
  return isSTAConnected() ? WiFi.RSSI() : 0;
}

