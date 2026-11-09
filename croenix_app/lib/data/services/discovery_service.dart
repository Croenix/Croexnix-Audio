import 'dart:async';
import 'dart:developer' as developer;
import '../../core/constants/app_constants.dart';
import '../models/discovered_device.dart';

class DiscoveryService {
  final List<DiscoveredDevice> _discoveredDevices = [];
  final _devicesController = StreamController<List<DiscoveredDevice>>.broadcast();

  Stream<List<DiscoveredDevice>> get devicesStream => _devicesController.stream;
  List<DiscoveredDevice> get devices => List.unmodifiable(_discoveredDevices);

  Future<void> startDiscovery() async {
    _discoveredDevices.clear();

    // Add hotspot fallback immediately
    _addDevice(DiscoveredDevice(
      name: AppConstants.deviceName,
      ip: AppConstants.defaultHotspotIp,
      port: AppConstants.defaultPort,
      hostname: null,
      method: DiscoveryMethod.hotspot,
    ));

    // mDNS discovery can be added in Phase 3 with proper platform support
    developer.log('Discovery started - hotspot fallback available', name: 'DiscoveryService');
  }

  void _addDevice(DiscoveredDevice device) {
    if (!_discoveredDevices.contains(device)) {
      _discoveredDevices.add(device);
      _devicesController.add(_discoveredDevices);
      developer.log('Device discovered: ${device.ip}', name: 'DiscoveryService');
    }
  }

  void addManualDevice(String ip, {int? port}) {
    _addDevice(DiscoveredDevice(
      name: AppConstants.deviceName,
      ip: ip,
      port: port ?? AppConstants.defaultPort,
      hostname: null,
      method: DiscoveryMethod.manual,
    ));
  }

  Future<void> stopDiscovery() async {
    developer.log('Discovery stopped', name: 'DiscoveryService');
  }

  void dispose() {
    stopDiscovery();
    _devicesController.close();
  }
}
