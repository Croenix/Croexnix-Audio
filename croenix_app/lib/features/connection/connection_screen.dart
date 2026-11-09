import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/services/discovery_service.dart';
import '../../data/services/connection_service.dart';
import '../../data/services/auth_service.dart';
import '../../data/repositories/device_repository.dart';
import '../../data/models/connection_state.dart' as conn;
import '../../data/models/discovered_device.dart';

// Providers
final discoveryServiceProvider = Provider((ref) => DiscoveryService());

final connectionServiceProvider = Provider((ref) {
  return ConnectionService(
    repository: DeviceRepository(),
    authService: AuthService(),
    discoveryService: ref.watch(discoveryServiceProvider),
  );
});

final discoveredDevicesProvider = StreamProvider<List<DiscoveredDevice>>((ref) {
  final service = ref.watch(discoveryServiceProvider);
  return service.devicesStream;
});

final connectionStateProvider = StreamProvider<conn.ConnectionState>((ref) {
  final service = ref.watch(connectionServiceProvider);
  return service.stateStream;
});

class ConnectionScreen extends ConsumerStatefulWidget {
  const ConnectionScreen({super.key});

  @override
  ConsumerState<ConnectionScreen> createState() => _ConnectionScreenState();
}

class _ConnectionScreenState extends ConsumerState<ConnectionScreen> {
  @override
  void initState() {
    super.initState();
    // Start discovery when screen loads
    WidgetsBinding.instance.addPostFrameCallback((_) {
      ref.read(connectionServiceProvider).startDiscovery();
    });
  }

  @override
  Widget build(BuildContext context) {
    final devicesAsync = ref.watch(discoveredDevicesProvider);
    final connectionStateAsync = ref.watch(connectionStateProvider);

    // Navigate to dashboard when connected
    connectionStateAsync.whenData((state) {
      if (state.isConnected) {
        WidgetsBinding.instance.addPostFrameCallback((_) {
          context.go('/dashboard');
        });
      }
    });

    return Scaffold(
      appBar: AppBar(
        title: const Text('Connect to Croenix'),
      ),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(24.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              // Connection Status Card
              _buildConnectionStatusCard(connectionStateAsync),

              const SizedBox(height: 32),

              // Device List
              Expanded(
                child: _buildDeviceList(devicesAsync),
              ),

              const SizedBox(height: 16),

              // Manual IP Button
              OutlinedButton.icon(
                onPressed: () => _showManualIpDialog(),
                icon: const Icon(Icons.edit),
                label: const Text('Enter IP Manually'),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildConnectionStatusCard(AsyncValue<conn.ConnectionState> stateAsync) {
    return stateAsync.when(
      data: (state) {
        Color statusColor;
        IconData statusIcon;
        String statusText;

        switch (state.status) {
          case conn.ConnectionStatus.scanning:
            statusColor = CroenixColors.info;
            statusIcon = Icons.search;
            statusText = 'Scanning for devices...';
            break;
          case conn.ConnectionStatus.connecting:
            statusColor = CroenixColors.warning;
            statusIcon = Icons.sync;
            statusText = 'Connecting...';
            break;
          case conn.ConnectionStatus.authenticating:
            statusColor = CroenixColors.warning;
            statusIcon = Icons.lock;
            statusText = 'Authenticating...';
            break;
          case conn.ConnectionStatus.connected:
            statusColor = CroenixColors.success;
            statusIcon = Icons.check_circle;
            statusText = 'Connected';
            break;
          case conn.ConnectionStatus.authFailed:
            statusColor = CroenixColors.error;
            statusIcon = Icons.error;
            statusText = state.errorMessage ?? 'Authentication failed';
            break;
          case conn.ConnectionStatus.error:
            statusColor = CroenixColors.error;
            statusIcon = Icons.error;
            statusText = state.errorMessage ?? 'Connection error';
            break;
          default:
            statusColor = CroenixColors.textSecondary;
            statusIcon = Icons.circle_outlined;
            statusText = 'Not connected';
        }

        return Card(
          child: Padding(
            padding: const EdgeInsets.all(20.0),
            child: Row(
              children: [
                Icon(statusIcon, color: statusColor, size: 32),
                const SizedBox(width: 16),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        statusText,
                        style: Theme.of(context).textTheme.titleMedium?.copyWith(
                          color: statusColor,
                        ),
                      ),
                      if (state.device != null) ...[
                        const SizedBox(height: 4),
                        Text(
                          state.device!.ip,
                          style: Theme.of(context).textTheme.bodySmall,
                        ),
                      ],
                    ],
                  ),
                ),
                if (state.isConnecting)
                  const SizedBox(
                    width: 24,
                    height: 24,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  ),
              ],
            ),
          ),
        );
      },
      loading: () => const Card(
        child: Padding(
          padding: EdgeInsets.all(20.0),
          child: Center(child: CircularProgressIndicator()),
        ),
      ),
      error: (error, stack) => Card(
        child: Padding(
          padding: const EdgeInsets.all(20.0),
          child: Text('Error: $error'),
        ),
      ),
    );
  }

