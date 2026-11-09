#include "AuthManager.h"
#include "../storage/SettingsManager.h"
#include "../Config.h"
#include <esp_system.h>

AuthManager::AuthManager(SettingsManager* settingsManager)
  : _settingsManager(settingsManager),
    _pairingMode(false),
    _pairingStartTime(0),
    _pairingTimeoutMs(PAIRING_TIMEOUT_MS),
    _failedAttempts(0),
    _lockoutStartTime(0),
    _lockedOut(false) {
}

void AuthManager::begin() {
  if (_settingsManager) {
    _activeToken = _settingsManager->getAuthToken();
  }

  if (_activeToken.isEmpty()) {
    CROENIX_LOG("AuthManager", "No token found in NVS. First boot: enabling pairing window.");
    enablePairingMode();
  } else {
    CROENIX_LOG("AuthManager", "Token loaded from NVS. Pairing secured.");
  }
}

void AuthManager::update() {
  if (_pairingMode) {
    if (millis() - _pairingStartTime > _pairingTimeoutMs) {
      _pairingMode = false;
      CROENIX_LOG("AuthManager", "Pairing window closed (timeout).");
    }
  }

  if (_lockedOut) {
    if (millis() - _lockoutStartTime > AUTH_LOCKOUT_DURATION_MS) {
      _lockedOut = false;
      _failedAttempts = 0;
      CROENIX_LOG("AuthManager", "Rate-limit lockout expired. Access restored.");
    }
  }
}

bool AuthManager::isLockedOut() {
  update();
  return _lockedOut;
}

unsigned long AuthManager::getRemainingLockoutSeconds() {
  if (!_lockedOut) return 0;
  unsigned long elapsed = millis() - _lockoutStartTime;
  if (elapsed >= AUTH_LOCKOUT_DURATION_MS) return 0;
  return (AUTH_LOCKOUT_DURATION_MS - elapsed) / 1000;
}

bool AuthManager::validateToken(const String& token) {
  if (isLockedOut()) {
    CROENIX_LOG("AuthManager", "Token validation rejected: Device locked out.");
    return false;
  }

  if (_activeToken.isEmpty() || token.isEmpty()) {
    _recordFailedAttempt();
    return false;
  }

  if (token.length() != _activeToken.length()) {
    _recordFailedAttempt();
    return false;
  }

  // Constant-time comparison to mitigate timing attacks
  volatile uint8_t diff = 0;
  for (size_t i = 0; i < token.length(); i++) {
    diff |= (token[i] ^ _activeToken[i]);
  }

  if (diff == 0) {
    _resetFailedAttempts();
    return true;
  } else {
    _recordFailedAttempt();
    return false;
  }
}

bool AuthManager::isPairingModeActive() {
  update();
  return _pairingMode;
}

void AuthManager::enablePairingMode(unsigned long timeoutMs) {
  _pairingMode = true;
  _pairingStartTime = millis();
  _pairingTimeoutMs = timeoutMs;
  CROENIX_LOG("AuthManager", "Pairing mode ENABLED for %lu ms.", timeoutMs);
}

void AuthManager::disablePairingMode() {
  _pairingMode = false;
  CROENIX_LOG("AuthManager", "Pairing mode DISABLED.");
}

String AuthManager::generateAndStoreToken() {
  if (!isPairingModeActive()) {
    CROENIX_LOG("AuthManager", "Token request rejected: Pairing mode not active.");
    return "";
  }

  String newToken = _generateSecureRandomToken();
  _activeToken = newToken;
  if (_settingsManager) {
    _settingsManager->saveAuthToken(newToken);
  }
  _pairingMode = false;
  CROENIX_LOG("AuthManager", "New pairing token generated and committed to NVS.");
  return newToken;
}

bool AuthManager::confirmPairing() {
  if (!isPairingModeActive()) return false;
  generateAndStoreToken();
  return true;
}

void AuthManager::_recordFailedAttempt() {
  _failedAttempts++;
  CROENIX_LOG("AuthManager", "Failed auth attempt (%d / %d)", _failedAttempts, MAX_FAILED_AUTH_ATTEMPTS);
  if (_failedAttempts >= MAX_FAILED_AUTH_ATTEMPTS) {
    _lockedOut = true;
    _lockoutStartTime = millis();
    CROENIX_LOG("AuthManager", "MAX FAILED ATTEMPTS REACHED: Locked out for %d seconds", AUTH_LOCKOUT_DURATION_MS / 1000);
  }
}

void AuthManager::_resetFailedAttempts() {
  _failedAttempts = 0;
  _lockedOut = false;
}

String AuthManager::_generateSecureRandomToken() {
  const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  const size_t charsetLen = sizeof(charset) - 1;
  const size_t tokenLength = 32;

  char tokenBuf[tokenLength + 1];
  for (size_t i = 0; i < tokenLength; i++) {
    uint32_t randVal = esp_random();
    tokenBuf[i] = charset[randVal % charsetLen];
  }
  tokenBuf[tokenLength] = '\0';
  return String(tokenBuf);
}

