import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;
import 'package:web_socket_channel/web_socket_channel.dart';
import '../models/player_state.dart';
import '../models/sd_file.dart';

enum ConnectionStateEnum {
  disconnected,
  connecting,
  connected,
  error,
}

class Esp32Service extends ChangeNotifier {
  String _espIp = '192.168.4.1'; // Default SoftAP IP of ESP32
  final int _port = 80;
  
  ConnectionStateEnum _connectionState = ConnectionStateEnum.disconnected;
  String _lastErrorMessage = '';

  PlayerStateModel _playerState = PlayerStateModel();
  List<SdFile> _sdFiles = [];
  bool _isLoadingFiles = false;

  WebSocketChannel? _wsChannel;
  StreamSubscription? _wsSubscription;
  Timer? _heartbeatTimer;
  Timer? _reconnectTimer;
  Timer? _fallbackPollTimer;

  Esp32Service() {
    connect();
  }

  // Getters
  String get espIp => _espIp;
  ConnectionStateEnum get connectionState => _connectionState;
  bool get isConnected => _connectionState == ConnectionStateEnum.connected;
  String get lastErrorMessage => _lastErrorMessage;
  PlayerStateModel get playerState => _playerState;
  List<SdFile> get sdFiles => _sdFiles;
  bool get isLoadingFiles => _isLoadingFiles;

  void updateEspIp(String newIp) {
    if (newIp.trim().isEmpty || newIp == _espIp) return;
    _espIp = newIp.trim();
    disconnect();
    connect();
    notifyListeners();
  }

  void connect() {
    if (_connectionState == ConnectionStateEnum.connecting ||
        _connectionState == ConnectionStateEnum.connected) {
      return;
    }

    _connectionState = ConnectionStateEnum.connecting;
    notifyListeners();

    _setupWebSocket();
    fetchStatus();
    fetchSdFiles();

    // Start fallback poll timer (every 2.5s) in case WS packet drops
    _fallbackPollTimer?.cancel();
    _fallbackPollTimer = Timer.periodic(const Duration(milliseconds: 2500), (_) {
      if (_connectionState == ConnectionStateEnum.connected) {
        fetchStatus();
      }
    });
  }

  void disconnect() {
    _wsSubscription?.cancel();
    _wsChannel?.sink.close();
    _wsChannel = null;
    _heartbeatTimer?.cancel();
    _reconnectTimer?.cancel();
    _fallbackPollTimer?.cancel();
    _connectionState = ConnectionStateEnum.disconnected;
    notifyListeners();
  }

  void _setupWebSocket() {
    _wsSubscription?.cancel();
    _wsChannel?.sink.close();

    final wsUrl = Uri.parse('ws://$_espIp:$_port/ws');
    try {
      _wsChannel = WebSocketChannel.connect(wsUrl);
      _wsSubscription = _wsChannel!.stream.listen(
        (data) {
          _onWsMessage(data);
        },
        onError: (error) {
          debugPrint('WS error: $error');
          _onWsDisconnected();
        },
        onDone: () {
          debugPrint('WS closed');
          _onWsDisconnected();
        },
        cancelOnError: true,
      );

      // Connected!
      _connectionState = ConnectionStateEnum.connected;
      _lastErrorMessage = '';
      notifyListeners();

      // Start ping heartbeat
      _heartbeatTimer?.cancel();
      _heartbeatTimer = Timer.periodic(const Duration(seconds: 5), (_) {
        sendWsCommand({'cmd': 'ping'});
      });
    } catch (e) {
      debugPrint('WS Connect Exception: $e');
      _onWsDisconnected();
    }
  }

  void _onWsDisconnected() {
    _connectionState = ConnectionStateEnum.disconnected;
    notifyListeners();

    // Schedule auto-reconnect
    _reconnectTimer?.cancel();
    _reconnectTimer = Timer(const Duration(seconds: 4), () {
      if (_connectionState != ConnectionStateEnum.connected) {
        connect();
      }
    });
  }

