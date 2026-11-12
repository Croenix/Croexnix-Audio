import 'package:equatable/equatable.dart';

enum AudioSource {
  none,
  bluetooth,
  sdCard,
  fmRadio,
  webStream,
}

class DeviceState extends Equatable {
  // Audio state
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
  final String macAddress;
  final int connectedClients;

  // SD Card state
  final bool sdCardMounted;
  final int sdCardSize; // bytes
  final int sdCardUsed; // bytes
  final int sdTotalTracks;

  // Bluetooth state
  final bool bluetoothConnected;
  final String bluetoothDevice;

  // System & Hardware state
  final String deviceName;
  final String firmwareVersion;
  final String compileDate;
  final String compileTime;
  final String chipModel;
  final int chipCores;
  final int cpuFreqMHz;
  final String sdkVersion;
  final int uptime; // seconds
  final int cpuUsageCore0; // 0-100
  final int cpuUsageCore1; // 0-100
  final int freeHeap; // bytes
  final int totalHeap; // bytes
  final int minFreeHeap; // bytes
  final int heapUsagePercent; // 0-100%
  final int flashSize; // bytes

  // EQ Preset & Hardware Tone
  final int eqPreset; // 0=Flat, 1=Rock, 2=Pop, etc.
  final int eqBass;   // -10 to +10
  final int eqMid;    // -10 to +10
  final int eqTreble; // -10 to +10

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
    this.macAddress = '',
    this.connectedClients = 0,
    this.sdCardMounted = false,
    this.sdCardSize = 0,
    this.sdCardUsed = 0,
    this.sdTotalTracks = 0,
    this.bluetoothConnected = false,
    this.bluetoothDevice = 'Croenix',
    this.deviceName = 'Croenix',
    this.firmwareVersion = '1.0.0',
    this.compileDate = '',
    this.compileTime = '',
    this.chipModel = 'ESP32',
    this.chipCores = 2,
    this.cpuFreqMHz = 240,
    this.sdkVersion = '',
    this.uptime = 0,
    this.cpuUsageCore0 = 0,
    this.cpuUsageCore1 = 0,
    this.freeHeap = 0,
    this.totalHeap = 0,
    this.minFreeHeap = 0,
    this.heapUsagePercent = 0,
    this.flashSize = 0,
    this.eqPreset = 0,
    this.eqBass = 0,
    this.eqMid = 0,
    this.eqTreble = 0,
  });

  // Human-readable formatting helpers
  String get formattedUptime {
    if (uptime <= 0) return '0s';
    final days = uptime ~/ 86400;
    final hours = (uptime % 86400) ~/ 3600;
    final minutes = (uptime % 3600) ~/ 60;
    final seconds = uptime % 60;

    if (days > 0) {
      return '${days}d ${hours.toString().padLeft(2, '0')}h ${minutes.toString().padLeft(2, '0')}m';
    } else if (hours > 0) {
      return '${hours}h ${minutes.toString().padLeft(2, '0')}m ${seconds.toString().padLeft(2, '0')}s';
    } else if (minutes > 0) {
      return '${minutes}m ${seconds.toString().padLeft(2, '0')}s';
    } else {
      return '${seconds}s';
    }
  }

  String get formattedFreeHeap {
    if (freeHeap <= 0) return '0 KB';
    return '${(freeHeap / 1024).toStringAsFixed(1)} KB';
  }

  String get formattedTotalHeap {
    if (totalHeap <= 0) return '320 KB';
    return '${(totalHeap / 1024).toStringAsFixed(1)} KB';
  }

  String get formattedFlashSize {
    if (flashSize <= 0) return '4 MB';
    final mb = flashSize / (1024 * 1024);
    return '${mb.toStringAsFixed(0)} MB';
  }

  String get formattedCpuUsage {
    final avg = (cpuUsageCore0 + cpuUsageCore1) ~/ 2;
    return '$avg%';
  }

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
    String? macAddress,
    int? connectedClients,
    bool? sdCardMounted,
    int? sdCardSize,
    int? sdCardUsed,
    int? sdCardMountedState,
    int? sdTotalTracks,
    bool? bluetoothConnected,
    String? bluetoothDevice,
    String? deviceName,
    String? firmwareVersion,
    String? compileDate,
    String? compileTime,
    String? chipModel,
    int? chipCores,
    int? cpuFreqMHz,
    String? sdkVersion,
    int? uptime,
    int? cpuUsageCore0,
    int? cpuUsageCore1,
    int? freeHeap,
    int? totalHeap,
    int? minFreeHeap,
    int? heapUsagePercent,
    int? flashSize,
    int? eqPreset,
    int? eqBass,
    int? eqMid,
    int? eqTreble,
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
      macAddress: macAddress ?? this.macAddress,
      connectedClients: connectedClients ?? this.connectedClients,
      sdCardMounted: sdCardMounted ?? this.sdCardMounted,
      sdCardSize: sdCardSize ?? this.sdCardSize,
      sdCardUsed: sdCardUsed ?? this.sdCardUsed,
      sdTotalTracks: sdTotalTracks ?? this.sdTotalTracks,
      bluetoothConnected: bluetoothConnected ?? this.bluetoothConnected,
      bluetoothDevice: bluetoothDevice ?? this.bluetoothDevice,
      deviceName: deviceName ?? this.deviceName,
      firmwareVersion: firmwareVersion ?? this.firmwareVersion,
      compileDate: compileDate ?? this.compileDate,
      compileTime: compileTime ?? this.compileTime,
      chipModel: chipModel ?? this.chipModel,
      chipCores: chipCores ?? this.chipCores,
      cpuFreqMHz: cpuFreqMHz ?? this.cpuFreqMHz,
      sdkVersion: sdkVersion ?? this.sdkVersion,
      uptime: uptime ?? this.uptime,
      cpuUsageCore0: cpuUsageCore0 ?? this.cpuUsageCore0,
      cpuUsageCore1: cpuUsageCore1 ?? this.cpuUsageCore1,
      freeHeap: freeHeap ?? this.freeHeap,
      totalHeap: totalHeap ?? this.totalHeap,
      minFreeHeap: minFreeHeap ?? this.minFreeHeap,
      heapUsagePercent: heapUsagePercent ?? this.heapUsagePercent,
      flashSize: flashSize ?? this.flashSize,
      eqPreset: eqPreset ?? this.eqPreset,
      eqBass: eqBass ?? this.eqBass,
      eqMid: eqMid ?? this.eqMid,
      eqTreble: eqTreble ?? this.eqTreble,
    );
  }

  factory DeviceState.fromJson(Map<String, dynamic> json, {DeviceState? previous}) {
    final prev = previous ?? const DeviceState();

    return DeviceState(
      currentSource: json.containsKey('source')
          ? AudioSource.values[(json['source'] as num).toInt().clamp(0, AudioSource.values.length - 1)]
          : prev.currentSource,
      volume: json.containsKey('volume') ? (json['volume'] as num).toInt() : prev.volume,
      isPlaying: json.containsKey('playing') ? (json['playing'] as bool) : prev.isPlaying,
      isMuted: json.containsKey('muted') ? (json['muted'] as bool) : prev.isMuted,
      trackTitle: json['title'] ?? prev.trackTitle,
      trackArtist: json['artist'] ?? prev.trackArtist,
      trackAlbum: json['album'] ?? prev.trackAlbum,
      trackDuration: json.containsKey('duration') ? (json['duration'] as num).toInt() : prev.trackDuration,
      trackPosition: json.containsKey('position') ? (json['position'] as num).toInt() : prev.trackPosition,
      trackBitrate: json.containsKey('bitrate') ? (json['bitrate'] as num).toInt() : prev.trackBitrate,
      fmFrequency: json.containsKey('fm_frequency')
          ? (json['fm_frequency'] as num).toDouble()
          : prev.fmFrequency,
      fmSignalStrength: json.containsKey('fm_rssi')
          ? (json['fm_rssi'] as num).toInt()
          : prev.fmSignalStrength,
      fmStationName: json['fm_station'] ?? prev.fmStationName,
      wifiConnected: json.containsKey('wifi_connected') ? (json['wifi_connected'] as bool) : prev.wifiConnected,
      apMode: json.containsKey('ap_mode') ? (json['ap_mode'] as bool) : prev.apMode,
      wifiSSID: json['ssid'] ?? prev.wifiSSID,
      wifiRSSI: json.containsKey('rssi') ? (json['rssi'] as num).toInt() : prev.wifiRSSI,
      localIP: json['local_ip'] ?? prev.localIP,
      macAddress: json['mac'] ?? prev.macAddress,
      connectedClients: json.containsKey('clients') ? (json['clients'] as num).toInt() : prev.connectedClients,
      sdCardMounted: json.containsKey('sd_mounted') ? (json['sd_mounted'] as bool) : prev.sdCardMounted,
      sdCardSize: json.containsKey('sd_size') ? (json['sd_size'] as num).toInt() * 1024 : prev.sdCardSize,
      sdCardUsed: json.containsKey('sd_used') ? (json['sd_used'] as num).toInt() * 1024 : prev.sdCardUsed,
      sdTotalTracks: json.containsKey('sd_total_tracks')
          ? (json['sd_total_tracks'] as num).toInt()
          : (json.containsKey('sd_tracks') ? (json['sd_tracks'] as num).toInt() : prev.sdTotalTracks),
      bluetoothConnected: json.containsKey('bt_connected') ? (json['bt_connected'] as bool) : prev.bluetoothConnected,
      bluetoothDevice: json['bt_device'] ?? prev.bluetoothDevice,
      deviceName: json['device_name'] ?? prev.deviceName,
      firmwareVersion: json['firmware_version'] ?? prev.firmwareVersion,
      compileDate: json['compile_date'] ?? prev.compileDate,
      compileTime: json['compile_time'] ?? prev.compileTime,
      chipModel: json['chip_model'] ?? prev.chipModel,
      chipCores: json.containsKey('chip_cores') ? (json['chip_cores'] as num).toInt() : prev.chipCores,
      cpuFreqMHz: json.containsKey('cpu_freq') ? (json['cpu_freq'] as num).toInt() : prev.cpuFreqMHz,
      sdkVersion: json['sdk_version'] ?? prev.sdkVersion,
      uptime: json.containsKey('uptime') ? (json['uptime'] as num).toInt() : prev.uptime,
      cpuUsageCore0: json.containsKey('cpu0') ? (json['cpu0'] as num).toInt() : prev.cpuUsageCore0,
      cpuUsageCore1: json.containsKey('cpu1') ? (json['cpu1'] as num).toInt() : prev.cpuUsageCore1,
      freeHeap: json.containsKey('free_heap') ? (json['free_heap'] as num).toInt() : prev.freeHeap,
      totalHeap: json.containsKey('total_heap') ? (json['total_heap'] as num).toInt() : prev.totalHeap,
      minFreeHeap: json.containsKey('min_free_heap') ? (json['min_free_heap'] as num).toInt() : prev.minFreeHeap,
      heapUsagePercent: json.containsKey('heap_usage')
          ? (json['heap_usage'] as num).toInt()
          : (json.containsKey('total_heap') && (json['total_heap'] as num) > 0 && json.containsKey('free_heap'))
              ? (((json['total_heap'] as num).toInt() - (json['free_heap'] as num).toInt()) * 100 ~/ (json['total_heap'] as num).toInt())
              : prev.heapUsagePercent,
      flashSize: json.containsKey('flash_size') ? (json['flash_size'] as num).toInt() : prev.flashSize,
      eqPreset: json.containsKey('eq_preset') ? (json['eq_preset'] as num).toInt() : prev.eqPreset,
      eqBass: json.containsKey('eq_bass') ? (json['eq_bass'] as num).toInt() : prev.eqBass,
      eqMid: json.containsKey('eq_mid') ? (json['eq_mid'] as num).toInt() : prev.eqMid,
      eqTreble: json.containsKey('eq_treble') ? (json['eq_treble'] as num).toInt() : prev.eqTreble,
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
      'device_name': deviceName,
      'firmware_version': firmwareVersion,
      'uptime': uptime,
      'free_heap': freeHeap,
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
        uptime,
        freeHeap,
        heapUsagePercent,
        localIP,
        wifiSSID,
        deviceName,
        firmwareVersion,
      ];
}
