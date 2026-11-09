import 'dart:async';
import 'dart:developer' as developer;
import '../../core/constants/app_constants.dart';
import '../models/discovered_device.dart';
import 'ble_service.dart';

class DiscoveryService {
  final BleService? _bleService;
  StreamSubscription? _bleSubscription;

  final List<DiscoveredDevice> _discoveredDevices = [];
  final _devicesController = StreamController<List<DiscoveredDevice>>.broadcast();

  Stream<List<DiscoveredDevice>> get devicesStream => _devicesController.stream;
  List<DiscoveredDevice> get devices => List.unmodifiable(_discoveredDevices);

  // ignore: prefer_initializing_formals
  DiscoveryService({BleService? bleService}) : _bleService = bleService;

  Future<void> startDiscovery() async {
    _discoveredDevices.clear();

    // 1. Always provide Direct Hotspot option (192.168.4.1)
    _addDevice(DiscoveredDevice(
      name: '${AppConstants.deviceName} (Wi-Fi Hotspot)',
      ip: AppConstants.defaultHotspotIp,
      port: AppConstants.defaultPort,
      hostname: null,
      method: DiscoveryMethod.hotspot,
    ));

    // 2. Start Bluetooth LE (BLE) discovery
    final ble = _bleService;
    if (ble != null) {
      await _bleSubscription?.cancel();
      _bleSubscription = ble.bleDevicesStream.listen((bleDevices) {
        for (final device in bleDevices) {
          _addDevice(device);
        }
      });
      await ble.startScan();
    }

    developer.log('Discovery started (Hotspot + Bluetooth LE)', name: 'DiscoveryService');
  }

  void _addDevice(DiscoveredDevice device) {
    final existingIndex = _discoveredDevices.indexWhere((d) => d == device);
    if (existingIndex == -1) {
      _discoveredDevices.add(device);
      _devicesController.add(List.unmodifiable(_discoveredDevices));
      developer.log('Device added to discovery list: ${device.name}', name: 'DiscoveryService');
    }
  }

  void addManualDevice(String ip, {int? port}) {
    _addDevice(DiscoveredDevice(
      name: '${AppConstants.deviceName} (Manual IP)',
      ip: ip,
      port: port ?? AppConstants.defaultPort,
      hostname: null,
      method: DiscoveryMethod.manual,
    ));
  }

  Future<void> stopDiscovery() async {
    await _bleSubscription?.cancel();
    _bleSubscription = null;
    await _bleService?.stopScan();
    developer.log('Discovery stopped', name: 'DiscoveryService');
  }

  void dispose() {
    stopDiscovery();
    _devicesController.close();
  }
}
