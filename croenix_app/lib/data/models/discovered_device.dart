class DiscoveredDevice {
  final String name;
  final String ip;
  final int port;
  final String? hostname;
  final DiscoveryMethod method;

  DiscoveredDevice({
    required this.name,
    required this.ip,
    required this.port,
    this.hostname,
    required this.method,
  });

  String get baseUrl => 'http://$ip:$port';

  @override
  bool operator ==(Object other) {
    if (identical(this, other)) return true;
    return other is DiscoveredDevice && other.ip == ip && other.port == port;
  }

  @override
  int get hashCode => ip.hashCode ^ port.hashCode;
}

enum DiscoveryMethod {
  mdns,
  hotspot,
  manual,
  saved,
}
