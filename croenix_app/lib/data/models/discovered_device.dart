class DiscoveredDevice {
  final String name;
  final String hostname;
  final String ipAddress;
  final int port;

  const DiscoveredDevice({
    required this.name,
    required this.hostname,
    required this.ipAddress,
    required this.port,
  });

  String get displayName => name.replaceAll('.local', '');

  @override
  String toString() => '$displayName ($ipAddress:$port)';
}
