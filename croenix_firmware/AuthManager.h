#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class AuthManager {
public:
  AuthManager();
  void begin();

  // Authentication
  bool validateToken(const String& token);
  String getAuthToken();

  // Pairing
  bool isPairingModeActive();
  void enablePairingMode();
  void disablePairingMode();
  bool confirmPairing();  // Called when encoder pressed
  String generateNewToken();

  // Rate limiting
  bool checkRateLimit();
  void recordFailedAttempt();
  void resetFailedAttempts();

private:
  Preferences _prefs;
  String _authToken;
  bool _pairingMode;
  unsigned long _pairingModeStartTime;

  // Rate limiting
  int _failedAttempts;
  unsigned long _lockoutStartTime;
  bool _isLockedOut;

  void _loadAuthToken();
  void _saveAuthToken(const String& token);
  String _generateRandomToken();
};

#endif // AUTH_MANAGER_H
