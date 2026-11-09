#ifndef CROENIX_AUDIO_SOURCE_H
#define CROENIX_AUDIO_SOURCE_H

#include <Arduino.h>

enum class SourceType {
  NONE,
  SD_CARD,
  BLUETOOTH_A2DP,
  FM_TUNER,
  WEB_RADIO,
  ONLINE_APP
};

class AudioSource {
public:
  virtual ~AudioSource() {}

  virtual bool begin() = 0;
  virtual void stop() = 0;
  virtual bool play() = 0;
  virtual bool pause() = 0;
  virtual bool isAvailable() = 0;
  virtual SourceType getType() = 0;
  virtual const char* getName() = 0;
  virtual void update() {}
};

#endif // CROENIX_AUDIO_SOURCE_H

