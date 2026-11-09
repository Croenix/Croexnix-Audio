#include "SdAudioSource.h"
#include "../core/PlayerStateManager.h"
#include "../Config.h"

SdAudioSource::SdAudioSource(PlayerStateManager* stateManager)
  : _stateManager(stateManager), _isPlaying(false) {
}

bool SdAudioSource::begin() {
  CROENIX_LOG("SdAudioSource", "SD Audio subsystem initialized.");
  return true;
}

void SdAudioSource::stop() {
  _isPlaying = false;
  if (_stateManager) _stateManager->setPlaybackStatus("stopped", true);
}

bool SdAudioSource::play() {
  if (!isAvailable()) return false;
  _isPlaying = true;
  if (_stateManager) _stateManager->setPlaybackStatus("playing", true);
  return true;
}

bool SdAudioSource::pause() {
  _isPlaying = false;
  if (_stateManager) _stateManager->setPlaybackStatus("paused", true);
  return true;
}

bool SdAudioSource::isAvailable() {
  if (!_stateManager) return false;
  return _stateManager->getState().storage.sd_inserted;
}

void SdAudioSource::update() {
  // Decoding task / buffer feeder loop placeholder
}