  Widget _buildDeviceList(AsyncValue<List<DiscoveredDevice>> devicesAsync) {
    return devicesAsync.when(
      data: (devices) {
        if (devices.isEmpty) {
          return Center(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(
                  Icons.search,
                  size: 64,
                  color: CroenixColors.textTertiary,
                ),
                const SizedBox(height: 16),
                Text(
                  'Searching for Croenix devices...',
                  style: Theme.of(context).textTheme.bodyLarge,
                ),
                const SizedBox(height: 8),
                Text(
                  'Make sure your device is powered on',
                  style: Theme.of(context).textTheme.bodySmall,
                  textAlign: TextAlign.center,
                ),
              ],
            ),
          );
        }

        return ListView.builder(
          itemCount: devices.length,
          itemBuilder: (context, index) {
            final device = devices[index];
            return Card(
              margin: const EdgeInsets.only(bottom: 12),
              child: ListTile(
                contentPadding: const EdgeInsets.all(16),
                leading: Container(
                  width: 56,
                  height: 56,
                  decoration: BoxDecoration(
                    gradient: LinearGradient(
                      colors: CroenixColors.primaryGradient,
                      begin: Alignment.topLeft,
                      end: Alignment.bottomRight,
                    ),
                    borderRadius: BorderRadius.circular(12),
                  ),
                  child: const Icon(
                    Icons.music_note,
                    color: Colors.white,
                  ),
                ),
                title: Text(
                  device.name,
                  style: Theme.of(context).textTheme.titleMedium,
                ),
                subtitle: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const SizedBox(height: 4),
                    Text(device.ip),
                    const SizedBox(height: 2),
                    Text(
                      _getMethodLabel(device.method),
                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                        color: CroenixColors.primary,
                      ),
                    ),
                  ],
                ),
                trailing: ElevatedButton(
                  onPressed: () => _connectToDevice(device),
                  child: const Text('Connect'),
                ),
              ),
            );
          },
        );
      },
      loading: () => const Center(child: CircularProgressIndicator()),
      error: (error, stack) => Center(child: Text('Error: $error')),
    );
  }

  String _getMethodLabel(DiscoveryMethod method) {
    switch (method) {
      case DiscoveryMethod.mdns:
        return 'Found via mDNS';
      case DiscoveryMethod.hotspot:
        return 'Direct Hotspot';
      case DiscoveryMethod.manual:
        return 'Manual Entry';
      case DiscoveryMethod.saved:
        return 'Saved Device';
    }
  }

  Future<void> _connectToDevice(DiscoveredDevice device) async {
    await ref.read(connectionServiceProvider).connectToDevice(device);
  }

  void _showManualIpDialog() {
    final controller = TextEditingController();

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Enter IP Address'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(
            hintText: '192.168.x.x',
            labelText: 'IP Address',
          ),
          keyboardType: TextInputType.number,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              final ip = controller.text.trim();
              if (ip.isNotEmpty) {
                ref.read(discoveryServiceProvider).addManualDevice(ip);
                Navigator.pop(context);
              }
            },
            child: const Text('Add'),
          ),
        ],
      ),
    );
  }
}
