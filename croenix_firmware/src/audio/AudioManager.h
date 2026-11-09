#ifndef CROENIX_AUDIO_MANAGER_H
#define CROENIX_AUDIO_MANAGER_H

#include <Arduino.h>
#include "I2SManager.h"
#include "SdAudioSource.h"
#include "BluetoothAudioSource.h"
#include "FmAudioSource.h"
#include "WebRadioSource.h"

class PlayerStateManager;

class AudioManager {
public:
  AudioManager(PlayerStateManager* stateManager);
  void begin();
  void update();

  bool setVolume(int volume);
  bool setMute(bool muted);
  bool setSource(const String& sourceName);
  bool play();
  bool pause();
  void stop();

  String getCurrentSource();
  bool isPlaying();

private:
  PlayerStateManager* _stateManager;
  I2SManager _i2s;
  SdAudioSource _sdSource;
  BluetoothAudioSource _btSource;
  FmAudioSource _fmSource;
  WebRadioSource _webSource;

  AudioSource* _activeSource;
};

#endif // CROENIX_AUDIO_MANAGER_H

