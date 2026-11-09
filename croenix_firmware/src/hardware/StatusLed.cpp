#include "StatusLed.h"
#include "../connectivity/CroenixWiFiManager.h"
#include "../diagnostics/SystemMonitor.h"
#include "../PinConfig.h"
#include "../Config.h"

StatusLed::StatusLed(CroenixWiFiManager* wifiManager, SystemMonitor* systemMonitor)
  : _wifiManager(wifiManager),
    _systemMonitor(systemMonitor),
    _initialized(false),
    _manualError(false),
    _lastBlinkCycleStart(0),
    _ledState(false) {
}

void StatusLed::begin() {
  if (PIN_STATUS_LED < 0) {
    CROENIX_LOG("StatusLed", "Status LED pin disabled in PinConfig.h");
    return;
  }

  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, STATUS_LED_ACTIVE_HIGH ? LOW : HIGH);
  _initialized = true;
  _lastBlinkCycleStart = millis();

  CROENIX_LOG("StatusLed", "Status LED initialized on GPIO %d (Normal: 5s heartbeat, Error: 1s cadence)", PIN_STATUS_LED);
}

void StatusLed::setManualError(bool hasError) {
  _manualError = hasError;
}

bool StatusLed::_isSystemNormal() {
  if (_manualError) return false;

  // Check AP hotspot: must be active for app connection
  if (_wifiManager && !_wifiManager->isAPActive()) {
    return false;
  }

  // Check heap memory
  if (_systemMonitor && _systemMonitor->getFreeHeap() < HEAP_WARNING_THRESHOLD_BYTES) {
    return false;
  }

  return true;
}

void StatusLed::update() {
  if (!_initialized) return;

  unsigned long now = millis();
  bool normal = _isSystemNormal();

  if (normal) {
    // Normal operation: blink once every 5 seconds (5000 ms period, 150 ms pulse)
    unsigned long elapsed = now - _lastBlinkCycleStart;
    if (elapsed >= 5000) {
      _lastBlinkCycleStart = now;
      elapsed = 0;
    }

    bool shouldBeOn = (elapsed < 150);
    if (shouldBeOn != _ledState) {
      _ledState = shouldBeOn;
      digitalWrite(PIN_STATUS_LED, _ledState ? (STATUS_LED_ACTIVE_HIGH ? HIGH : LOW)
                                             : (STATUS_LED_ACTIVE_HIGH ? LOW : HIGH));
    }
  } else {
    // Abnormal / Error operation: blink every 1 second (1000 ms period, 500 ms on / 500 ms off)
    unsigned long elapsed = now - _lastBlinkCycleStart;
    if (elapsed >= 1000) {
      _lastBlinkCycleStart = now;
      elapsed = 0;
    }

    bool shouldBeOn = (elapsed < 500);
    if (shouldBeOn != _ledState) {
      _ledState = shouldBeOn;
      digitalWrite(PIN_STATUS_LED, _ledState ? (STATUS_LED_ACTIVE_HIGH ? HIGH : LOW)
                                             : (STATUS_LED_ACTIVE_HIGH ? LOW : HIGH));
    }
  }
}

