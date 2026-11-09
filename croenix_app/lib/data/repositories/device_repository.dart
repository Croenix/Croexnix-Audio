import 'package:dio/dio.dart';
import '../../core/constants/app_constants.dart';
import '../models/device_state.dart';

class DeviceRepository {
  final Dio _dio;
  String? _authToken;

  DeviceRepository({Dio? dio})
      : _dio = dio ??
            Dio(BaseOptions(
              connectTimeout: Duration(seconds: AppConstants.connectionTimeout),
              receiveTimeout: Duration(seconds: AppConstants.requestTimeout),
              sendTimeout: Duration(seconds: AppConstants.requestTimeout),
            ));

  void setBaseUrl(String baseUrl) {
    _dio.options.baseUrl = baseUrl;
  }

  void setAuthToken(String? token) {
    _authToken = token;
  }

  Map<String, String> get _authHeaders {
    if (_authToken == null) return {};
    return {'Authorization': 'Bearer $_authToken'};
  }

  // Device Info
  Future<Map<String, dynamic>> getDeviceInfo() async {
    final response = await _dio.get(
      '/api/device',
      options: Options(headers: _authHeaders),
    );
    return response.data;
  }

  // Status
  Future<DeviceState> getStatus() async {
    final response = await _dio.get(
      '/api/status',
      options: Options(headers: _authHeaders),
    );
    return DeviceState.fromJson(response.data);
  }

  // Network
  Future<Map<String, dynamic>> getNetworkStatus() async {
    final response = await _dio.get(
      '/api/network',
      options: Options(headers: _authHeaders),
    );
    return response.data;
  }

  // System
  Future<Map<String, dynamic>> getSystemInfo() async {
    final response = await _dio.get(
      '/api/system',
      options: Options(headers: _authHeaders),
    );
    return response.data;
  }

  // Pairing
  Future<String> pair() async {
    final response = await _dio.post('/api/pair');
    return response.data['token'];
  }

  // Commands
  Future<Map<String, dynamic>> executeCommand(
    String command,
    Map<String, dynamic> params,
  ) async {
    final response = await _dio.post(
      '/api/commands',
      data: {
        'command': command,
        'params': params,
      },
      options: Options(headers: _authHeaders),
    );
    return response.data;
  }

  // Network Configuration
  Future<void> configureWiFi(String ssid, String password) async {
    await _dio.post(
      '/api/network/config',
      data: {
        'ssid': ssid,
        'password': password,
      },
      options: Options(headers: _authHeaders),
    );
  }

  // Restart
  Future<void> restart() async {
    await _dio.post(
      '/api/device/restart',
      options: Options(headers: _authHeaders),
    );
  }
}
