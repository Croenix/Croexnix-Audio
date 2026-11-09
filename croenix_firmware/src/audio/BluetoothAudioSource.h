#ifndef CROENIX_BT_AUDIO_SOURCE_H
#define CROENIX_BT_AUDIO_SOURCE_H

#include "AudioSource.h"
#include <esp_a2dp_api.h>

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

  void handleAudioData(const uint8_t *data, uint32_t len);
  void handleConnectionEvent(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

private:
  PlayerStateManager* _stateManager;
  bool _connected;
  bool _isStreaming;
};

#endif // CROENIX_BT_AUDIO_SOURCE_H
