#include "PlayerStateManager.h"
#include "EventManager.h"
#include "../Config.h"
#include <WiFi.h>

PlayerStateManager::PlayerStateManager() {
  _mutex = xSemaphoreCreateMutex();
}

void PlayerStateManager::begin() {
  _lock();

  _state.device_id = _generateDeviceId();

  // Device Info
  _state.device.name = DEVICE_NAME;
  _state.device.firmware_version = FIRMWARE_VERSION;
  _state.device.uptime_seconds = 0;

  // Connection Info
  _state.connection.router_connected = false;
  _state.connection.router_ip = "";
  _state.connection.router_ssid = "";
  _state.connection.router_rssi = 0;
  _state.connection.hotspot_active = false;
  _state.connection.hotspot_clients = 0;
  _state.connection.active_interface = "none";

  // Audio Info
  _state.audio.source = "none";
  _state.audio.status = "stopped";
  _state.audio.volume = DEFAULT_VOLUME;
  _state.audio.muted = false;
  _state.audio.codec = "";
  _state.audio.sample_rate = 0;
  _state.audio.bit_depth = 0;
  _state.audio.channels = 0;
  _state.audio.current_track = "";
  _state.audio.current_artist = "";
  _state.audio.duration_ms = 0;
  _state.audio.position_ms = 0;

  // Storage Info
  _state.storage.sd_inserted = false;
  _state.storage.total_bytes = 0;
  _state.storage.used_bytes = 0;
  _state.storage.free_bytes = 0;

  // System Info
  _state.system.free_heap = esp_get_free_heap_size();
  _state.system.total_heap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
  _state.system.min_free_heap = esp_get_minimum_free_heap_size();
  _state.system.restart_reason = "power_on";
  _state.system.wifi_reconnect_count = 0;
  _state.system.ble_active = false;
  _state.system.ble_connected = false;

  _unlock();

  CROENIX_LOG("StateManager", "Initialized with Device ID: %s", _state.device_id.c_str());
}

void PlayerStateManager::update() {
  _lock();
  _state.device.uptime_seconds = millis() / 1000;
  _unlock();
}

PlayerState PlayerStateManager::getState() {
  _lock();
  PlayerState copy = _state;
  _unlock();
  return copy;
}

String PlayerStateManager::getDeviceId() {
  _lock();
  String id = _state.device_id;
  _unlock();
  return id;
}

bool PlayerStateManager::setVolume(int volume, bool publish) {
  if (volume < 0) volume = 0;
  if (volume > 100) volume = 100;

  int oldVol;
  _lock();
  oldVol = _state.audio.volume;
  _state.audio.volume = volume;
  _unlock();

  if (oldVol != volume && publish) {
    JsonDocument doc;
    doc["volume"] = volume;
    String payload;
    serializeJson(doc, payload);
    EventManager::getInstance().dispatchEvent(EventType::VOLUME_CHANGED, "volume_changed", payload);
  }
  return true;
}

bool PlayerStateManager::setMute(bool muted, bool publish) {
  bool oldMute;
  _lock();
  oldMute = _state.audio.muted;
  _state.audio.muted = muted;
  _unlock();

  if (oldMute != muted && publish) {
    JsonDocument doc;
    doc["muted"] = muted;
    String payload;
    serializeJson(doc, payload);
    EventManager::getInstance().dispatchEvent(EventType::MUTE_CHANGED, "mute_changed", payload);
  }
  return true;
}

bool PlayerStateManager::setSource(const String& source, bool publish) {
  String oldSource;
  _lock();
  oldSource = _state.audio.source;
  _state.audio.source = source;
  _unlock();

  if (oldSource != source && publish) {
    JsonDocument doc;
    doc["source"] = source;
    String payload;
    serializeJson(doc, payload);
    EventManager::getInstance().dispatchEvent(EventType::SOURCE_CHANGED, "source_changed", payload);
  }
  return true;
}

bool PlayerStateManager::setPlaybackStatus(const String& status, bool publish) {
  String oldStatus;
  _lock();
  oldStatus = _state.audio.status;
  _state.audio.status = status;
  _unlock();

  if (oldStatus != status && publish) {
    JsonDocument doc;
    doc["status"] = status;
    String payload;
    serializeJson(doc, payload);
    EventManager::getInstance().dispatchEvent(EventType::PLAYBACK_CHANGED, "playback_changed", payload);
  }
  return true;
}

