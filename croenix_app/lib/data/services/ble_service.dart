import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'websocket_service.dart';

class BleService {
  // Singleton pattern
  static final BleService _instance = BleService._internal();
  factory BleService() => _instance;
  BleService._internal();

  static const String serviceUuid = '000000ff-0000-1000-8000-00805f9b34fb';
  static const String cmdCharUuid = '0000ff01-0000-1000-8000-00805f9b34fb';
  static const String statusCharUuid = '0000ff02-0000-1000-8000-00805f9b34fb';
  static const String infoCharUuid = '0000ff03-0000-1000-8000-00805f9b34fb';

  BluetoothDevice? _connectedDevice;
  BluetoothCharacteristic? _cmdChar;
  BluetoothCharacteristic? _statusChar;

  bool _isConnecting = false;
  bool _isConnected = false;
  bool get isConnected => _isConnected;

  final _bleConnectionController = StreamController<bool>.broadcast();
  Stream<bool> get bleConnectionStream => _bleConnectionController.stream;

  StreamSubscription? _scanSub;
  StreamSubscription? _notifySub;
  StreamSubscription? _connStateSub;

  Future<void> startAutoScanAndConnect() async {
    if (_isConnected || _isConnecting) return;

    // Check if Bluetooth is supported on this platform
    if (!Platform.isAndroid && !Platform.isIOS) {
      return;
    }

    try {
      final isSupported = await FlutterBluePlus.isSupported;
      if (!isSupported) return;

      final adapterState = await FlutterBluePlus.adapterState.first;
      if (adapterState != BluetoothAdapterState.on) {
        return;
      }

      _isConnecting = true;
      _scanSub?.cancel();

      await FlutterBluePlus.startScan(
        timeout: const Duration(seconds: 6),
        withNames: ['CROENIX', 'Croenix BT', 'Croenix Audio'],
      );

      _scanSub = FlutterBluePlus.scanResults.listen((results) async {
        for (final r in results) {
          final name = r.device.platformName;
          if (name == 'CROENIX' || name.contains('Croenix')) {
            await FlutterBluePlus.stopScan();
            await _connectToDevice(r.device);
            break;
          }
        }
      });
    } catch (_) {
      _isConnecting = false;
    }
  }

  Future<void> _connectToDevice(BluetoothDevice device) async {
    try {
      _connectedDevice = device;
      await device.connect(
        license: License.nonprofit,
        autoConnect: false,
        timeout: const Duration(seconds: 5),
      );

      _connStateSub?.cancel();
      _connStateSub = device.connectionState.listen((state) {
        if (state == BluetoothConnectionState.connected) {
          _isConnected = true;
          _bleConnectionController.add(true);
        } else if (state == BluetoothConnectionState.disconnected) {
          _isConnected = false;
          _bleConnectionController.add(false);
          _cmdChar = null;
          _statusChar = null;
        }
      });

      // Discover Services
      final services = await device.discoverServices();
      for (final s in services) {
        if (s.uuid.toString().toLowerCase().contains('00ff')) {
          for (final c in s.characteristics) {
            final uuid = c.uuid.toString().toLowerCase();
            if (uuid.contains('ff01')) {
              _cmdChar = c;
            } else if (uuid.contains('ff02')) {
              _statusChar = c;
              await _subscribeToStatus(_statusChar!);
            }
          }
        }
      }

      _isConnecting = false;
    } catch (_) {
      _isConnecting = false;
      _isConnected = false;
    }
  }

  Future<void> _subscribeToStatus(BluetoothCharacteristic char) async {
    try {
      await char.setNotifyValue(true);
      _notifySub?.cancel();
      _notifySub = char.onValueReceived.listen((bytes) {
        if (bytes.isNotEmpty) {
          final str = utf8.decode(bytes, allowMalformed: true);
          _parseStatusJson(str);
        }
      });
    } catch (_) {}
  }

  void _parseStatusJson(String payload) {
    try {
      final data = jsonDecode(payload) as Map<String, dynamic>;
      // If ESP32 reports a home Wi-Fi IP address, trigger WebSocket connection
      if (data.containsKey('ip')) {
        final ip = data['ip'] as String?;
        if (ip != null && ip.isNotEmpty && !WebSocketService().isConnected) {
          WebSocketService().connect(ip);
        }
      }
    } catch (_) {}
  }

  Future<bool> sendCommand(String cmd) async {
    if (_cmdChar != null && _isConnected) {
      try {
        final bytes = utf8.encode(cmd);
        await _cmdChar!.write(bytes, withoutResponse: true);
        return true;
      } catch (_) {
        return false;
      }
    }
    return false;
  }

  Future<void> disconnect() async {
    _scanSub?.cancel();
    _notifySub?.cancel();
    _connStateSub?.cancel();
    await _connectedDevice?.disconnect();
    _isConnected = false;
    _isConnecting = false;
    _bleConnectionController.add(false);
  }
}
