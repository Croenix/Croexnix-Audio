#ifndef CROENIX_AUTH_MANAGER_H
#define CROENIX_AUTH_MANAGER_H

#include <Arduino.h>

class SettingsManager;

class AuthManager {
public:
  AuthManager(SettingsManager* settingsManager);
  void begin();
  void update();

  bool validateToken(const String& token);
  bool hasValidToken() const { return !_activeToken.isEmpty(); }
  bool isPairingModeActive();
  void enablePairingMode(unsigned long timeoutMs = 30000);
  void disablePairingMode();
  String generateAndStoreToken();
  bool confirmPairing();

  // Rate limiting
  bool isLockedOut();
  unsigned long getRemainingLockoutSeconds();

private:
  SettingsManager* _settingsManager;
  String _activeToken;
  bool _pairingMode;
  unsigned long _pairingStartTime;
  unsigned long _pairingTimeoutMs;

  int _failedAttempts;
  unsigned long _lockoutStartTime;
  bool _lockedOut;

  void _recordFailedAttempt();
  void _resetFailedAttempts();
  String _generateSecureRandomToken();
};

#endif // CROENIX_AUTH_MANAGER_H

