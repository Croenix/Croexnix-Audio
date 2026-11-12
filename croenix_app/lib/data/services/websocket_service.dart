import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:web_socket_channel/status.dart' as ws_status;
import '../models/device_state.dart';
import 'discovery_service.dart';
import 'ble_service.dart';

enum ConnectionStatus {
  disconnected,
  connecting,
  connected,
  error,
}

class WebSocketService {
  // Singleton pattern
  static final WebSocketService _instance = WebSocketService._internal();
  factory WebSocketService() => _instance;
  WebSocketService._internal();

  WebSocketChannel? _channel;
  StreamSubscription? _subscription;

  DeviceState _currentState = const DeviceState();
  DeviceState get currentState => _currentState;

  final _stateController = StreamController<DeviceState>.broadcast();
  Stream<DeviceState> get stateStream => _stateController.stream;

  ConnectionStatus _status = ConnectionStatus.disconnected;
  ConnectionStatus get status => _status;
  bool get isConnected => _status == ConnectionStatus.connected;

  final _statusController = StreamController<ConnectionStatus>.broadcast();
  Stream<ConnectionStatus> get statusStream => _statusController.stream;

  // Backwards compatibility stream for bool connection
  final _connectionController = StreamController<bool>.broadcast();
  Stream<bool> get connectionStream => _connectionController.stream;

  // Track list & File list streams for SD Card explorer
  final _trackListController = StreamController<List<Map<String, dynamic>>>.broadcast();
  Stream<List<Map<String, dynamic>>> get trackListStream => _trackListController.stream;
  List<Map<String, dynamic>> _lastTrackList = [];
  List<Map<String, dynamic>> get lastTrackList => _lastTrackList;

  final _fileListController = StreamController<List<Map<String, dynamic>>>.broadcast();
  Stream<List<Map<String, dynamic>>> get fileListStream => _fileListController.stream;

  String? _deviceIP;
  String? get deviceIP => _deviceIP;
  int _port = 8080;
  int get port => _port;

  Timer? _reconnectTimer;
  Timer? _pingTimer;
  int _reconnectAttempts = 0;
  bool _intentionalDisconnect = false;

  static const String _prefLastIPKey = 'croenix_last_ip';
  static const String defaultApIP = '192.168.4.1';

  void _setStatus(ConnectionStatus newStatus) {
    if (_status != newStatus) {
      _status = newStatus;
      _statusController.add(newStatus);
      _connectionController.add(newStatus == ConnectionStatus.connected);
      print('[WebSocket] Connection status changed to: $newStatus');
    }
  }

  // Fast non-intrusive TCP socket probe (does not trigger reconnect loops)
  Future<bool> _probeHost(String ip, int port, Duration timeout) async {
    try {
      final socket = await Socket.connect(ip, port, timeout: timeout);
      await socket.close();
      return true;
    } catch (_) {
      return false;
    }
  }

  // Auto-connect on startup: probes Bluetooth BLE first (zero hotspot), then LAN Wi-Fi
  Future<bool> autoConnect() async {
    // 1. Initiate low-power BLE nearby discovery immediately
    BleService().startAutoScanAndConnect();

    if (isConnected) return true;

    try {
      final prefs = await SharedPreferences.getInstance();
      final lastIP = prefs.getString(_prefLastIPKey);

      final candidateIPs = <String>[];
      if (lastIP != null && lastIP.isNotEmpty && lastIP != defaultApIP) {
        candidateIPs.add(lastIP);
      }

      print('[WebSocket] Probing candidates: $candidateIPs');
      for (final ip in candidateIPs) {
        final isUp = await _probeHost(ip, _port, const Duration(seconds: 1));
        if (isUp) {
          print('[WebSocket] Host $ip responded! Connecting...');
          final success = await connect(ip, timeout: const Duration(seconds: 3), autoReconnect: true);
          if (success) return true;
        }
      }

      // Try mDNS discovery
      print('[WebSocket] Running mDNS discovery for Croenix...');
      final discovery = DiscoveryService();
      discovery.startDiscovery();
      await Future.delayed(const Duration(seconds: 2));
      final devices = discovery.devices;
      discovery.stopDiscovery();

      if (devices.isNotEmpty) {
        final target = devices.first;
        print('[WebSocket] Found mDNS device: ${target.ipAddress}');
        return await connect(target.ipAddress, port: target.port, autoReconnect: true);
      }
    } catch (e) {
      print('[WebSocket] Auto-connect error: $e');
    }

    _setStatus(ConnectionStatus.disconnected);
    return false;
  }

