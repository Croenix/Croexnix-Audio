import 'discovered_device.dart';

enum ConnectionStatus {
  disconnected,
  scanning,
  deviceFound,
  connecting,
  authenticating,
  connected,
  connectionLost,
  reconnecting,
  authFailed,
  error,
}

class ConnectionState {
  final ConnectionStatus status;
  final DiscoveredDevice? device;
  final String? authToken;
  final String? errorMessage;

  ConnectionState({
    required this.status,
    this.device,
    this.authToken,
    this.errorMessage,
  });

  ConnectionState.disconnected()
      : status = ConnectionStatus.disconnected,
        device = null,
        authToken = null,
        errorMessage = null;

  ConnectionState copyWith({
    ConnectionStatus? status,
    DiscoveredDevice? device,
    String? authToken,
    String? errorMessage,
  }) {
    return ConnectionState(
      status: status ?? this.status,
      device: device ?? this.device,
      authToken: authToken ?? this.authToken,
      errorMessage: errorMessage ?? this.errorMessage,
    );
  }

  bool get isConnected => status == ConnectionStatus.connected;
  bool get isConnecting =>
      status == ConnectionStatus.connecting ||
      status == ConnectionStatus.authenticating;
  bool get hasError =>
      status == ConnectionStatus.error || status == ConnectionStatus.authFailed;
}
