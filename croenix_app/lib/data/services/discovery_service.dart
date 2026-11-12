import 'dart:async';
import 'dart:io';
import 'package:multicast_dns/multicast_dns.dart';
import '../models/discovered_device.dart';

class DiscoveryService {
  final _devicesController = StreamController<List<DiscoveredDevice>>.broadcast();
  Stream<List<DiscoveredDevice>> get devicesStream => _devicesController.stream;

  final List<DiscoveredDevice> _discoveredDevices = [];
  MDnsClient? _mdnsClient;

  // Start mDNS discovery for ESP32 devices
  Future<void> startDiscovery() async {
    print('[Discovery] Starting mDNS discovery...');

    try {
      _mdnsClient = MDnsClient(
        rawDatagramSocketFactory: (
          dynamic host,
          int port, {
          bool reuseAddress = true,
          bool reusePort = false,
          int ttl = 1,
        }) {
          return RawDatagramSocket.bind(
            host,
            port,
            reuseAddress: true,
            reusePort: false, // Fix for Android: reusePort is not supported
            ttl: ttl,
          );
        },
      );
      await _mdnsClient!.start();

      // Query for Croenix audio service
      await for (final PtrResourceRecord ptr in _mdnsClient!.lookup<PtrResourceRecord>(
        ResourceRecordQuery.serverPointer('_croenix-audio._tcp.local'),
      )) {
        await for (final SrvResourceRecord srv in _mdnsClient!.lookup<SrvResourceRecord>(
          ResourceRecordQuery.service(ptr.domainName),
        )) {
          // Query for IP address
          await for (final IPAddressResourceRecord ip in _mdnsClient!.lookup<IPAddressResourceRecord>(
            ResourceRecordQuery.addressIPv4(srv.target),
          )) {
            final device = DiscoveredDevice(
              name: ptr.domainName,
              hostname: srv.target,
              ipAddress: ip.address.address,
              port: srv.port,
            );

            if (!_discoveredDevices.any((d) => d.ipAddress == device.ipAddress)) {
              _discoveredDevices.add(device);
              _devicesController.add(List.from(_discoveredDevices));

              print('[Discovery] Found device: ${device.name} at ${device.ipAddress}');
            }
          }
        }
      }
    } catch (e) {
      print('[Discovery] Error: $e');
    }
  }

  // Stop discovery
  void stopDiscovery() {
    print('[Discovery] Stopping discovery...');
    _mdnsClient?.stop();
    _mdnsClient = null;
  }

  // Get list of discovered devices
  List<DiscoveredDevice> get devices => List.unmodifiable(_discoveredDevices);

  // Clear discovered devices
  void clearDevices() {
    _discoveredDevices.clear();
    _devicesController.add([]);
  }

  void dispose() {
    stopDiscovery();
    _devicesController.close();
  }
}
