#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Represents the complete device state
struct DeviceState {
  // Device Info
  struct {
    String name;
    String firmware_version;
    unsigned long uptime_seconds;
  } device;

  // Connection Status
  struct {
    bool router_connected;
    String router_ip;
    String router_ssid;
    int router_rssi;
    bool hotspot_active;
    int hotspot_clients;
    String active_interface;  // "router" or "hotspot"
  } connection;

  // Audio State (placeholders for Phase 4+)
  struct {
    String source;      // "none", "bluetooth", "sd", "fm", "webradio", "online"
    String status;      // "stopped", "playing", "paused"
    int volume;         // 0-100
    bool muted;
    String codec;       // "FLAC", "WAV", etc.
    int sample_rate;    // Hz
    int bit_depth;      // bits
    int channels;       // 2 for stereo
  } audio;

  // Storage (placeholder)
  struct {
    bool sd_inserted;
    unsigned long total_bytes;
    unsigned long used_bytes;
    unsigned long free_bytes;
  } storage;

  // System Diagnostics
  struct {
    unsigned long free_heap;
    unsigned long total_heap;
    unsigned long min_free_heap;
    String restart_reason;
  } system;
};

class StateManager {
public:
  StateManager();
  void begin();
  void update();

  // State access
  DeviceState getState();
  void setState(const DeviceState& newState);

  // Partial updates
  void updateDeviceInfo(const String& name, unsigned long uptime);
  void updateConnection(bool routerConnected, const String& routerIP,
                       const String& routerSSID, int routerRSSI,
                       bool hotspotActive, int hotspotClients);
  void updateAudio(const String& source, const String& status,
                  int volume, bool muted);
  void updateSystem(unsigned long freeHeap, unsigned long totalHeap,
                   unsigned long minFreeHeap, const String& restartReason);

  // JSON serialization
  String toJson();
  void fromJson(const String& json);

private:
  DeviceState _state;
  SemaphoreHandle_t _mutex;

  void _lock();
  void _unlock();
};

#endif // STATE_MANAGER_H
