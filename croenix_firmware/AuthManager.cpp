#include "AuthManager.h"
#include "Config.h"

AuthManager::AuthManager()
  : _pairingMode(false),
    _pairingModeStartTime(0),
    _failedAttempts(0),
    _lockoutStartTime(0),
    _isLockedOut(false) {
}

void AuthManager::begin() {
  _loadAuthToken();

  // If no token exists, enable pairing mode
  if (_authToken.isEmpty()) {
    DEBUG_PRINTLN("[AuthManager] No auth token found - enabling pairing mode");
    enablePairingMode();
  } else {
    DEBUG_PRINTLN("[AuthManager] Auth token loaded from NVS");
  }
}

bool AuthManager::validateToken(const String& token) {
  // Check rate limit
  if (!checkRateLimit()) {
    DEBUG_PRINTLN("[AuthManager] Rate limit exceeded");
    return false;
  }

  // Validate token
  bool valid = !_authToken.isEmpty() && token == _authToken;

  if (!valid) {
    recordFailedAttempt();
    DEBUG_PRINTLN("[AuthManager] Invalid token");
  } else {
    resetFailedAttempts();
  }

  return valid;
}

String AuthManager::getAuthToken() {
  return _authToken;
}

bool AuthManager::isPairingModeActive() {
  // Check if pairing mode timed out
  if (_pairingMode) {
    unsigned long now = millis();
    if (now - _pairingModeStartTime > PAIRING_TIMEOUT_MS) {
      disablePairingMode();
      DEBUG_PRINTLN("[AuthManager] Pairing mode timed out");
      return false;
    }
  }
  return _pairingMode;
}

void AuthManager::enablePairingMode() {
  _pairingMode = true;
  _pairingModeStartTime = millis();
  DEBUG_PRINTLN("[AuthManager] Pairing mode enabled");
}

void AuthManager::disablePairingMode() {
  _pairingMode = false;
  DEBUG_PRINTLN("[AuthManager] Pairing mode disabled");
}

bool AuthManager::confirmPairing() {
  if (!_pairingMode) {
    return false;
  }

  // Generate new token
  String newToken = _generateRandomToken();
  _saveAuthToken(newToken);
  _authToken = newToken;

  disablePairingMode();
  DEBUG_PRINTLN("[AuthManager] Pairing confirmed - new token generated");

  return true;
}

String AuthManager::generateNewToken() {
  if (!isPairingModeActive()) {
    DEBUG_PRINTLN("[AuthManager] Cannot generate token - pairing mode not active");
    return "";
  }

  String newToken = _generateRandomToken();
  _saveAuthToken(newToken);
  _authToken = newToken;

  disablePairingMode();

  return newToken;
}

bool AuthManager::checkRateLimit() {
  unsigned long now = millis();

  // Check if currently locked out
  if (_isLockedOut) {
    if (now - _lockoutStartTime >= AUTH_LOCKOUT_DURATION_MS) {
      // Lockout expired
      _isLockedOut = false;
      _failedAttempts = 0;
      DEBUG_PRINTLN("[AuthManager] Lockout expired");
    } else {
      return false;
    }
  }

  return true;
}

void AuthManager::recordFailedAttempt() {
  _failedAttempts++;

  if (_failedAttempts >= MAX_FAILED_AUTH_ATTEMPTS) {
    _isLockedOut = true;
    _lockoutStartTime = millis();
    DEBUG_PRINT("[AuthManager] Too many failed attempts - locked out for ");
    DEBUG_PRINT(AUTH_LOCKOUT_DURATION_MS / 1000);
    DEBUG_PRINTLN(" seconds");
  }
}

void AuthManager::resetFailedAttempts() {
  _failedAttempts = 0;
  _isLockedOut = false;
}

// Private methods

void AuthManager::_loadAuthToken() {
  _prefs.begin(NVS_NAMESPACE, true); // Read-only
  _authToken = _prefs.getString(NVS_KEY_AUTH_TOKEN, "");
  _prefs.end();
}

void AuthManager::_saveAuthToken(const String& token) {
  _prefs.begin(NVS_NAMESPACE, false);
  _prefs.putString(NVS_KEY_AUTH_TOKEN, token);
  _prefs.end();
  DEBUG_PRINTLN("[AuthManager] Token saved to NVS");
}

String AuthManager::_generateRandomToken() {
  const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  const int tokenLength = 32;
  String token = "";

  // Seed random with hardware RNG
  randomSeed(esp_random());

  for (int i = 0; i < tokenLength; i++) {
    int index = random(0, sizeof(charset) - 1);
    token += charset[index];
  }

  return token;
}
