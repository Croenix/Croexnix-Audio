#ifndef CROENIX_STATUS_LED_H
#define CROENIX_STATUS_LED_H

#include <Arduino.h>

class CroenixWiFiManager;
class SystemMonitor;

class StatusLed {
public:
  StatusLed(CroenixWiFiManager* wifiManager, SystemMonitor* systemMonitor);
  void begin();
  void update();

  void setManualError(bool hasError);

private:
  CroenixWiFiManager* _wifiManager;
  SystemMonitor* _systemMonitor;

  bool _initialized;
  bool _manualError;
  unsigned long _lastBlinkCycleStart;
  bool _ledState;

  bool _isSystemNormal();
};

#endif // CROENIX_STATUS_LED_H

