class DiscoveredDevice {
  final String name;
  final String ip;
  final int port;
  final String? hostname;
  final DiscoveryMethod method;
  final String? bleDeviceId;

  DiscoveredDevice({
    required this.name,
    required this.ip,
    required this.port,
    this.hostname,
    required this.method,
    this.bleDeviceId,
  });

  bool get isBle => method == DiscoveryMethod.ble;
  String get baseUrl => 'http://$ip:$port';

  @override
  bool operator ==(Object other) {
    if (identical(this, other)) return true;
    return other is DiscoveredDevice &&
        ((isBle && other.isBle && other.bleDeviceId == bleDeviceId) ||
            (!isBle && !other.isBle && other.ip == ip && other.port == port));
  }

  @override
  int get hashCode => isBle ? (bleDeviceId?.hashCode ?? 0) : (ip.hashCode ^ port.hashCode);
}

enum DiscoveryMethod {
  mdns,
  hotspot,
  manual,
  saved,
  ble,
}
