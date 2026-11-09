class AppConstants {
  AppConstants._();

  // Network
  static const String defaultHotspotIp = '192.168.4.1';
  static const String defaultHostname = 'croenix.local';
  static const int defaultPort = 80;

  // Discovery
  static const String mdnsServiceType = '_croenix._tcp';
  static const int discoveryTimeout = 10; // seconds
  static const int udpDiscoveryPort = 5353;
  static const String discoveryMagic = 'CROENIX_DISCOVER';

  // Connection
  static const int connectionTimeout = 10; // seconds
  static const int requestTimeout = 5; // seconds
  static const int reconnectDelay = 3; // seconds
  static const int maxReconnectAttempts = 3;

  // Polling (until WebSocket in Phase 3)
  static const int statusPollInterval = 5; // seconds

  // Storage
  static const String storageKeyAuthToken = 'auth_token_';
  static const String storageKeySavedDevices = 'saved_devices';
  static const String storageKeyLastDevice = 'last_device';

  // Device
  static const String deviceName = 'Croenix';
  static const String appVersion = '1.0.0';

  // Pairing
  static const int pairingTimeout = 30; // seconds
}
