#ifndef CROENIX_WEB_RADIO_SOURCE_H
#define CROENIX_WEB_RADIO_SOURCE_H

#include "AudioSource.h"

class PlayerStateManager;

class WebRadioSource : public AudioSource {
public:
  WebRadioSource(PlayerStateManager* stateManager);
  bool begin() override;
  void stop() override;
  bool play() override;
  bool pause() override;
  bool isAvailable() override;
  SourceType getType() override { return SourceType::WEB_RADIO; }
  const char* getName() override { return "webradio"; }

  void setStreamUrl(const String& url);

private:
  PlayerStateManager* _stateManager;
  String _streamUrl;
  bool _streaming;
};

#endif // CROENIX_WEB_RADIO_SOURCE_H

