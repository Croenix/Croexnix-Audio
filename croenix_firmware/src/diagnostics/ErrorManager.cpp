#include "ErrorManager.h"
#include "../core/EventManager.h"
#include "../Config.h"
#include <ArduinoJson.h>

ErrorManager& ErrorManager::getInstance() {
  static ErrorManager instance;
  return instance;
}

ErrorManager::ErrorManager() : _errorCount(0) {
  _mutex = xSemaphoreCreateMutex();
}

void ErrorManager::reportError(uint32_t code, const String& subsystem, const String& message) {
  ErrorRecord rec;
  rec.code = code;
  rec.subsystem = subsystem;
  rec.message = message;
  rec.timestamp = millis();

  if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    _errorCount++;
    if (_history.size() >= 10) {
      _history.erase(_history.begin());
    }
    _history.push_back(rec);
    xSemaphoreGive(_mutex);
  }

  CROENIX_LOG("ErrorManager", "[ERROR %u] in %s: %s", code, subsystem.c_str(), message.c_str());

  JsonDocument doc;
  doc["code"] = code;
  doc["subsystem"] = subsystem;
  doc["message"] = message;
  String payload;
  serializeJson(doc, payload);
  EventManager::getInstance().dispatchEvent(EventType::SYSTEM_ERROR, "system_error", payload);
}

void ErrorManager::reportWarning(const String& subsystem, const String& message) {
  CROENIX_LOG("ErrorManager", "[WARN] in %s: %s", subsystem.c_str(), message.c_str());

  JsonDocument doc;
  doc["subsystem"] = subsystem;
  doc["message"] = message;
  String payload;
  serializeJson(doc, payload);
  EventManager::getInstance().dispatchEvent(EventType::SYSTEM_WARNING, "system_warning", payload);
}

uint32_t ErrorManager::getErrorCount() {
  uint32_t count = 0;
  if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    count = _errorCount;
    xSemaphoreGive(_mutex);
  }
  return count;
}

String ErrorManager::getLastErrorJson() {
  JsonDocument doc;
  if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (!_history.empty()) {
      const ErrorRecord& rec = _history.back();
      doc["has_error"] = true;
      doc["code"] = rec.code;
      doc["subsystem"] = rec.subsystem;
      doc["message"] = rec.message;
      doc["timestamp"] = rec.timestamp;
    } else {
      doc["has_error"] = false;
    }
    xSemaphoreGive(_mutex);
  }
  String json;
  serializeJson(doc, json);
  return json;
}

