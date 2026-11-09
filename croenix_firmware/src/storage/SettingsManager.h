#ifndef CROENIX_SETTINGS_MANAGER_H
#define CROENIX_SETTINGS_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class SettingsManager {
public:
  SettingsManager();
  void begin();

  // WiFi credentials
  bool saveWifiCredentials(const String& ssid, const String& password);
  String getWifiSSID();
  String getWifiPassword();
  bool hasWifiCredentials();
  void clearWifiCredentials();

  // Auth Token
  bool saveAuthToken(const String& token);
  String getAuthToken();
  bool hasAuthToken();
  void clearAuthToken();

  // Audio Settings
  bool saveVolume(int volume);
  int getVolume(int defaultVal = 50);
  bool saveMute(bool muted);
  bool getMute(bool defaultVal = false);

  // Device Name
  bool saveDeviceName(const String& name);
  String getDeviceName();

  // Factory reset
  void factoryReset();

private:
  Preferences _prefs;
};

#endif // CROENIX_SETTINGS_MANAGER_H