void PlayerStateManager::setTrackMetadata(const String& title, const String& artist, unsigned long durationMs) {
  _lock();
  _state.audio.current_track = title;
  _state.audio.current_artist = artist;
  _state.audio.duration_ms = durationMs;
  _unlock();

  JsonDocument doc;
  doc["title"] = title;
  doc["artist"] = artist;
  doc["duration_ms"] = durationMs;
  String payload;
  serializeJson(doc, payload);
  EventManager::getInstance().dispatchEvent(EventType::TRACK_CHANGED, "track_changed", payload);
}

void PlayerStateManager::setPlaybackPosition(unsigned long positionMs) {
  _lock();
  _state.audio.position_ms = positionMs;
  _unlock();

  JsonDocument doc;
  doc["position_ms"] = positionMs;
  String payload;
  serializeJson(doc, payload);
  EventManager::getInstance().dispatchEvent(EventType::POSITION_CHANGED, "position_changed", payload);
}

void PlayerStateManager::updateConnection(bool routerConnected, const String& routerIp,
                                          const String& routerSsid, int routerRssi,
                                          bool hotspotActive, int hotspotClients) {
  _lock();
  _state.connection.router_connected = routerConnected;
  _state.connection.router_ip = routerIp;
  _state.connection.router_ssid = routerSsid;
  _state.connection.router_rssi = routerRssi;
  _state.connection.hotspot_active = hotspotActive;
  _state.connection.hotspot_clients = hotspotClients;

  if (routerConnected) {
    _state.connection.active_interface = "router";
  } else if (hotspotActive) {
    _state.connection.active_interface = "hotspot";
  } else {
    _state.connection.active_interface = "none";
  }
  _unlock();

  JsonDocument doc;
  doc["router_connected"] = routerConnected;
  doc["router_ip"] = routerIp;
  doc["router_ssid"] = routerSsid;
  doc["router_rssi"] = routerRssi;
  doc["hotspot_active"] = hotspotActive;
  doc["hotspot_clients"] = hotspotClients;
  doc["active_interface"] = routerConnected ? "router" : (hotspotActive ? "hotspot" : "none");
  String payload;
  serializeJson(doc, payload);
  EventManager::getInstance().dispatchEvent(EventType::WIFI_CHANGED, "wifi_changed", payload);
}

void PlayerStateManager::updateSystemDiagnostics(unsigned long freeHeap, unsigned long totalHeap,
                                                 unsigned long minFreeHeap, const String& restartReason) {
  _lock();
  _state.system.free_heap = freeHeap;
  _state.system.total_heap = totalHeap;
  _state.system.min_free_heap = minFreeHeap;
  _state.system.restart_reason = restartReason;
  _unlock();
}

void PlayerStateManager::updateStorage(bool sdInserted, uint64_t totalBytes, uint64_t usedBytes, uint64_t freeBytes) {
  bool changed = false;
  _lock();
  if (_state.storage.sd_inserted != sdInserted) changed = true;
  _state.storage.sd_inserted = sdInserted;
  _state.storage.total_bytes = totalBytes;
  _state.storage.used_bytes = usedBytes;
  _state.storage.free_bytes = freeBytes;
  _unlock();

  if (changed) {
    JsonDocument doc;
    doc["sd_inserted"] = sdInserted;
    doc["total_bytes"] = totalBytes;
    String payload;
    serializeJson(doc, payload);
    EventManager::getInstance().dispatchEvent(EventType::SD_CARD_CHANGED, "sd_card_changed", payload);
  }
}

void PlayerStateManager::setBleStatus(bool active, bool connected) {
  _lock();
  _state.system.ble_active = active;
  _state.system.ble_connected = connected;
  _unlock();

  JsonDocument doc;
  doc["active"] = active;
  doc["connected"] = connected;
  String payload;
  serializeJson(doc, payload);
  EventManager::getInstance().dispatchEvent(EventType::BLUETOOTH_CHANGED, "bluetooth_changed", payload);
}

void PlayerStateManager::incrementWifiReconnectCount() {
  _lock();
  _state.system.wifi_reconnect_count++;
  _unlock();
}

