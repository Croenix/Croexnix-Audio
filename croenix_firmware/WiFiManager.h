#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

enum class WiFiConnectionState {
  DISCONNECTED,
  CONNECTING,
  CONNECTED,
  FAILED
};

struct WiFiStatus {
  bool ap_active;
  int ap_clients;
  IPAddress ap_ip;

  WiFiConnectionState sta_state;
  bool sta_connected;
  IPAddress sta_ip;
  String sta_ssid;
  int sta_rssi;
};

class WiFiManager {
public:
  WiFiManager();
  void begin();
  void update();

  // AP + STA management
  bool startAP();
  bool connectToRouter(const String& ssid, const String& password, bool save = true);
  void disconnect();

  // Status
  WiFiStatus getStatus();
  bool isConnectedToRouter();
  bool isAPActive();
  int getAPClientCount();

  // Saved credentials
  bool hasSavedCredentials();
  void clearSavedCredentials();

private:
  Preferences _prefs;

  // STA state
  WiFiConnectionState _staState;
  unsigned long _lastConnectAttempt;
  unsigned long _reconnectInterval;
  int _reconnectAttempts;
  String _savedSSID;
  String _savedPassword;

  // AP state
  bool _apActive;

  void _loadSavedCredentials();
  void _attemptSTAConnection();
  void _handleSTAStateChange();
  String _getReconnectIntervalString();
};

#endif // WIFI_MANAGER_H
