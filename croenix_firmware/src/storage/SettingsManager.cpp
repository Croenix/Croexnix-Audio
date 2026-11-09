#include "SettingsManager.h"
#include "../Config.h"

SettingsManager::SettingsManager() {
}

void SettingsManager::begin() {
  CROENIX_LOG("SettingsManager", "Storage initialized on namespace: %s", NVS_NAMESPACE);
}

bool SettingsManager::saveWifiCredentials(const String& ssid, const String& password) {
  if (ssid.isEmpty()) return false;
  if (!_prefs.begin(NVS_NAMESPACE, false)) return false;
  _prefs.putString(NVS_KEY_WIFI_SSID, ssid);
  _prefs.putString(NVS_KEY_WIFI_PASSWORD, password);
  _prefs.end();
  CROENIX_LOG("SettingsManager", "Saved WiFi credentials for SSID: %s", ssid.c_str());
  return true;
}

String SettingsManager::getWifiSSID() {
  if (!_prefs.begin(NVS_NAMESPACE, true)) return "";
  String ssid = _prefs.getString(NVS_KEY_WIFI_SSID, "");
  _prefs.end();
  return ssid;
}

String SettingsManager::getWifiPassword() {
  if (!_prefs.begin(NVS_NAMESPACE, true)) return "";
  String pass = _prefs.getString(NVS_KEY_WIFI_PASSWORD, "");
  _prefs.end();
  return pass;
}

bool SettingsManager::hasWifiCredentials() {
  return !getWifiSSID().isEmpty();
}

void SettingsManager::clearWifiCredentials() {
  if (!_prefs.begin(NVS_NAMESPACE, false)) return;
  _prefs.remove(NVS_KEY_WIFI_SSID);
  _prefs.remove(NVS_KEY_WIFI_PASSWORD);
  _prefs.end();
  CROENIX_LOG("SettingsManager", "Cleared WiFi credentials");
}

bool SettingsManager::saveAuthToken(const String& token) {
  if (token.isEmpty()) return false;
  if (!_prefs.begin(NVS_NAMESPACE, false)) return false;
  _prefs.putString(NVS_KEY_AUTH_TOKEN, token);
  _prefs.end();
  CROENIX_LOG("SettingsManager", "Saved auth token to NVS");
  return true;
}

String SettingsManager::getAuthToken() {
  if (!_prefs.begin(NVS_NAMESPACE, true)) return "";
  String token = _prefs.getString(NVS_KEY_AUTH_TOKEN, "");
  _prefs.end();
  return token;
}

bool SettingsManager::hasAuthToken() {
  return !getAuthToken().isEmpty();
}

void SettingsManager::clearAuthToken() {
  if (!_prefs.begin(NVS_NAMESPACE, false)) return;
  _prefs.remove(NVS_KEY_AUTH_TOKEN);
  _prefs.end();
  CROENIX_LOG("SettingsManager", "Cleared auth token");
}

bool SettingsManager::saveVolume(int volume) {
  if (!_prefs.begin(NVS_NAMESPACE, false)) return false;
  _prefs.putInt(NVS_KEY_VOLUME, volume);
  _prefs.end();
  return true;
}

int SettingsManager::getVolume(int defaultVal) {
  if (!_prefs.begin(NVS_NAMESPACE, true)) return defaultVal;
  int vol = _prefs.getInt(NVS_KEY_VOLUME, defaultVal);
  _prefs.end();
  return vol;
}

bool SettingsManager::saveMute(bool muted) {
  if (!_prefs.begin(NVS_NAMESPACE, false)) return false;
  _prefs.putBool(NVS_KEY_MUTED, muted);
  _prefs.end();
  return true;
}

bool SettingsManager::getMute(bool defaultVal) {
  if (!_prefs.begin(NVS_NAMESPACE, true)) return defaultVal;
  bool m = _prefs.getBool(NVS_KEY_MUTED, defaultVal);
  _prefs.end();
  return m;
}

bool SettingsManager::saveDeviceName(const String& name) {
  if (!_prefs.begin(NVS_NAMESPACE, false)) return false;
  _prefs.putString(NVS_KEY_DEVICE_NAME, name);
  _prefs.end();
  return true;
}

String SettingsManager::getDeviceName() {
  if (!_prefs.begin(NVS_NAMESPACE, true)) return DEVICE_NAME;
  String name = _prefs.getString(NVS_KEY_DEVICE_NAME, DEVICE_NAME);
  _prefs.end();
  return name;
}

void SettingsManager::factoryReset() {
  if (!_prefs.begin(NVS_NAMESPACE, false)) return;
  _prefs.clear();
  _prefs.end();
  CROENIX_LOG("SettingsManager", "Factory reset executed: NVS namespace cleared");
}

