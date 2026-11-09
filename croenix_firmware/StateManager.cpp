#include "StateManager.h"
#include "Config.h"

StateManager::StateManager() {
  _mutex = xSemaphoreCreateMutex();
}

void StateManager::begin() {
  _lock();

  // Initialize device info
  _state.device.name = DEVICE_NAME;
  _state.device.firmware_version = FIRMWARE_VERSION;
  _state.device.uptime_seconds = 0;

  // Initialize connection
  _state.connection.router_connected = false;
  _state.connection.router_ip = "";
  _state.connection.router_ssid = "";
  _state.connection.router_rssi = 0;
  _state.connection.hotspot_active = false;
  _state.connection.hotspot_clients = 0;
  _state.connection.active_interface = "none";

  // Initialize audio (placeholders)
  _state.audio.source = "none";
  _state.audio.status = "stopped";
  _state.audio.volume = 50;
  _state.audio.muted = false;
  _state.audio.codec = "";
  _state.audio.sample_rate = 0;
  _state.audio.bit_depth = 0;
  _state.audio.channels = 0;

  // Initialize storage
  _state.storage.sd_inserted = false;
  _state.storage.total_bytes = 0;
  _state.storage.used_bytes = 0;
  _state.storage.free_bytes = 0;

  // Initialize system
  _state.system.free_heap = 0;
  _state.system.total_heap = 0;
  _state.system.min_free_heap = 0;
  _state.system.restart_reason = "";

  _unlock();

  DEBUG_PRINTLN("[StateManager] Initialized");
}

void StateManager::update() {
  // Update uptime
  _lock();
  _state.device.uptime_seconds = millis() / 1000;
  _unlock();
}

DeviceState StateManager::getState() {
  _lock();
  DeviceState copy = _state;
  _unlock();
  return copy;
}

void StateManager::setState(const DeviceState& newState) {
  _lock();
  _state = newState;
  _unlock();
}

void StateManager::updateDeviceInfo(const String& name, unsigned long uptime) {
  _lock();
  _state.device.name = name;
  _state.device.uptime_seconds = uptime;
  _unlock();
}

void StateManager::updateConnection(bool routerConnected, const String& routerIP,
                                   const String& routerSSID, int routerRSSI,
                                   bool hotspotActive, int hotspotClients) {
  _lock();
  _state.connection.router_connected = routerConnected;
  _state.connection.router_ip = routerIP;
  _state.connection.router_ssid = routerSSID;
  _state.connection.router_rssi = routerRSSI;
  _state.connection.hotspot_active = hotspotActive;
  _state.connection.hotspot_clients = hotspotClients;

  // Determine active interface
  if (routerConnected) {
    _state.connection.active_interface = "router";
  } else if (hotspotActive) {
    _state.connection.active_interface = "hotspot";
  } else {
    _state.connection.active_interface = "none";
  }
  _unlock();
}

void StateManager::updateAudio(const String& source, const String& status,
                              int volume, bool muted) {
  _lock();
  _state.audio.source = source;
  _state.audio.status = status;
  _state.audio.volume = volume;
  _state.audio.muted = muted;
  _unlock();
}

void StateManager::updateSystem(unsigned long freeHeap, unsigned long totalHeap,
                               unsigned long minFreeHeap, const String& restartReason) {
  _lock();
  _state.system.free_heap = freeHeap;
  _state.system.total_heap = totalHeap;
  _state.system.min_free_heap = minFreeHeap;
  _state.system.restart_reason = restartReason;
  _unlock();
}

String StateManager::toJson() {
  _lock();

  JsonDocument doc;

  // Device
  doc["device"]["name"] = _state.device.name;
  doc["device"]["firmware_version"] = _state.device.firmware_version;
  doc["device"]["uptime_seconds"] = _state.device.uptime_seconds;

  // Connection
  doc["connection"]["router_connected"] = _state.connection.router_connected;
  doc["connection"]["router_ip"] = _state.connection.router_ip;
  doc["connection"]["router_ssid"] = _state.connection.router_ssid;
  doc["connection"]["router_rssi"] = _state.connection.router_rssi;
  doc["connection"]["hotspot_active"] = _state.connection.hotspot_active;
  doc["connection"]["hotspot_clients"] = _state.connection.hotspot_clients;
  doc["connection"]["active_interface"] = _state.connection.active_interface;

  // Audio
  doc["audio"]["source"] = _state.audio.source;
  doc["audio"]["status"] = _state.audio.status;
  doc["audio"]["volume"] = _state.audio.volume;
  doc["audio"]["muted"] = _state.audio.muted;
  doc["audio"]["codec"] = _state.audio.codec;
  doc["audio"]["sample_rate"] = _state.audio.sample_rate;
  doc["audio"]["bit_depth"] = _state.audio.bit_depth;
  doc["audio"]["channels"] = _state.audio.channels;

  // Storage
  doc["storage"]["sd_inserted"] = _state.storage.sd_inserted;
  doc["storage"]["total_bytes"] = _state.storage.total_bytes;
  doc["storage"]["used_bytes"] = _state.storage.used_bytes;
  doc["storage"]["free_bytes"] = _state.storage.free_bytes;

  // System
  doc["system"]["free_heap"] = _state.system.free_heap;
  doc["system"]["total_heap"] = _state.system.total_heap;
  doc["system"]["min_free_heap"] = _state.system.min_free_heap;
  doc["system"]["restart_reason"] = _state.system.restart_reason;

  String json;
  serializeJson(doc, json);

  _unlock();
  return json;
}

void StateManager::fromJson(const String& json) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    DEBUG_PRINT("[StateManager] JSON parse error: ");
    DEBUG_PRINTLN(error.c_str());
    return;
  }

  _lock();

  // Parse device
  if (doc.containsKey("device")) {
    _state.device.name = doc["device"]["name"].as<String>();
  }

  // Parse audio (for remote updates)
  if (doc.containsKey("audio")) {
    if (doc["audio"].containsKey("volume")) {
      _state.audio.volume = doc["audio"]["volume"];
    }
    if (doc["audio"].containsKey("muted")) {
      _state.audio.muted = doc["audio"]["muted"];
    }
  }

  _unlock();
}

void StateManager::_lock() {
  xSemaphoreTake(_mutex, portMAX_DELAY);
}

void StateManager::_unlock() {
  xSemaphoreGive(_mutex);
}
