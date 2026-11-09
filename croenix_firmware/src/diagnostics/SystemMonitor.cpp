#include "SystemMonitor.h"
#include "../core/PlayerStateManager.h"
#include "../Config.h"
#include <esp_system.h>
#include <esp_heap_caps.h>

SystemMonitor::SystemMonitor(PlayerStateManager* stateManager)
  : _stateManager(stateManager), _lastUpdateMs(0) {
}

void SystemMonitor::begin() {
  _restartReason = _determineRestartReason();
  CROENIX_LOG("SystemMonitor", "Device started. Reset reason: %s", _restartReason.c_str());
  update();
}

void SystemMonitor::update() {
  unsigned long now = millis();
  if (now - _lastUpdateMs < SYSTEM_MONITOR_INTERVAL_MS && _lastUpdateMs != 0) {
    return;
  }
  _lastUpdateMs = now;

  unsigned long freeHeap = esp_get_free_heap_size();
  unsigned long totalHeap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
  unsigned long minFreeHeap = esp_get_minimum_free_heap_size();

  if (_stateManager) {
    _stateManager->updateSystemDiagnostics(freeHeap, totalHeap, minFreeHeap, _restartReason);
  }

  if (freeHeap < HEAP_WARNING_THRESHOLD_BYTES) {
    CROENIX_LOG("SystemMonitor", "WARNING: Free heap critically low: %lu bytes (min: %lu)", freeHeap, minFreeHeap);
  }
}

unsigned long SystemMonitor::getFreeHeap() {
  return esp_get_free_heap_size();
}

unsigned long SystemMonitor::getTotalHeap() {
  return heap_caps_get_total_size(MALLOC_CAP_8BIT);
}

unsigned long SystemMonitor::getMinFreeHeap() {
  return esp_get_minimum_free_heap_size();
}

unsigned long SystemMonitor::getUptimeSeconds() {
  return millis() / 1000;
}

String SystemMonitor::getRestartReason() {
  return _restartReason;
}

String SystemMonitor::_determineRestartReason() {
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_POWERON:   return "power_on";
    case ESP_RST_EXT:       return "external_reset";
    case ESP_RST_SW:        return "software_reset";
    case ESP_RST_PANIC:     return "panic";
    case ESP_RST_INT_WDT:   return "interrupt_wdt";
    case ESP_RST_TASK_WDT:  return "task_wdt";
    case ESP_RST_WDT:       return "other_wdt";
    case ESP_RST_DEEPSLEEP: return "deep_sleep";
    case ESP_RST_BROWNOUT:  return "brownout";
    case ESP_RST_SDIO:      return "sdio";
    default:                return "unknown";
  }
}

