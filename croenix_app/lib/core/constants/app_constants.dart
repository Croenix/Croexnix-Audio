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
  static const String storageKeyPasslockPin = 'security_passlock_pin';
  static const String storageKeyPasslockEnabled = 'security_passlock_enabled';

  // Bluetooth Low Energy (BLE)
  static const String bleDeviceName = 'Croenix';
  static const String bleServiceUuid = '19b10000-e8f2-537e-4f6c-d104768a1214';
  static const String bleCharDeviceInfo = '19b10001-e8f2-537e-4f6c-d104768a1214';
  static const String bleCharProvisioning = '19b10002-e8f2-537e-4f6c-d104768a1214';
  static const String bleCharNetworkStatus = '19b10003-e8f2-537e-4f6c-d104768a1214';
  static const String bleCharPairing = '19b10004-e8f2-537e-4f6c-d104768a1214';
  static const String bleCharEvents = '19b10005-e8f2-537e-4f6c-d104768a1214';

  // Device
  static const String deviceName = 'Croenix';
  static const String appVersion = '1.0.0';

  // Pairing
  static const int pairingTimeout = 30; // seconds
}
