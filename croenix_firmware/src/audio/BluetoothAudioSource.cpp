#include "BluetoothAudioSource.h"
#include "../core/PlayerStateManager.h"
#include "../Config.h"

BluetoothAudioSource::BluetoothAudioSource(PlayerStateManager* stateManager)
  : _stateManager(stateManager), _connected(false) {
}

bool BluetoothAudioSource::begin() {
  CROENIX_LOG("BTAudioSource", "Bluetooth Classic A2DP Sink architecture registered.");
  return true;
}

void BluetoothAudioSource::stop() {
  _connected = false;
  if (_stateManager) _stateManager->setPlaybackStatus("stopped", true);
}

bool BluetoothAudioSource::play() {
  if (!isAvailable()) return false;
  if (_stateManager) _stateManager->setPlaybackStatus("playing", true);
  return true;
}

bool BluetoothAudioSource::pause() {
  if (_stateManager) _stateManager->setPlaybackStatus("paused", true);
  return true;
}

bool BluetoothAudioSource::isAvailable() {
  return _connected;
}