  // Connect to ESP32 device with timeout verification
  Future<bool> connect(
    String ip, {
    int port = 8080,
    Duration timeout = const Duration(seconds: 4),
    bool autoReconnect = true,
  }) async {
    if (_status == ConnectionStatus.connecting) {
      print('[WebSocket] Already connecting...');
      return false;
    }

    _intentionalDisconnect = false;
    _deviceIP = ip.trim();
    _port = port;

    _setStatus(ConnectionStatus.connecting);
    print('[WebSocket] Connecting to ws://$_deviceIP:$_port...');

    try {
      // Step 1: Pre-flight TCP probe with short timeout to verify host is up
      final socket = await Socket.connect(_deviceIP!, _port, timeout: timeout);
      await socket.close();

      // Step 2: Establish WebSocket connection
      final uri = Uri.parse('ws://$_deviceIP:$_port');
      _channel = WebSocketChannel.connect(uri);

      // Listen to incoming messages
      _subscription?.cancel();
      _subscription = _channel!.stream.listen(
        _handleMessage,
        onError: _handleError,
        onDone: _handleDisconnect,
        cancelOnError: false,
      );

      _setStatus(ConnectionStatus.connected);
      _reconnectAttempts = 0;

      // Save successful IP
      final prefs = await SharedPreferences.getInstance();
      await prefs.setString(_prefLastIPKey, _deviceIP!);

      // Immediately query full status from device
      requestStatus();

      // Start periodic status requests
      _startHeartbeat();

      print('[WebSocket] Connected successfully to $_deviceIP:$_port');
      return true;
    } catch (e) {
      print('[WebSocket] Connection failed to $_deviceIP:$_port: $e');
      _channel = null;
      _setStatus(ConnectionStatus.error);
      if (autoReconnect) {
        _scheduleReconnect();
      }
      return false;
    }
  }

  // Disconnect from device
  void disconnect() {
    print('[WebSocket] Disconnecting intentionally...');
    _intentionalDisconnect = true;
    _reconnectTimer?.cancel();
    _pingTimer?.cancel();
    _subscription?.cancel();
    _subscription = null;
    try {
      _channel?.sink.close(ws_status.goingAway);
    } catch (_) {}
    _channel = null;
    _setStatus(ConnectionStatus.disconnected);
  }

  void _scheduleReconnect() {
    if (_intentionalDisconnect || _deviceIP == null) return;

    _reconnectTimer?.cancel();
    _reconnectAttempts++;
    final delaySeconds = (_reconnectAttempts * 3).clamp(3, 15);

    print('[WebSocket] Scheduling reconnect attempt $_reconnectAttempts in ${delaySeconds}s...');
    _reconnectTimer = Timer(Duration(seconds: delaySeconds), () {
      if (!isConnected && !_intentionalDisconnect && _deviceIP != null) {
        connect(_deviceIP!, port: _port);
      }
    });
  }

  void _startHeartbeat() {
    _pingTimer?.cancel();
    _pingTimer = Timer.periodic(const Duration(seconds: 2), (timer) {
      if (isConnected) {
        requestStatus();
      } else {
        timer.cancel();
      }
    });
  }

  // Send command to ESP32 (dual-routed via Bluetooth BLE and WebSocket)
  void sendCommand(String command, [Map<String, dynamic>? params]) {
    final payload = {
      'command': command,
      if (params != null) ...params,
    };
    final jsonString = jsonEncode(payload);

    // 1. Dispatch over Bluetooth BLE if connected
    BleService().sendCommand(jsonString);

    // 2. Dispatch over local Wi-Fi WebSocket if connected
    if (isConnected && _channel != null) {
      try {
        _channel!.sink.add(jsonString);
        print('[WebSocket] Sent: $command');
      } catch (e) {
        print('[WebSocket] Error sending command: $e');
      }
    }
  }

  // Request fresh state & system info from ESP32
  void requestStatus() {
    sendCommand('get_status');
  }

