#include "SDManager.h"
#include "../core/PlayerStateManager.h"
#include "../PinConfig.h"
#include "../Config.h"

SDManager::SDManager(PlayerStateManager* stateManager)
  : _stateManager(stateManager),
    _cardDetected(false),
    _totalBytes(0),
    _usedBytes(0),
    _freeBytes(0),
    _lastCheckMs(0) {
}

void SDManager::begin() {
  if (PIN_SD_CS < 0) {
    CROENIX_LOG("SDManager", "SD CS pin not configured in PinConfig.h. SD interface inactive.");
    return;
  }
  _checkCardState();
}

void SDManager::update() {
  unsigned long now = millis();
  if (now - _lastCheckMs < 3000) return;
  _lastCheckMs = now;

  _checkCardState();
}

void SDManager::_checkCardState() {
  // If card detect switch pin is wired
  if (PIN_SD_DETECT >= 0) {
    bool raw = (digitalRead(PIN_SD_DETECT) == LOW);
    if (raw != _cardDetected) {
      _cardDetected = raw;
      if (_stateManager) {
        _stateManager->updateStorage(_cardDetected, _totalBytes, _usedBytes, _freeBytes);
      }
    }
  } else {
    // Without hardware card detect switch, state defaults to false until card is mounted
    if (_stateManager) {
      _stateManager->updateStorage(_cardDetected, _totalBytes, _usedBytes, _freeBytes);
    }
  }
}

bool SDManager::isInserted() {
  return _cardDetected;
}

uint64_t SDManager::getTotalBytes() {
  return _totalBytes;
}

uint64_t SDManager::getUsedBytes() {
  return _usedBytes;
}

uint64_t SDManager::getFreeBytes() {
  return _freeBytes;
}

