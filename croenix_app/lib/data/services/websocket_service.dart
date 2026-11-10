import 'dart:async';
import 'dart:convert';
import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:web_socket_channel/status.dart' as status;
import '../models/device_state.dart';

class WebSocketService {
  WebSocketChannel? _channel;
  final _stateController = StreamController<DeviceState>.broadcast();
  final _connectionController = StreamController<bool>.broadcast();

  Stream<DeviceState> get stateStream => _stateController.stream;
  Stream<bool> get connectionStream => _connectionController.stream;

  bool _isConnected = false;
  bool get isConnected => _isConnected;

  String? _deviceIP;
  Timer? _reconnectTimer;
  int _reconnectAttempts = 0;

  // Connect to ESP32 device
  Future<bool> connect(String deviceIP, {int port = 8080}) async {
    try {
      _deviceIP = deviceIP;
      final uri = Uri.parse('ws://$deviceIP:$port');

      print('[WebSocket] Connecting to $uri...');

      _channel = WebSocketChannel.connect(uri);

      // Listen to incoming messages
      _channel!.stream.listen(
        _handleMessage,
        onError: _handleError,
        onDone: _handleDisconnect,
        cancelOnError: false,
      );

      _isConnected = true;
      _reconnectAttempts = 0;
      _connectionController.add(true);

      print('[WebSocket] Connected successfully');
      return true;
    } catch (e) {
      print('[WebSocket] Connection failed: $e');
      _handleDisconnect();
      return false;
    }
  }

  // Disconnect from device
  void disconnect() {
    print('[WebSocket] Disconnecting...');
    _reconnectTimer?.cancel();
    _channel?.sink.close(status.goingAway);
    _isConnected = false;
    _connectionController.add(false);
  }

  // Send command to ESP32
  void sendCommand(String command, Map<String, dynamic>? params) {
    if (!_isConnected || _channel == null) {
      print('[WebSocket] Cannot send command: not connected');
      return;
    }

    final payload = {
      'command': command,
      if (params != null) ...params,
    };

    final jsonString = jsonEncode(payload);
    _channel!.sink.add(jsonString);

    print('[WebSocket] Sent: $command');
  }

  // === Audio Control Commands ===
  void play() => sendCommand('play', null);
  void pause() => sendCommand('pause', null);
  void stop() => sendCommand('stop', null);

  void setVolume(int volume) => sendCommand('volume', {'value': volume});

  void setSource(AudioSource source) =>
      sendCommand('source', {'value': source.index});

  void setEQPreset(int preset) => sendCommand('eq', {'value': preset});

  // === Wi-Fi Configuration ===
  void configureWiFi(String ssid, String password) {
    sendCommand('wifi_config', {'ssid': ssid, 'password': password});
  }

  void scanNetworks() => sendCommand('scan_networks', null);

  // === File Management ===
  void listFiles(String path) => sendCommand('file_list', {'path': path});

  void loadFile(String filepath) =>
      sendCommand('load_file', {'path': filepath});

  // === FM Radio ===
  void setFMFrequency(double frequency) =>
      sendCommand('fm_frequency', {'value': frequency});

  // === Web Streaming ===
  void loadStreamURL(String url) => sendCommand('stream_url', {'url': url});

  // === Message Handling ===
  void _handleMessage(dynamic message) {
    try {
      final data = jsonDecode(message as String);
      final type = data['type'] as String?;

      switch (type) {
        case 'state_update':
          _handleStateUpdate(data);
          break;
        case 'metadata':
          _handleMetadata(data);
          break;
        case 'network_info':
          _handleNetworkInfo(data);
          break;
        case 'scan_result':
          _handleScanResult(data);
          break;
        case 'file_list':
          _handleFileList(data);
          break;
        default:
          print('[WebSocket] Unknown message type: $type');
      }
    } catch (e) {
      print('[WebSocket] Failed to parse message: $e');
    }
  }

  void _handleStateUpdate(Map<String, dynamic> data) {
    final state = DeviceState.fromJson(data);
    _stateController.add(state);
  }

  void _handleMetadata(Map<String, dynamic> data) {
    // Handle metadata update
    print('[WebSocket] Metadata: ${data['title']}');
  }

  void _handleNetworkInfo(Map<String, dynamic> data) {
    // Handle network info
    print('[WebSocket] Network: ${data['local_ip']}');
  }

  void _handleScanResult(Map<String, dynamic> data) {
    // Handle Wi-Fi scan results
    print('[WebSocket] Found ${data['count']} networks');
  }

  void _handleFileList(Map<String, dynamic> data) {
    // Handle file list
    print('[WebSocket] File list received');
  }

  void _handleError(error) {
    print('[WebSocket] Error: $error');
    _handleDisconnect();
  }

  void _handleDisconnect() {
    print('[WebSocket] Disconnected');
    _isConnected = false;
    _connectionController.add(false);

    // Auto-reconnect logic
    if (_deviceIP != null && _reconnectAttempts < 5) {
      _reconnectAttempts++;
      final delay = Duration(seconds: _reconnectAttempts * 2);

      print('[WebSocket] Reconnecting in ${delay.inSeconds}s (attempt $_reconnectAttempts)');

      _reconnectTimer = Timer(delay, () {
        connect(_deviceIP!);
      });
    }
  }

  void dispose() {
    _reconnectTimer?.cancel();
    _channel?.sink.close();
    _stateController.close();
    _connectionController.close();
  }
}