  // === Audio Control Commands ===
  void play() => sendCommand('play');
  void pause() => sendCommand('pause');
  void stop() => sendCommand('stop');
  void nextTrack() => sendCommand('next_track');
  void previousTrack() => sendCommand('prev_track');

  void togglePlayPause() {
    if (_currentState.isPlaying) {
      pause();
    } else {
      play();
    }
  }

  void stepVolume(int delta) {
    int newVol = (_currentState.volume + delta).clamp(0, 100);
    setVolume(newVol);
  }

  int _preMuteVolume = 50;
  void toggleMute() {
    if (_currentState.volume > 0) {
      _preMuteVolume = _currentState.volume;
      setVolume(0);
    } else {
      setVolume(_preMuteVolume > 0 ? _preMuteVolume : 50);
    }
  }

  void setTftScreen(int screenIndex) {
    sendCommand('set_screen', {'value': screenIndex});
  }

  void sendEncoderStep(int direction) {
    sendCommand('encoder_step', {'dir': direction});
  }

  void sendEncoderClick() {
    sendCommand('encoder_click');
  }

  void setVolume(int volume) {
    // Optimistic local state update for zero UI latency
    _currentState = _currentState.copyWith(volume: volume);
    _stateController.add(_currentState);
    sendCommand('volume', {'value': volume});
  }

  void setSource(AudioSource source) {
    _currentState = _currentState.copyWith(currentSource: source);
    _stateController.add(_currentState);
    sendCommand('source', {'value': source.index});
  }

  void setEQPreset(int preset) {
    _currentState = _currentState.copyWith(eqPreset: preset);
    _stateController.add(_currentState);
    sendCommand('eq', {'value': preset});
  }

  void setHardwareTone(int bass, int mid, int treble) {
    _currentState = _currentState.copyWith(eqBass: bass, eqMid: mid, eqTreble: treble);
    _stateController.add(_currentState);
    sendCommand('set_tone', {'bass': bass, 'mid': mid, 'treble': treble});
  }

  // === Wi-Fi Configuration ===
  void configureWiFi(String ssid, String password) {
    sendCommand('wifi_config', {'ssid': ssid, 'password': password});
  }

  void scanNetworks() => sendCommand('scan_networks');

  // === File & Track Management ===
  void listFiles(String path) => sendCommand('file_list', {'path': path});

  void loadFile(String filepath) =>
      sendCommand('load_file', {'path': filepath});

  void requestTrackList() => sendCommand('get_tracks');

  void playSDIndex(int index) => sendCommand('play_index', {'index': index});

  // === FM Radio ===
  void setFMFrequency(double frequency) {
    _currentState = _currentState.copyWith(fmFrequency: frequency);
    _stateController.add(_currentState);
    sendCommand('fm_frequency', {'value': frequency});
  }

  void seekFMUp() => sendCommand('fm_seek', {'dir': 1});
  void seekFMDown() => sendCommand('fm_seek', {'dir': -1});
  void seekBestFMStation() => sendCommand('fm_seek_best');

  // === Web Streaming & SoundCloud ===
  void loadStreamURL(String url, {String? title, String? artist, String? album}) {
    sendCommand('stream_url', {
      'url': url,
      if (title != null) 'title': title,
      if (artist != null) 'artist': artist,
      if (album != null) 'album': album,
    });
  }

  // === SoundCloud & Web Stream Playlist Management ===
  List<Map<String, dynamic>> _soundCloudPlaylist = [];
  int _currentSoundCloudIndex = 0;
  bool _isAutoPlayingPlaylist = false;

  List<Map<String, dynamic>> get soundCloudPlaylist => _soundCloudPlaylist;
  int get currentSoundCloudIndex => _currentSoundCloudIndex;
  bool get isAutoPlayingPlaylist => _isAutoPlayingPlaylist;

  void playSoundCloudTrack({
    required String url,
    required String title,
    required String artist,
    String album = 'SoundCloud Master',
    int duration = 0,
  }) {
    // Optimistic local state update for zero UI lag
    _currentState = _currentState.copyWith(
      currentSource: AudioSource.webStream,
      trackTitle: title,
      trackArtist: artist,
      trackAlbum: album,
      isPlaying: true,
    );
    _stateController.add(_currentState);

    sendCommand('play_stream', {
      'url': url,
      'title': title,
      'artist': artist,
      'album': album,
      'duration': duration,
    });
  }

