#include "FmAudioSource.h"
#include "../core/PlayerStateManager.h"
#include "../PinConfig.h"
#include "../Config.h"

FmAudioSource::FmAudioSource(PlayerStateManager* stateManager)
  : _stateManager(stateManager), _frequency(98.3f), _hardwareDetected(false) {
}

bool FmAudioSource::begin() {
  if (!PIN_FM_ENABLED) {
    CROENIX_LOG("FmAudioSource", "Physical FM Tuner disabled in PinConfig.h (hardware model TBD).");
    return false;
  }
  return true;
}

void FmAudioSource::stop() {
  if (_stateManager) _stateManager->setPlaybackStatus("stopped", true);
}

bool FmAudioSource::play() {
  if (!isAvailable()) return false;
  if (_stateManager) _stateManager->setPlaybackStatus("playing", true);
  return true;
}

bool FmAudioSource::pause() {
  if (_stateManager) _stateManager->setPlaybackStatus("paused", true);
  return true;
}

bool FmAudioSource::isAvailable() {
  return _hardwareDetected;
}

void FmAudioSource::setFrequency(float mhz) {
  _frequency = mhz;
  CROENIX_LOG("FmAudioSource", "FM frequency tuned to %.1f MHz", mhz);
}

float FmAudioSource::getFrequency() {
  return _frequency;
}

