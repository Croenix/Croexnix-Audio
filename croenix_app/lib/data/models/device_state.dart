class DeviceState {
  final DeviceInfo device;
  final ConnectionInfo connection;
  final AudioInfo audio;
  final StorageInfo storage;
  final SystemInfo system;

  DeviceState({
    required this.device,
    required this.connection,
    required this.audio,
    required this.storage,
    required this.system,
  });

  factory DeviceState.fromJson(Map<String, dynamic> json) {
    return DeviceState(
      device: DeviceInfo.fromJson(json['device'] ?? {}),
      connection: ConnectionInfo.fromJson(json['connection'] ?? {}),
      audio: AudioInfo.fromJson(json['audio'] ?? {}),
      storage: StorageInfo.fromJson(json['storage'] ?? {}),
      system: SystemInfo.fromJson(json['system'] ?? {}),
    );
  }
}

class DeviceInfo {
  final String name;
  final String firmwareVersion;
  final int uptimeSeconds;

  DeviceInfo({
    required this.name,
    required this.firmwareVersion,
    required this.uptimeSeconds,
  });

  factory DeviceInfo.fromJson(Map<String, dynamic> json) {
    return DeviceInfo(
      name: json['name'] ?? 'Unknown',
      firmwareVersion: json['firmware_version'] ?? '0.0.0',
      uptimeSeconds: json['uptime_seconds'] ?? 0,
    );
  }

  String get uptimeFormatted {
    final duration = Duration(seconds: uptimeSeconds);
    final hours = duration.inHours;
    final minutes = duration.inMinutes.remainder(60);
    return '${hours}h ${minutes}m';
  }
}

class ConnectionInfo {
  final bool routerConnected;
  final String routerIp;
  final String routerSsid;
  final int routerRssi;
  final bool hotspotActive;
  final int hotspotClients;
  final String activeInterface;

  ConnectionInfo({
    required this.routerConnected,
    required this.routerIp,
    required this.routerSsid,
    required this.routerRssi,
    required this.hotspotActive,
    required this.hotspotClients,
    required this.activeInterface,
  });

  factory ConnectionInfo.fromJson(Map<String, dynamic> json) {
    return ConnectionInfo(
      routerConnected: json['router_connected'] ?? false,
      routerIp: json['router_ip'] ?? '',
      routerSsid: json['router_ssid'] ?? '',
      routerRssi: json['router_rssi'] ?? 0,
      hotspotActive: json['hotspot_active'] ?? false,
      hotspotClients: json['hotspot_clients'] ?? 0,
      activeInterface: json['active_interface'] ?? 'none',
    );
  }

  String get signalStrength {
    if (routerRssi >= -50) return 'Excellent';
    if (routerRssi >= -60) return 'Good';
    if (routerRssi >= -70) return 'Fair';
    return 'Weak';
  }
}

class AudioInfo {
  final String source;
  final String status;
  final int volume;
  final bool muted;
  final String codec;
  final int sampleRate;
  final int bitDepth;
  final int channels;

  AudioInfo({
    required this.source,
    required this.status,
    required this.volume,
    required this.muted,
    required this.codec,
    required this.sampleRate,
    required this.bitDepth,
    required this.channels,
  });

  factory AudioInfo.fromJson(Map<String, dynamic> json) {
    return AudioInfo(
      source: json['source'] ?? 'none',
      status: json['status'] ?? 'stopped',
      volume: json['volume'] ?? 50,
      muted: json['muted'] ?? false,
      codec: json['codec'] ?? '',
      sampleRate: json['sample_rate'] ?? 0,
      bitDepth: json['bit_depth'] ?? 0,
      channels: json['channels'] ?? 0,
    );
  }

  bool get isPlaying => status == 'playing';
  bool get hasActiveSource => source != 'none';
}

class StorageInfo {
  final bool sdInserted;
  final int totalBytes;
  final int usedBytes;
  final int freeBytes;

  StorageInfo({
    required this.sdInserted,
    required this.totalBytes,
    required this.usedBytes,
    required this.freeBytes,
  });

  factory StorageInfo.fromJson(Map<String, dynamic> json) {
    return StorageInfo(
      sdInserted: json['sd_inserted'] ?? false,
      totalBytes: json['total_bytes'] ?? 0,
      usedBytes: json['used_bytes'] ?? 0,
      freeBytes: json['free_bytes'] ?? 0,
    );
  }
}

class SystemInfo {
  final int freeHeap;
  final int totalHeap;
  final int minFreeHeap;
  final String restartReason;

  SystemInfo({
    required this.freeHeap,
    required this.totalHeap,
    required this.minFreeHeap,
    required this.restartReason,
  });

  factory SystemInfo.fromJson(Map<String, dynamic> json) {
    return SystemInfo(
      freeHeap: json['free_heap'] ?? 0,
      totalHeap: json['total_heap'] ?? 0,
      minFreeHeap: json['min_free_heap'] ?? 0,
      restartReason: json['restart_reason'] ?? 'Unknown',
    );
  }

  int get heapUsagePercent {
    if (totalHeap == 0) return 0;
    return ((totalHeap - freeHeap) * 100 / totalHeap).round();
  }
}
