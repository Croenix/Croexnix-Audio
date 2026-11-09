#include "AudioManager.h"
#include "../core/PlayerStateManager.h"
#include "../Config.h"

AudioManager::AudioManager(PlayerStateManager* stateManager)
  : _stateManager(stateManager),
    _sdSource(stateManager),
    _btSource(stateManager),
    _fmSource(stateManager),
    _webSource(stateManager),
    _activeSource(nullptr) {
}

void AudioManager::begin() {
  CROENIX_LOG("AudioManager", "Initializing Central Audio Subsystem...");
  _i2s.begin(I2S_SAMPLE_RATE, I2S_BIT_DEPTH);

  _sdSource.begin();
  _btSource.begin();
  _fmSource.begin();
  _webSource.begin();

  CROENIX_LOG("AudioManager", "Audio sources initialized and standing by.");
}

void AudioManager::update() {
  if (_activeSource) {
    _activeSource->update();
  }
}

bool AudioManager::setVolume(int volume) {
  if (!_stateManager) return false;
  return _stateManager->setVolume(volume, true);
}

bool AudioManager::setMute(bool muted) {
  if (!_stateManager) return false;
  return _stateManager->setMute(muted, true);
}

bool AudioManager::setSource(const String& sourceName) {
  if (sourceName == "none") {
    if (_activeSource) _activeSource->stop();
    _activeSource = nullptr;
    if (_stateManager) _stateManager->setSource("none", true);
    return true;
  }

  AudioSource* target = nullptr;
  if (sourceName == "sd") target = &_sdSource;
  else if (sourceName == "bluetooth") target = &_btSource;
  else if (sourceName == "fm") target = &_fmSource;
  else if (sourceName == "webradio") target = &_webSource;

  if (!target) return false;

  if (!target->isAvailable()) {
    CROENIX_LOG("AudioManager", "Source '%s' requested but not currently available", sourceName.c_str());
    return false;
  }

  if (_activeSource) _activeSource->stop();
  _activeSource = target;
  if (_stateManager) _stateManager->setSource(sourceName, true);
  return true;
}

bool AudioManager::play() {
  if (_activeSource) {
    return _activeSource->play();
  }
  return false;
}

bool AudioManager::pause() {
  if (_activeSource) {
    return _activeSource->pause();
  }
  return false;
}

void AudioManager::stop() {
  if (_activeSource) {
    _activeSource->stop();
  }
}

String AudioManager::getCurrentSource() {
  return _activeSource ? _activeSource->getName() : "none";
}

bool AudioManager::isPlaying() {
  if (!_stateManager) return false;
  return _stateManager->getState().audio.status == "playing";
}

