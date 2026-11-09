#ifndef CROENIX_FM_AUDIO_SOURCE_H
#define CROENIX_FM_AUDIO_SOURCE_H

#include "AudioSource.h"

class PlayerStateManager;

class FmAudioSource : public AudioSource {
public:
  FmAudioSource(PlayerStateManager* stateManager);
  bool begin() override;
  void stop() override;
  bool play() override;
  bool pause() override;
  bool isAvailable() override;
  SourceType getType() override { return SourceType::FM_TUNER; }
  const char* getName() override { return "fm"; }

  void setFrequency(float mhz);
  float getFrequency();

private:
  PlayerStateManager* _stateManager;
  float _frequency;
  bool _hardwareDetected;
};

#endif // CROENIX_FM_AUDIO_SOURCE_H

