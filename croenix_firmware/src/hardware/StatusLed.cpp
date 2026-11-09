#include "StatusLed.h"
#include "../connectivity/CroenixWiFiManager.h"
#include "../diagnostics/SystemMonitor.h"
#include "../PinConfig.h"
#include "../Config.h"
#include <WiFi.h>

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
  // Turn LED on briefly during boot
  digitalWrite(PIN_STATUS_LED, STATUS_LED_ACTIVE_HIGH ? HIGH : LOW);
  _initialized = true;
  _lastBlinkCycleStart = millis();

  CROENIX_LOG("StatusLed", "Status LED initialized on GPIO %d (Normal: blink every 5s, Issue: full-time solid ON)", PIN_STATUS_LED);
}

void StatusLed::setManualError(bool hasError) {
  _manualError = hasError;
}

bool StatusLed::_isSystemNormal() {
  if (_manualError) return false;

  // Allow 5-second initial grace period after power-on while AP and radio start up
  if (millis() < 5000) {
    return true;
  }

  // Check AP hotspot: must be active with valid IP for app connection
  if (_wifiManager) {
    if (!_wifiManager->isAPActive() && WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
      return false;
    }
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
    // Normal operation: blink power LED every 5 seconds (5000 ms period: 300 ms flash, then off)
    unsigned long elapsed = now - _lastBlinkCycleStart;
    if (elapsed >= 5000) {
      _lastBlinkCycleStart = now;
      elapsed = 0;
    }

    bool shouldBeOn = (elapsed < 300);
    if (shouldBeOn != _ledState) {
      _ledState = shouldBeOn;
      digitalWrite(PIN_STATUS_LED, _ledState ? (STATUS_LED_ACTIVE_HIGH ? HIGH : LOW)
                                             : (STATUS_LED_ACTIVE_HIGH ? LOW : HIGH));
    }
  } else {
    // If any issues: DO NOT BLINK. LED is FULL-TIME SOLID ON.
    if (!_ledState) {
      _ledState = true;
      digitalWrite(PIN_STATUS_LED, STATUS_LED_ACTIVE_HIGH ? HIGH : LOW);
      CROENIX_LOG("StatusLed", "Issue detected: Status LED held full-time solid ON.");
    }
  }
}
