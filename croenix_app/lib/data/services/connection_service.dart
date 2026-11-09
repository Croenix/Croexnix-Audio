import 'dart:async';
import 'package:dio/dio.dart';
import '../models/connection_state.dart';
import '../models/discovered_device.dart';
import '../repositories/device_repository.dart';
import '../services/auth_service.dart';
import '../services/discovery_service.dart';
import '../../core/constants/app_constants.dart';

class ConnectionService {
  final DeviceRepository _repository;
  final AuthService _authService;
  final DiscoveryService _discoveryService;

  ConnectionState _state = ConnectionState.disconnected();
  final _stateController = StreamController<ConnectionState>.broadcast();

  Stream<ConnectionState> get stateStream => _stateController.stream;
  ConnectionState get state => _state;

  ConnectionService({
    required this._repository,
    required this._authService,
    required this._discoveryService,
  });

  void _setState(ConnectionState newState) {
    _state = newState;
    _stateController.add(_state);
  }

  Future<void> startDiscovery() async {
    _setState(_state.copyWith(status: ConnectionStatus.scanning));
    await _discoveryService.startDiscovery();
  }

  void stopDiscovery() {
    _discoveryService.stopDiscovery();
  }

  Future<void> connectToDevice(DiscoveredDevice device) async {
    try {
      _setState(ConnectionState(
        status: ConnectionStatus.connecting,
        device: device,
      ));

      // Set base URL
      _repository.setBaseUrl(device.baseUrl);

      // Check if we have a saved token
      final savedToken = await _authService.getToken(device.ip);

      if (savedToken != null) {
        // Try to authenticate with saved token
        _repository.setAuthToken(savedToken);

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
          await _authService.deleteToken(device.ip);
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
      final token = await _repository.pair();

      // Save token
      await _authService.saveToken(device.ip, token);
      _repository.setAuthToken(token);

      // Test connection
      final isValid = await _testConnection();

      if (isValid) {
        _setState(_state.copyWith(
          status: ConnectionStatus.connected,
          authToken: token,
        ));
      } else {
        throw Exception('Authentication failed');
      }
    } on DioException catch (e) {
      if (e.response?.statusCode == 403) {
        _setState(_state.copyWith(
          status: ConnectionStatus.authFailed,
          errorMessage: 'Pairing mode not active. Press encoder button on device.',
        ));
      } else {
        rethrow;
      }
    }
  }

  Future<bool> _testConnection() async {
    try {
      await _repository.getDeviceInfo();
      return true;
    } catch (e) {
      return false;
    }
  }

  Future<void> disconnect() async {
    _setState(ConnectionState.disconnected());
    _repository.setAuthToken(null);
  }

  Future<void> reconnect() async {
    if (_state.device == null) return;

    _setState(_state.copyWith(status: ConnectionStatus.reconnecting));

    await Future.delayed(Duration(seconds: AppConstants.reconnectDelay));
    await connectToDevice(_state.device!);
  }

  String _getDioErrorMessage(DioException e) {
    switch (e.type) {
      case DioExceptionType.connectionTimeout:
      case DioExceptionType.sendTimeout:
      case DioExceptionType.receiveTimeout:
        return 'Connection timeout';
      case DioExceptionType.badResponse:
        return 'Server error: ${e.response?.statusCode}';
      case DioExceptionType.connectionError:
        return 'Cannot connect to device';
      default:
        return 'Connection error';
    }
  }

  void dispose() {
    _stateController.close();
    _discoveryService.dispose();
  }
}
