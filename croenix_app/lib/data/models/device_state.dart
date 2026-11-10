import 'package:equatable/equatable.dart';

enum AudioSource {
  none,
  bluetooth,
  sdCard,
  fmRadio,
  webStream,
}

class DeviceState extends Equatable {
  final AudioSource currentSource;
  final int volume;
  final bool isPlaying;
  final bool isMuted;

  // Track metadata
  final String trackTitle;
  final String trackArtist;
  final String trackAlbum;
  final int trackDuration; // seconds
  final int trackPosition; // seconds
  final int trackBitrate; // kbps

  // FM Radio state
  final double fmFrequency; // MHz
  final int fmSignalStrength; // 0-100
  final String fmStationName;

  // Network state
  final bool wifiConnected;
  final bool apMode;
  final String wifiSSID;
  final int wifiRSSI; // dBm
  final String localIP;
  final int connectedClients;

  // SD Card state
  final bool sdCardMounted;
  final int sdCardSize; // bytes
  final int sdCardUsed; // bytes

  // Bluetooth state
  final bool bluetoothConnected;
  final String bluetoothDevice;

  // System state
  final int uptime; // seconds
  final int cpuUsageCore0; // 0-100
  final int cpuUsageCore1; // 0-100
  final int freeHeap; // bytes

  // EQ Preset
  final int eqPreset; // 0=Flat, 1=Rock, 2=Pop, etc.

  const DeviceState({
    this.currentSource = AudioSource.none,
    this.volume = 50,
    this.isPlaying = false,
    this.isMuted = false,
    this.trackTitle = '',
    this.trackArtist = '',
    this.trackAlbum = '',
    this.trackDuration = 0,
    this.trackPosition = 0,
    this.trackBitrate = 0,
    this.fmFrequency = 88.0,
    this.fmSignalStrength = 0,
    this.fmStationName = '',
    this.wifiConnected = false,
    this.apMode = false,
    this.wifiSSID = '',
    this.wifiRSSI = 0,
    this.localIP = '',
    this.connectedClients = 0,
    this.sdCardMounted = false,
    this.sdCardSize = 0,
    this.sdCardUsed = 0,
    this.bluetoothConnected = false,
    this.bluetoothDevice = '',
    this.uptime = 0,
    this.cpuUsageCore0 = 0,
    this.cpuUsageCore1 = 0,
    this.freeHeap = 0,
    this.eqPreset = 0,
  });

  DeviceState copyWith({
    AudioSource? currentSource,
    int? volume,
    bool? isPlaying,
    bool? isMuted,
    String? trackTitle,
    String? trackArtist,
    String? trackAlbum,
    int? trackDuration,
    int? trackPosition,
    int? trackBitrate,
    double? fmFrequency,
    int? fmSignalStrength,
    String? fmStationName,
    bool? wifiConnected,
    bool? apMode,
    String? wifiSSID,
    int? wifiRSSI,
    String? localIP,
    int? connectedClients,
    bool? sdCardMounted,
    int? sdCardSize,
    int? sdCardUsed,
    bool? bluetoothConnected,
    String? bluetoothDevice,
    int? uptime,
    int? cpuUsageCore0,
    int? cpuUsageCore1,
    int? freeHeap,
    int? eqPreset,
  }) {
    return DeviceState(
      currentSource: currentSource ?? this.currentSource,
      volume: volume ?? this.volume,
      isPlaying: isPlaying ?? this.isPlaying,
      isMuted: isMuted ?? this.isMuted,
      trackTitle: trackTitle ?? this.trackTitle,
      trackArtist: trackArtist ?? this.trackArtist,
      trackAlbum: trackAlbum ?? this.trackAlbum,
      trackDuration: trackDuration ?? this.trackDuration,
      trackPosition: trackPosition ?? this.trackPosition,
      trackBitrate: trackBitrate ?? this.trackBitrate,
      fmFrequency: fmFrequency ?? this.fmFrequency,
      fmSignalStrength: fmSignalStrength ?? this.fmSignalStrength,
      fmStationName: fmStationName ?? this.fmStationName,
      wifiConnected: wifiConnected ?? this.wifiConnected,
      apMode: apMode ?? this.apMode,
      wifiSSID: wifiSSID ?? this.wifiSSID,
      wifiRSSI: wifiRSSI ?? this.wifiRSSI,
      localIP: localIP ?? this.localIP,
      connectedClients: connectedClients ?? this.connectedClients,
      sdCardMounted: sdCardMounted ?? this.sdCardMounted,
      sdCardSize: sdCardSize ?? this.sdCardSize,
      sdCardUsed: sdCardUsed ?? this.sdCardUsed,
      bluetoothConnected: bluetoothConnected ?? this.bluetoothConnected,
      bluetoothDevice: bluetoothDevice ?? this.bluetoothDevice,
      uptime: uptime ?? this.uptime,
      cpuUsageCore0: cpuUsageCore0 ?? this.cpuUsageCore0,
      cpuUsageCore1: cpuUsageCore1 ?? this.cpuUsageCore1,
      freeHeap: freeHeap ?? this.freeHeap,
      eqPreset: eqPreset ?? this.eqPreset,
    );
  }

  factory DeviceState.fromJson(Map<String, dynamic> json) {
    return DeviceState(
      currentSource: AudioSource.values[json['source'] ?? 0],
      volume: json['volume'] ?? 50,
      isPlaying: json['playing'] ?? false,
      trackTitle: json['title'] ?? '',
      trackArtist: json['artist'] ?? '',
      trackAlbum: json['album'] ?? '',
      trackDuration: json['duration'] ?? 0,
      trackPosition: json['position'] ?? 0,
      trackBitrate: json['bitrate'] ?? 0,
      eqPreset: json['eq_preset'] ?? 0,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'source': currentSource.index,
      'volume': volume,
      'playing': isPlaying,
      'title': trackTitle,
      'artist': trackArtist,
      'album': trackAlbum,
      'duration': trackDuration,
      'position': trackPosition,
      'bitrate': trackBitrate,
      'eq_preset': eqPreset,
    };
  }

  @override
  List<Object?> get props => [
        currentSource,
        volume,
        isPlaying,
        trackTitle,
        trackArtist,
        trackPosition,
        trackDuration,
        wifiConnected,
        fmFrequency,
      ];
}
