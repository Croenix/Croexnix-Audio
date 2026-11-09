import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import '../../core/constants/app_constants.dart';

class PasslockService {
  final FlutterSecureStorage _storage;
  bool _sessionUnlocked = false;

  PasslockService({FlutterSecureStorage? storage})
      : _storage = storage ?? const FlutterSecureStorage();

  bool get isSessionUnlocked => _sessionUnlocked;

  void markSessionUnlocked() {
    _sessionUnlocked = true;
  }

  void lockSession() {
    _sessionUnlocked = false;
  }

  Future<bool> isPasslockEnabled() async {
    final enabled = await _storage.read(key: AppConstants.storageKeyPasslockEnabled);
    return enabled == 'true';
  }

  Future<void> setPasslock(String pin) async {
    await _storage.write(key: AppConstants.storageKeyPasslockPin, value: pin);
    await _storage.write(key: AppConstants.storageKeyPasslockEnabled, value: 'true');
    _sessionUnlocked = true;
  }

  Future<bool> verifyPasslock(String pin) async {
    final storedPin = await _storage.read(key: AppConstants.storageKeyPasslockPin);
    final isValid = (storedPin != null && storedPin == pin);
    if (isValid) {
      _sessionUnlocked = true;
    }
    return isValid;
  }

  Future<void> disablePasslock() async {
    await _storage.delete(key: AppConstants.storageKeyPasslockPin);
    await _storage.write(key: AppConstants.storageKeyPasslockEnabled, value: 'false');
    _sessionUnlocked = true;
  }
}