  void _onWsMessage(dynamic rawData) {
    try {
      final jsonMap = jsonDecode(rawData.toString());
      if (jsonMap is Map<String, dynamic>) {
        if (jsonMap.containsKey('title') || jsonMap.containsKey('isPlaying')) {
          _playerState = PlayerStateModel.fromJson(jsonMap);
          if (_connectionState != ConnectionStateEnum.connected) {
            _connectionState = ConnectionStateEnum.connected;
          }
          notifyListeners();
        } else if (jsonMap.containsKey('files')) {
          final List list = jsonMap['files'] ?? [];
          _sdFiles = list.map((item) => SdFile.fromJson(item)).toList();
          notifyListeners();
        }
      }
    } catch (e) {
      debugPrint('Error parsing WS message: $e');
    }
  }

  // HTTP API Calls
  Future<void> fetchStatus() async {
    try {
      final uri = Uri.parse('http://$_espIp:$_port/api/status');
      final res = await http.get(uri).timeout(const Duration(seconds: 3));
      if (res.statusCode == 200) {
        final data = jsonDecode(res.body);
        _playerState = PlayerStateModel.fromJson(data);
        _connectionState = ConnectionStateEnum.connected;
        _lastErrorMessage = '';
        notifyListeners();
      }
    } catch (e) {
      // WS might still be working or offline
    }
  }

  Future<void> fetchSdFiles() async {
    _isLoadingFiles = true;
    notifyListeners();

    try {
      final uri = Uri.parse('http://$_espIp:$_port/api/files');
      final res = await http.get(uri).timeout(const Duration(seconds: 5));
      if (res.statusCode == 200) {
        final data = jsonDecode(res.body);
        final List list = data['files'] ?? [];
        _sdFiles = list.map((item) => SdFile.fromJson(item)).toList();
        _connectionState = ConnectionStateEnum.connected;
      }
    } catch (e) {
      debugPrint('Error fetching SD files: $e');
      _lastErrorMessage = 'Failed to load SD files';
    } finally {
      _isLoadingFiles = false;
      notifyListeners();
    }
  }

  // Playback Control Commands
  Future<void> togglePlayPause() async {
    _playerState = _playerState.copyWith(isPlaying: !_playerState.isPlaying);
    notifyListeners();
    await _sendCommand('toggle');
  }

  Future<void> play() async {
    _playerState = _playerState.copyWith(isPlaying: true);
    notifyListeners();
    await _sendCommand('play');
  }

  Future<void> pause() async {
    _playerState = _playerState.copyWith(isPlaying: false);
    notifyListeners();
    await _sendCommand('pause');
  }

  Future<void> nextTrack() async {
    await _sendCommand('next');
  }

  Future<void> prevTrack() async {
    await _sendCommand('prev');
  }

  Future<void> setVolume(int volume) async {
    final v = volume.clamp(0, 100);
    _playerState = _playerState.copyWith(volume: v);
    notifyListeners();
    await _sendCommand('volume', value: v.toString());
  }

  Future<void> seekTo(int seconds) async {
    _playerState = _playerState.copyWith(currentTime: seconds);
    notifyListeners();
    await _sendCommand('seek', value: seconds.toString());
  }

  Future<void> playFile(String path) async {
    await _sendCommand('select', extraParam: '&path=${Uri.encodeComponent(path)}');
  }

  Future<void> _sendCommand(String action, {String? value, String? extraParam}) async {
    // Try sending over WebSocket first for sub-10ms latency
    bool sentWs = false;
    if (_wsChannel != null && _connectionState == ConnectionStateEnum.connected) {
      final Map<String, dynamic> msg = {'cmd': action};
      if (value != null) msg['value'] = value;
      if (extraParam != null && extraParam.startsWith('&path=')) {
        msg['path'] = Uri.decodeComponent(extraParam.replaceFirst('&path=', ''));
      }
      sentWs = sendWsCommand(msg);
    }

    // Always fallback or supplement with HTTP POST
    try {
      var urlStr = 'http://$_espIp:$_port/api/control?action=$action';
      if (value != null) urlStr += '&val=$value';
      if (extraParam != null) urlStr += extraParam;

      await http.get(Uri.parse(urlStr)).timeout(const Duration(milliseconds: 1500));
    } catch (e) {
      if (!sentWs) {
        debugPrint('Send command error: $e');
      }
    }
  }

  bool sendWsCommand(Map<String, dynamic> cmd) {
    try {
      if (_wsChannel != null) {
        _wsChannel!.sink.add(jsonEncode(cmd));
        return true;
      }
    } catch (_) {}
    return false;
  }

  @override
  void dispose() {
    disconnect();
    super.dispose();
  }
}
