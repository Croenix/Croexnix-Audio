#ifndef CROENIX_SYSTEM_MONITOR_H
#define CROENIX_SYSTEM_MONITOR_H

#include <Arduino.h>

class PlayerStateManager;

class SystemMonitor {
public:
  SystemMonitor(PlayerStateManager* stateManager);
  void begin();
  void update();

  unsigned long getFreeHeap();
  unsigned long getTotalHeap();
  unsigned long getMinFreeHeap();
  unsigned long getUptimeSeconds();
  String getRestartReason();

private:
  PlayerStateManager* _stateManager;
  unsigned long _lastUpdateMs;
  String _restartReason;

  String _determineRestartReason();
};

#endif // CROENIX_SYSTEM_MONITOR_H

