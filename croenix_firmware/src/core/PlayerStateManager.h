#ifndef CROENIX_PLAYER_STATE_MANAGER_H
#define CROENIX_PLAYER_STATE_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "PlayerState.h"

class PlayerStateManager {
public:
  PlayerStateManager();
  void begin();
  void update();

  PlayerState getState();
  String getDeviceId();

  // Volume & Audio control
  bool setVolume(int volume, bool publish = true);
  bool setMute(bool muted, bool publish = true);
  bool setSource(const String& source, bool publish = true);
  bool setPlaybackStatus(const String& status, bool publish = true);
  void setTrackMetadata(const String& title, const String& artist, unsigned long durationMs);
  void setPlaybackPosition(unsigned long positionMs);

  // Network & System state updates
  void updateConnection(bool routerConnected, const String& routerIp,
                        const String& routerSsid, int routerRssi,
                        bool hotspotActive, int hotspotClients);
  void updateSystemDiagnostics(unsigned long freeHeap, unsigned long totalHeap,
                               unsigned long minFreeHeap, const String& restartReason);
  void updateStorage(bool sdInserted, uint64_t totalBytes, uint64_t usedBytes, uint64_t freeBytes);
  void setBleStatus(bool active, bool connected);
  void incrementWifiReconnectCount();

  // Serialization
  String toJson();
  String getDeviceApiJson();
  String getNetworkApiJson();
  String getSystemApiJson();

private:
  PlayerState _state;
  SemaphoreHandle_t _mutex;

  void _lock();
  void _unlock();
  String _generateDeviceId();
};

#endif // CROENIX_PLAYER_STATE_MANAGER_H

