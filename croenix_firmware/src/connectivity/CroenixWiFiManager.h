#ifndef CROENIX_WIFI_MANAGER_H
#define CROENIX_WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class SettingsManager;
class PlayerStateManager;

enum class CroenixNetworkState {
  WIFI_OFFLINE,
  AP_STARTING,
  AP_ONLINE,
  STA_CONNECTING,
  STA_ONLINE,
  AP_STA_ONLINE,
  STA_RECONNECTING,
  NETWORK_ERROR
};

class CroenixWiFiManager {
public:
  CroenixWiFiManager(SettingsManager* settingsManager, PlayerStateManager* stateManager);
  void begin();
  void update();

  bool startAP();
  bool connectToRouterAsync(const String& ssid, const String& password, bool saveCredentials = true);
  void disconnectSTA();

  CroenixNetworkState getNetworkState();
  const char* getNetworkStateString();

  bool isAPActive();
  bool isSTAConnected();
  int getAPClientCount();
  String getSTAIP();
  String getAPIP();
  String getSTASSID();
  int getSTARSSI();

private:
  SettingsManager* _settingsManager;
  PlayerStateManager* _stateManager;

  CroenixNetworkState _networkState;
  bool _apActive;
  bool _staConnecting;

  unsigned long _staConnectStartTime;
  unsigned long _lastReconnectAttemptMs;
  unsigned long _currentBackoffMs;
  int _reconnectAttempts;

  String _staTargetSsid;
  String _staTargetPassword;

  void _setupWiFiEvents();
  void _startSTAConnection();
  void _calculateNextBackoffWithJitter();
  void _syncStateWithManager();
};

#endif // CROENIX_WIFI_MANAGER_H

