#ifndef CROENIX_BT_AUDIO_SOURCE_H
#define CROENIX_BT_AUDIO_SOURCE_H

#include "AudioSource.h"

class PlayerStateManager;

class BluetoothAudioSource : public AudioSource {
public:
  BluetoothAudioSource(PlayerStateManager* stateManager);
  bool begin() override;
  void stop() override;
  bool play() override;
  bool pause() override;
  bool isAvailable() override;
  SourceType getType() override { return SourceType::BLUETOOTH_A2DP; }
  const char* getName() override { return "bluetooth"; }

private:
  PlayerStateManager* _stateManager;
  bool _connected;
};

#endif // CROENIX_BT_AUDIO_SOURCE_H

