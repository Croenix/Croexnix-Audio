#include "SystemMonitor.h"
#include "Config.h"

SystemMonitor::SystemMonitor() : _lastUpdate(0) {
}

void SystemMonitor::begin() {
  _info.restart_reason = _getRestartReason();
  DEBUG_PRINT("[SystemMonitor] Restart reason: ");
  DEBUG_PRINTLN(_info.restart_reason);

  update();
}

void SystemMonitor::update() {
  unsigned long now = millis();

  if (now - _lastUpdate < SYSTEM_MONITOR_INTERVAL_MS) {
    return;
  }

  _lastUpdate = now;

  // Update memory info
  _info.free_heap = esp_get_free_heap_size();
  _info.total_heap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
  _info.min_free_heap = esp_get_minimum_free_heap_size();
  _info.uptime_seconds = millis() / 1000;

  // Warn if heap is low
  if (_info.free_heap < HEAP_WARNING_THRESHOLD) {
    DEBUG_PRINT("[SystemMonitor] WARNING: Low heap! Free: ");
    DEBUG_PRINT(_info.free_heap);
    DEBUG_PRINTLN(" bytes");
  }
}

SystemInfo SystemMonitor::getInfo() {
  return _info;
}

String SystemMonitor::_getRestartReason() {
  esp_reset_reason_t reason = esp_reset_reason();

  switch (reason) {
    case ESP_RST_UNKNOWN:
      return "Unknown";
    case ESP_RST_POWERON:
      return "Power-on";
    case ESP_RST_EXT:
      return "External reset";
    case ESP_RST_SW:
      return "Software reset";
    case ESP_RST_PANIC:
      return "Panic/Exception";
    case ESP_RST_INT_WDT:
      return "Interrupt watchdog";
    case ESP_RST_TASK_WDT:
      return "Task watchdog";
    case ESP_RST_WDT:
      return "Watchdog";
    case ESP_RST_DEEPSLEEP:
      return "Deep sleep";
    case ESP_RST_BROWNOUT:
      return "Brownout";
    case ESP_RST_SDIO:
      return "SDIO";
    default:
      return "Other";
  }
}