  void playSoundCloudPlaylist(List<Map<String, dynamic>> playlist, {int startIndex = 0}) {
    if (playlist.isEmpty) return;
    _soundCloudPlaylist = List.from(playlist);
    _currentSoundCloudIndex = startIndex.clamp(0, playlist.length - 1);
    _isAutoPlayingPlaylist = true;

    final track = _soundCloudPlaylist[_currentSoundCloudIndex];
    playSoundCloudTrack(
      url: track['url'] as String,
      title: track['title'] as String,
      artist: track['artist'] as String,
      album: (track['album'] as String?) ?? 'SoundCloud Playlist',
      duration: (track['duration'] as int?) ?? 0,
    );
  }

  void nextSoundCloudTrack() {
    if (_soundCloudPlaylist.isEmpty) {
      nextTrack();
      return;
    }
    _currentSoundCloudIndex = (_currentSoundCloudIndex + 1) % _soundCloudPlaylist.length;
    final track = _soundCloudPlaylist[_currentSoundCloudIndex];
    playSoundCloudTrack(
      url: track['url'] as String,
      title: track['title'] as String,
      artist: track['artist'] as String,
      album: (track['album'] as String?) ?? 'SoundCloud Playlist',
      duration: (track['duration'] as int?) ?? 0,
    );
  }

  void prevSoundCloudTrack() {
    if (_soundCloudPlaylist.isEmpty) {
      previousTrack();
      return;
    }
    _currentSoundCloudIndex = (_currentSoundCloudIndex - 1 + _soundCloudPlaylist.length) % _soundCloudPlaylist.length;
    final track = _soundCloudPlaylist[_currentSoundCloudIndex];
    playSoundCloudTrack(
      url: track['url'] as String,
      title: track['title'] as String,
      artist: track['artist'] as String,
      album: (track['album'] as String?) ?? 'SoundCloud Playlist',
      duration: (track['duration'] as int?) ?? 0,
    );
  }

  // === System Commands ===
  void rebootDevice() => sendCommand('reboot');

  // === Message Handling ===
  void _handleMessage(dynamic message) {
    try {
      final data = jsonDecode(message as String) as Map<String, dynamic>;
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
        case 'track_list':
          if (data.containsKey('tracks') && data['tracks'] is List) {
            _lastTrackList = List<Map<String, dynamic>>.from(data['tracks']);
            _trackListController.add(_lastTrackList);
            _currentState = _currentState.copyWith(sdTotalTracks: _lastTrackList.length);
            _stateController.add(_currentState);
          }
          break;
        case 'file_list':
          if (data.containsKey('files') && data['files'] is List) {
            _fileListController.add(List<Map<String, dynamic>>.from(data['files']));
          }
          break;
        default:
          // Also check if state fields are sent in a general message
          if (data.containsKey('source') || data.containsKey('uptime')) {
            _handleStateUpdate(data);
          }
      }
    } catch (e) {
      print('[WebSocket] Failed to parse message: $e');
    }
  }

  void _handleStateUpdate(Map<String, dynamic> data) {
    _currentState = DeviceState.fromJson(data, previous: _currentState);
    _stateController.add(_currentState);
  }

  void _handleMetadata(Map<String, dynamic> data) {
    _currentState = _currentState.copyWith(
      trackTitle: data['title'] as String?,
      trackArtist: data['artist'] as String?,
      trackAlbum: data['album'] as String?,
    );
    _stateController.add(_currentState);
  }

  void _handleNetworkInfo(Map<String, dynamic> data) {
    _currentState = _currentState.copyWith(
      wifiConnected: data['wifi_connected'] as bool?,
      apMode: data['ap_mode'] as bool?,
      wifiSSID: data['ssid'] as String?,
      wifiRSSI: (data['rssi'] as num?)?.toInt(),
      localIP: data['local_ip'] as String?,
      macAddress: data['mac'] as String?,
      connectedClients: (data['clients'] as num?)?.toInt(),
    );
    _stateController.add(_currentState);
  }

  void _handleError(dynamic error) {
    print('[WebSocket] Stream error: $error');
    _handleDisconnect();
  }

  void _handleDisconnect() {
    print('[WebSocket] Stream closed / disconnected');
    _channel = null;
    _setStatus(ConnectionStatus.disconnected);
    _scheduleReconnect();
  }
}
