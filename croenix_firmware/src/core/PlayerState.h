#ifndef CROENIX_PLAYER_STATE_H
#define CROENIX_PLAYER_STATE_H

#include <Arduino.h>

struct DeviceInfoData {
  String name;
  String firmware_version;
  unsigned long uptime_seconds;
};

struct ConnectionInfoData {
  bool router_connected;
  String router_ip;
  String router_ssid;
  int router_rssi;
  bool hotspot_active;
  int hotspot_clients;
  String active_interface; // "router", "hotspot", or "none"
};

struct AudioInfoData {
  String source;           // "none", "sd", "bluetooth", "fm", "webradio", "online"
  String status;           // "stopped", "playing", "paused"
  int volume;              // 0-100
  bool muted;              // true/false
  String codec;            // e.g. "FLAC", "WAV", "MP3"
  int sample_rate;         // Hz, e.g. 44100
  int bit_depth;           // bits, e.g. 16, 24
  int channels;            // 2 for stereo
  String current_track;    // Title if available
  String current_artist;   // Artist if available
  unsigned long duration_ms;
  unsigned long position_ms;
};

struct StorageInfoData {
  bool sd_inserted;
  uint64_t total_bytes;
  uint64_t used_bytes;
  uint64_t free_bytes;
};

struct SystemInfoData {
  unsigned long free_heap;
  unsigned long total_heap;
  unsigned long min_free_heap;
  String restart_reason;
  int wifi_reconnect_count;
  bool ble_active;
  bool ble_connected;
};

struct PlayerState {
  String device_id;
  DeviceInfoData device;
  ConnectionInfoData connection;
  AudioInfoData audio;
  StorageInfoData storage;
  SystemInfoData system;
};

#endif // CROENIX_PLAYER_STATE_H

