#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <Arduino.h>
#include <esp_system.h>

struct SystemInfo {
  unsigned long free_heap;
  unsigned long total_heap;
  unsigned long min_free_heap;
  String restart_reason;
  unsigned long uptime_seconds;
};

class SystemMonitor {
public:
  SystemMonitor();
  void begin();
  void update();

  SystemInfo getInfo();

private:
  unsigned long _lastUpdate;
  SystemInfo _info;

  String _getRestartReason();
};

#endif // SYSTEM_MONITOR_H
