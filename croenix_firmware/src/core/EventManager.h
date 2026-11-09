#ifndef CROENIX_EVENT_MANAGER_H
#define CROENIX_EVENT_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <functional>

enum class EventType {
  STATE_SNAPSHOT,
  VOLUME_CHANGED,
  MUTE_CHANGED,
  SOURCE_CHANGED,
  PLAYBACK_CHANGED,
  TRACK_CHANGED,
  POSITION_CHANGED,
  WIFI_CHANGED,
  BLUETOOTH_CHANGED,
  SD_CARD_CHANGED,
  FM_FREQUENCY_CHANGED,
  SYSTEM_WARNING,
  SYSTEM_ERROR
};

struct CroenixEvent {
  EventType type;
  String name;
  String payloadJson;
  unsigned long timestamp;
  uint32_t sequenceNumber;
};

typedef std::function<void(const CroenixEvent&)> EventListener;

class EventManager {
public:
  static EventManager& getInstance();

  void addListener(EventListener listener);
  void dispatchEvent(EventType type, const String& name, const String& payloadJson);
  const char* eventTypeToString(EventType type);

private:
  EventManager();
  std::vector<EventListener> _listeners;
  uint32_t _nextSequenceNumber;
  SemaphoreHandle_t _mutex;
};

#endif // CROENIX_EVENT_MANAGER_H

