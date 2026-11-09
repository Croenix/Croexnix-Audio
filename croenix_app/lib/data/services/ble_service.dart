import 'dart:async';
import 'dart:convert';
import 'dart:developer' as developer;
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import '../../core/constants/app_constants.dart';
import '../models/discovered_device.dart';

class BleDeviceState {
  final bool isConnected;
  final String? deviceName;
  final String? deviceId;
  final String? currentIp;
  final String? authToken;

  const BleDeviceState({
    this.isConnected = false,
    this.deviceName,
    this.deviceId,
    this.currentIp,
    this.authToken,
  });

  BleDeviceState copyWith({
    bool? isConnected,
    String? deviceName,
    String? deviceId,
    String? currentIp,
    String? authToken,
  }) {
    return BleDeviceState(
      isConnected: isConnected ?? this.isConnected,
      deviceName: deviceName ?? this.deviceName,
      deviceId: deviceId ?? this.deviceId,
      currentIp: currentIp ?? this.currentIp,
      authToken: authToken ?? this.authToken,
    );
  }
}

class BleService {
  final _discoveredBleDevices = <DiscoveredDevice>[];
  final _bleDevicesController = StreamController<List<DiscoveredDevice>>.broadcast();

  BluetoothDevice? _connectedDevice;
  BluetoothCharacteristic? _charDeviceInfo;
  BluetoothCharacteristic? _charProvisioning;
  BluetoothCharacteristic? _charNetworkStatus;
  BluetoothCharacteristic? _charPairing;

  BleDeviceState _state = const BleDeviceState();
  final _stateController = StreamController<BleDeviceState>.broadcast();

  Stream<List<DiscoveredDevice>> get bleDevicesStream => _bleDevicesController.stream;
  Stream<BleDeviceState> get stateStream => _stateController.stream;
  BleDeviceState get state => _state;
  bool get isConnected => _state.isConnected;

  Future<void> startScan({Duration timeout = const Duration(seconds: 4)}) async {
    _discoveredBleDevices.clear();
    _bleDevicesController.add([]);

    try {
      final isSupported = await FlutterBluePlus.isSupported;
      if (!isSupported) {
        developer.log('Bluetooth LE not supported on this platform', name: 'BleService');
        return;
      }

      final adapterState = await FlutterBluePlus.adapterState.first;
      if (adapterState != BluetoothAdapterState.on) {
        developer.log('Bluetooth is disabled', name: 'BleService');
        return;
      }

      await FlutterBluePlus.startScan(
        timeout: timeout,
        withServices: [Guid(AppConstants.bleServiceUuid)],
      );

      FlutterBluePlus.scanResults.listen((results) {
        for (final r in results) {
          final advName = r.advertisementData.advName;
          final platformName = r.device.platformName;
          final name = advName.isNotEmpty ? advName : (platformName.isNotEmpty ? platformName : AppConstants.bleDeviceName);

          final device = DiscoveredDevice(
            name: name,
            ip: AppConstants.defaultHotspotIp,
            port: AppConstants.defaultPort,
            method: DiscoveryMethod.ble,
            bleDeviceId: r.device.remoteId.str,
          );

          if (!_discoveredBleDevices.any((d) => d.bleDeviceId == device.bleDeviceId)) {
            _discoveredBleDevices.add(device);
            _bleDevicesController.add(List.unmodifiable(_discoveredBleDevices));
            developer.log('BLE device discovered: $name (${r.device.remoteId.str})', name: 'BleService');
          }
        }
      });
    } catch (e) {
      developer.log('Error scanning for BLE: $e', name: 'BleService');
    }
  }

  Future<void> stopScan() async {
    try {
      await FlutterBluePlus.stopScan();
    } catch (_) {}
  }

  Future<BleDeviceState> connect(String remoteId) async {
    await stopScan();
    final device = BluetoothDevice.fromId(remoteId);
    _connectedDevice = device;

    try {
      await device.connect(
        license: License.nonprofit,
        autoConnect: false,
        timeout: const Duration(seconds: 10),
      );

      final services = await device.discoverServices();
      final targetServiceUuid = Guid(AppConstants.bleServiceUuid);

      final service = services.firstWhere(
        (s) => s.uuid == targetServiceUuid,
        orElse: () => throw Exception('Croenix BLE service not found on device'),
      );

      for (final char in service.characteristics) {
        final uuidStr = char.uuid.toString().toLowerCase();
        if (uuidStr == AppConstants.bleCharDeviceInfo.toLowerCase()) {
          _charDeviceInfo = char;
        } else if (uuidStr == AppConstants.bleCharProvisioning.toLowerCase()) {
          _charProvisioning = char;
        } else if (uuidStr == AppConstants.bleCharNetworkStatus.toLowerCase()) {
          _charNetworkStatus = char;
        } else if (uuidStr == AppConstants.bleCharPairing.toLowerCase()) {
          _charPairing = char;
        }
      }

      // Read device info
      String? deviceName;
      String? deviceId;
      if (_charDeviceInfo != null) {
        final bytes = await _charDeviceInfo!.read();
        final jsonStr = utf8.decode(bytes);
        try {
          final data = jsonDecode(jsonStr);
          deviceName = data['name'];
          deviceId = data['device_id'];
        } catch (_) {}
      }

      // Read or exchange pairing token
      String? token;
      if (_charPairing != null) {
        final bytes = await _charPairing!.read();
        token = utf8.decode(bytes).trim();
      }

      // Read network status for IP address
      String? ip = AppConstants.defaultHotspotIp;
      if (_charNetworkStatus != null) {
        final bytes = await _charNetworkStatus!.read();
        final jsonStr = utf8.decode(bytes);
        try {
          final data = jsonDecode(jsonStr);
          if (data['router_ip'] != null && data['router_ip'] != '0.0.0.0') {
            ip = data['router_ip'];
          } else if (data['ap_ip'] != null) {
            ip = data['ap_ip'];
          }
        } catch (_) {}
      }

      _state = _state.copyWith(
        isConnected: true,
        deviceName: deviceName ?? AppConstants.bleDeviceName,
        deviceId: deviceId,
        currentIp: ip,
        authToken: token,
      );
      _stateController.add(_state);
      return _state;
    } catch (e) {
      _state = const BleDeviceState();
      _stateController.add(_state);
      rethrow;
    }
  }

  Future<void> provisionWifi({required String ssid, required String password}) async {
    if (_charProvisioning == null) {
      throw Exception('BLE provisioning characteristic not available');
    }

    final payload = jsonEncode({'ssid': ssid, 'password': password});
    await _charProvisioning!.write(utf8.encode(payload), withoutResponse: false);
  }

  Future<void> disconnect() async {
    try {
      if (_connectedDevice != null) {
        await _connectedDevice!.disconnect();
      }
    } catch (_) {}
    _connectedDevice = null;
    _state = const BleDeviceState();
    _stateController.add(_state);
  }

  void dispose() {
    disconnect();
    _bleDevicesController.close();
    _stateController.close();
  }
}

