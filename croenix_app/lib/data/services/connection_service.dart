import 'dart:async';
import 'package:dio/dio.dart';
import '../models/connection_state.dart';
import '../models/discovered_device.dart';
import '../repositories/device_repository.dart';
import '../services/auth_service.dart';
import '../services/discovery_service.dart';
import '../services/ble_service.dart';

class ConnectionService {
  final DeviceRepository repository;
  final AuthService authService;
  final DiscoveryService discoveryService;
  final BleService? bleService;

  ConnectionState _state = ConnectionState.disconnected();
  final _stateController = StreamController<ConnectionState>.broadcast();

  Stream<ConnectionState> get stateStream => _stateController.stream;
  ConnectionState get state => _state;

  ConnectionService({
    required this.repository,
    required this.authService,
    required this.discoveryService,
    this.bleService,
  });

  void _setState(ConnectionState newState) {
    _state = newState;
    _stateController.add(_state);
  }

  Future<void> startDiscovery() async {
    _setState(_state.copyWith(status: ConnectionStatus.scanning));
    await discoveryService.startDiscovery();
  }

  void stopDiscovery() {
    discoveryService.stopDiscovery();
  }

  Future<void> connectToDevice(DiscoveredDevice device) async {
    try {
      _setState(ConnectionState(
        status: ConnectionStatus.connecting,
        device: device,
      ));

      // Handle Bluetooth LE Connection
      final ble = bleService;
      if (device.isBle && ble != null && device.bleDeviceId != null) {
        _setState(_state.copyWith(status: ConnectionStatus.authenticating));
        final bleState = await ble.connect(device.bleDeviceId!);

        if (bleState.currentIp != null && bleState.currentIp != '0.0.0.0') {
          repository.setBaseUrl('http://${bleState.currentIp}:${device.port}');
        } else {
          repository.setBaseUrl(device.baseUrl);
        }

        final token = bleState.authToken;
        if (token != null && token.isNotEmpty) {
          repository.setAuthToken(token);
          await authService.saveToken(device.ip, token);
        }

        _setState(_state.copyWith(
          status: ConnectionStatus.connected,
          authToken: token,
        ));
        return;
      }

      // Handle Wi-Fi (Hotspot / Home Router) HTTP Connection
      repository.setBaseUrl(device.baseUrl);

      // Check if we have a saved token
      final savedToken = await authService.getToken(device.ip);

      if (savedToken != null) {
        // Try to authenticate with saved token
        repository.setAuthToken(savedToken);

        _setState(_state.copyWith(
          status: ConnectionStatus.authenticating,
          authToken: savedToken,
        ));

        // Test the token
        final isValid = await _testConnection();

        if (isValid) {
          _setState(_state.copyWith(status: ConnectionStatus.connected));
          return;
        } else {
          // Token invalid, need to re-pair
          await authService.deleteToken(device.ip);
        }
      }

      // Need to pair
      await _pairWithDevice(device);

    } on DioException catch (e) {
      _setState(_state.copyWith(
        status: ConnectionStatus.error,
        errorMessage: _getDioErrorMessage(e),
      ));
    } catch (e) {
      _setState(_state.copyWith(
        status: ConnectionStatus.error,
        errorMessage: e.toString(),
      ));
    }
  }

  Future<void> _pairWithDevice(DiscoveredDevice device) async {
    _setState(_state.copyWith(status: ConnectionStatus.authenticating));

    try {
      // Call pair endpoint
      final token = await repository.pair();

      // Save token
      await authService.saveToken(device.ip, token);
      repository.setAuthToken(token);

      _setState(_state.copyWith(
        status: ConnectionStatus.connected,
        authToken: token,
      ));
    } on DioException catch (e) {
      if (e.response?.statusCode == 403) {
        _setState(_state.copyWith(
          status: ConnectionStatus.authFailed,
          errorMessage: 'Pairing mode not enabled on device. Press pairing button.',
        ));
      } else {
        _setState(_state.copyWith(
          status: ConnectionStatus.error,
          errorMessage: _getDioErrorMessage(e),
        ));
      }
    } catch (e) {
      _setState(_state.copyWith(
        status: ConnectionStatus.error,
        errorMessage: e.toString(),
      ));
    }
  }

  Future<bool> _testConnection() async {
    try {
      await repository.getDeviceInfo();
      return true;
    } catch (e) {
      return false;
    }
  }

  Future<void> disconnect() async {
    final currentDevice = _state.device;
    if (currentDevice != null) {
      await authService.deleteToken(currentDevice.ip);
    }

    repository.setAuthToken(null);
    _setState(ConnectionState.disconnected());
  }

  String _getDioErrorMessage(DioException e) {
    switch (e.type) {
      case DioExceptionType.connectionTimeout:
      case DioExceptionType.sendTimeout:
      case DioExceptionType.receiveTimeout:
        return 'Connection timed out. Check device IP and network.';
      case DioExceptionType.badResponse:
        final statusCode = e.response?.statusCode;
        if (statusCode == 401) {
          return 'Authentication failed. Please re-pair.';
        } else if (statusCode == 403) {
          return 'Access denied. Pairing mode not active.';
        } else if (statusCode == 404) {
          return 'Device endpoint not found.';
        }
        return 'Server error ($statusCode).';
      case DioExceptionType.connectionError:
        return 'Cannot connect to device. Ensure Wi-Fi/Hotspot or Bluetooth is connected.';
      default:
        return 'Network error: ${e.message}';
    }
  }

  void dispose() {
    _stateController.close();
    discoveryService.dispose();
  }
}
