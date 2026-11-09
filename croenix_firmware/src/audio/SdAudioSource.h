#ifndef CROENIX_SD_AUDIO_SOURCE_H
#define CROENIX_SD_AUDIO_SOURCE_H

#include "AudioSource.h"

class PlayerStateManager;

class SdAudioSource : public AudioSource {
public:
  SdAudioSource(PlayerStateManager* stateManager);
  bool begin() override;
  void stop() override;
  bool play() override;
  bool pause() override;
  bool isAvailable() override;
  SourceType getType() override { return SourceType::SD_CARD; }
  const char* getName() override { return "sd"; }
  void update() override;

private:
  PlayerStateManager* _stateManager;
  bool _isPlaying;
};

#endif // CROENIX_SD_AUDIO_SOURCE_H

