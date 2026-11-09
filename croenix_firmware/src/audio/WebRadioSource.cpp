#include "WebRadioSource.h"
#include "../core/PlayerStateManager.h"
#include "../Config.h"

WebRadioSource::WebRadioSource(PlayerStateManager* stateManager)
  : _stateManager(stateManager), _streamUrl(""), _streaming(false) {
}

bool WebRadioSource::begin() {
  CROENIX_LOG("WebRadioSource", "Web radio streaming architecture ready.");
  return true;
}

void WebRadioSource::stop() {
  _streaming = false;
  if (_stateManager) _stateManager->setPlaybackStatus("stopped", true);
}

bool WebRadioSource::play() {
  if (_streamUrl.isEmpty()) return false;
  _streaming = true;
  if (_stateManager) _stateManager->setPlaybackStatus("playing", true);
  return true;
}

bool WebRadioSource::pause() {
  _streaming = false;
  if (_stateManager) _stateManager->setPlaybackStatus("paused", true);
  return true;
}

bool WebRadioSource::isAvailable() {
  if (!_stateManager) return false;
  return _stateManager->getState().connection.router_connected;
}

void WebRadioSource::setStreamUrl(const String& url) {
  _streamUrl = url;
  CROENIX_LOG("WebRadioSource", "Target stream URL set to: %s", url.c_str());
}

