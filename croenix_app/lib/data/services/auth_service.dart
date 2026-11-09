import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import '../../core/constants/app_constants.dart';

class AuthService {
  final FlutterSecureStorage _storage = const FlutterSecureStorage();

  Future<String?> getToken(String deviceId) async {
    return await _storage.read(
      key: '${AppConstants.storageKeyAuthToken}$deviceId',
    );
  }

  Future<void> saveToken(String deviceId, String token) async {
    await _storage.write(
      key: '${AppConstants.storageKeyAuthToken}$deviceId',
      value: token,
    );
  }

  Future<void> deleteToken(String deviceId) async {
    await _storage.delete(
      key: '${AppConstants.storageKeyAuthToken}$deviceId',
    );
  }

  Future<void> deleteAllTokens() async {
    await _storage.deleteAll();
  }
}