String PlayerStateManager::toJson() {
  _lock();
  JsonDocument doc;

  // Device Info - matches Flutter DeviceInfo.fromJson
  doc["device"]["name"] = _state.device.name;
  doc["device"]["firmware_version"] = _state.device.firmware_version;
  doc["device"]["uptime_seconds"] = _state.device.uptime_seconds;

  // Connection Info - matches Flutter ConnectionInfo.fromJson
  doc["connection"]["router_connected"] = _state.connection.router_connected;
  doc["connection"]["router_ip"] = _state.connection.router_ip;
  doc["connection"]["router_ssid"] = _state.connection.router_ssid;
  doc["connection"]["router_rssi"] = _state.connection.router_rssi;
  doc["connection"]["hotspot_active"] = _state.connection.hotspot_active;
  doc["connection"]["hotspot_clients"] = _state.connection.hotspot_clients;
  doc["connection"]["active_interface"] = _state.connection.active_interface;

  // Audio Info - matches Flutter AudioInfo.fromJson
  doc["audio"]["source"] = _state.audio.source;
  doc["audio"]["status"] = _state.audio.status;
  doc["audio"]["volume"] = _state.audio.volume;
  doc["audio"]["muted"] = _state.audio.muted;
  doc["audio"]["codec"] = _state.audio.codec;
  doc["audio"]["sample_rate"] = _state.audio.sample_rate;
  doc["audio"]["bit_depth"] = _state.audio.bit_depth;
  doc["audio"]["channels"] = _state.audio.channels;

  // Storage Info - matches Flutter StorageInfo.fromJson
  doc["storage"]["sd_inserted"] = _state.storage.sd_inserted;
  doc["storage"]["total_bytes"] = _state.storage.total_bytes;
  doc["storage"]["used_bytes"] = _state.storage.used_bytes;
  doc["storage"]["free_bytes"] = _state.storage.free_bytes;

  // System Info - matches Flutter SystemInfo.fromJson
  doc["system"]["free_heap"] = _state.system.free_heap;
  doc["system"]["total_heap"] = _state.system.total_heap;
  doc["system"]["min_free_heap"] = _state.system.min_free_heap;
  doc["system"]["restart_reason"] = _state.system.restart_reason;

  String json;
  serializeJson(doc, json);
  _unlock();
  return json;
}

String PlayerStateManager::getDeviceApiJson() {
  _lock();
  JsonDocument doc;
  doc["name"] = _state.device.name;
  doc["device_id"] = _state.device_id;
  doc["firmware_version"] = _state.device.firmware_version;
  doc["hardware"] = HARDWARE_IDENTIFIER;
  doc["uptime_seconds"] = _state.device.uptime_seconds;

  JsonArray caps = doc["capabilities"].to<JsonArray>();
  caps.add("wifi_ap_sta");
  caps.add("ble");
  caps.add("websocket");
  caps.add("audio_control");
  if (_state.storage.sd_inserted) {
    caps.add("sd");
  }

  String json;
  serializeJson(doc, json);
  _unlock();
  return json;
}

String PlayerStateManager::getNetworkApiJson() {
  _lock();
  JsonDocument doc;

  doc["ap"]["active"] = _state.connection.hotspot_active;
  doc["ap"]["ssid"] = AP_SSID_DEFAULT;
  doc["ap"]["ip"] = AP_IP_ADDRESS.toString();
  doc["ap"]["clients"] = _state.connection.hotspot_clients;

  doc["router"]["connected"] = _state.connection.router_connected;
  doc["router"]["ssid"] = _state.connection.router_ssid;
  doc["router"]["ip"] = _state.connection.router_ip;
  doc["router"]["rssi"] = _state.connection.router_rssi;

  doc["ble"]["active"] = _state.system.ble_active;
  doc["ble"]["connected"] = _state.system.ble_connected;

  doc["active_interface"] = _state.connection.active_interface;

  String json;
  serializeJson(doc, json);
  _unlock();
  return json;
}

String PlayerStateManager::getSystemApiJson() {
  _lock();
  JsonDocument doc;
  doc["free_heap"] = _state.system.free_heap;
  doc["total_heap"] = _state.system.total_heap;
  doc["min_free_heap"] = _state.system.min_free_heap;
  doc["uptime_seconds"] = _state.device.uptime_seconds;
  doc["restart_reason"] = _state.system.restart_reason;
  doc["wifi_reconnect_count"] = _state.system.wifi_reconnect_count;
  doc["firmware_version"] = _state.device.firmware_version;
  doc["hardware"] = HARDWARE_IDENTIFIER;

  String json;
  serializeJson(doc, json);
  _unlock();
  return json;
}

void PlayerStateManager::_lock() {
  xSemaphoreTake(_mutex, portMAX_DELAY);
}

void PlayerStateManager::_unlock() {
  xSemaphoreGive(_mutex);
}

String PlayerStateManager::_generateDeviceId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char id[32];
  snprintf(id, sizeof(id), "croenix-%02x%02x%02x", mac[3], mac[4], mac[5]);
  return String(id);
}

