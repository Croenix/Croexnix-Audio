#include "EventManager.h"

EventManager& EventManager::getInstance() {
  static EventManager instance;
  return instance;
}

EventManager::EventManager() : _nextSequenceNumber(1) {
  _mutex = xSemaphoreCreateMutex();
}

void EventManager::addListener(EventListener listener) {
  if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    _listeners.push_back(listener);
    xSemaphoreGive(_mutex);
  }
}

const char* EventManager::eventTypeToString(EventType type) {
  switch (type) {
    case EventType::STATE_SNAPSHOT:       return "state_snapshot";
    case EventType::VOLUME_CHANGED:       return "volume_changed";
    case EventType::MUTE_CHANGED:         return "mute_changed";
    case EventType::SOURCE_CHANGED:       return "source_changed";
    case EventType::PLAYBACK_CHANGED:     return "playback_changed";
    case EventType::TRACK_CHANGED:        return "track_changed";
    case EventType::POSITION_CHANGED:     return "position_changed";
    case EventType::WIFI_CHANGED:         return "wifi_changed";
    case EventType::BLUETOOTH_CHANGED:    return "bluetooth_changed";
    case EventType::SD_CARD_CHANGED:      return "sd_card_changed";
    case EventType::FM_FREQUENCY_CHANGED: return "fm_frequency_changed";
    case EventType::SYSTEM_WARNING:       return "system_warning";
    case EventType::SYSTEM_ERROR:         return "system_error";
    default:                              return "unknown";
  }
}

void EventManager::dispatchEvent(EventType type, const String& name, const String& payloadJson) {
  CroenixEvent ev;
  ev.type = type;
  ev.name = name.length() > 0 ? name : String(eventTypeToString(type));
  ev.payloadJson = payloadJson;
  ev.timestamp = millis();

  std::vector<EventListener> listenersCopy;

  if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    ev.sequenceNumber = _nextSequenceNumber++;
    listenersCopy = _listeners;
    xSemaphoreGive(_mutex);
  } else {
    ev.sequenceNumber = 0;
  }

  for (auto& listener : listenersCopy) {
    if (listener) {
      listener(ev);
    }
  }
}

