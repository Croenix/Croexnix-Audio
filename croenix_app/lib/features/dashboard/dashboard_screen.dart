import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../core/theme/croenix_colors.dart';
import '../../core/constants/app_constants.dart';
import '../../data/models/device_state.dart';
import '../../data/repositories/device_repository.dart';
import '../connection/connection_screen.dart';
import '../settings/wifi_setup_dialog.dart';

// Provider for device state polling
final deviceStateProvider = StreamProvider.autoDispose<DeviceState>((ref) async* {
  final repository = DeviceRepository();
  final connectionService = ref.watch(connectionServiceProvider);
  final connectionState = connectionService.state;

  if (!connectionState.isConnected) {
    throw Exception('Not connected');
  }

  // Set up repository with current connection
  repository.setBaseUrl(connectionState.device!.baseUrl);
  repository.setAuthToken(connectionState.authToken);

  // Poll for status updates (Phase 2 - will be replaced by WebSocket in Phase 3)
  while (true) {
    try {
      final state = await repository.getStatus();
      yield state;
      await Future.delayed(Duration(seconds: AppConstants.statusPollInterval));
    } catch (e) {
      // Silently retry on error
      await Future.delayed(const Duration(seconds: 2));
    }
  }
});

class DashboardScreen extends ConsumerWidget {
  const DashboardScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final deviceStateAsync = ref.watch(deviceStateProvider);

    return Scaffold(
      appBar: AppBar(
        title: const Text('Croenix'),
        actions: [
          IconButton(
            icon: const Icon(Icons.wifi),
            tooltip: 'Wi-Fi Setup & Switch',
            onPressed: () => WifiSetupDialog.show(context),
          ),
          IconButton(
            icon: const Icon(Icons.settings),
            tooltip: 'Settings & Security',
            onPressed: () {
              context.push('/settings');
            },
          ),
        ],
      ),
      body: deviceStateAsync.when(
        data: (state) => _buildDashboard(context, state),
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (error, stack) => _buildError(context, error.toString()),
      ),
    );
  }

  Widget _buildDashboard(BuildContext context, DeviceState state) {
    return RefreshIndicator(
      onRefresh: () async {
        // Refresh is handled by the stream
        await Future.delayed(const Duration(milliseconds: 500));
      },
      child: SingleChildScrollView(
        physics: const AlwaysScrollableScrollPhysics(),
        padding: const EdgeInsets.all(24.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // Device Info Card
            _buildDeviceInfoCard(context, state.device),
            const SizedBox(height: 16),

            // Connection Status Card
            _buildConnectionCard(context, state.connection),
            const SizedBox(height: 16),

            // Audio Status Card
            _buildAudioCard(context, state.audio),
            const SizedBox(height: 16),

            // System Status Card
            _buildSystemCard(context, state.system),
            const SizedBox(height: 16),

            // Storage Card
            _buildStorageCard(context, state.storage),
          ],
        ),
      ),
    );
  }

  Widget _buildDeviceInfoCard(BuildContext context, DeviceInfo device) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Container(
                  width: 48,
                  height: 48,
                  decoration: BoxDecoration(
                    gradient: LinearGradient(
                      colors: CroenixColors.primaryGradient,
                      begin: Alignment.topLeft,
                      end: Alignment.bottomRight,
                    ),
                    borderRadius: BorderRadius.circular(12),
                  ),
                  child: const Icon(Icons.music_note, color: Colors.white),
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        device.name,
                        style: Theme.of(context).textTheme.headlineMedium,
                      ),
                      Text(
                        'Firmware ${device.firmwareVersion}',
                        style: Theme.of(context).textTheme.bodySmall,
                      ),
                    ],
                  ),
                ),
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                  decoration: BoxDecoration(
                    color: CroenixColors.success.withAlpha(51),
                    borderRadius: BorderRadius.circular(20),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Container(
                        width: 8,
                        height: 8,
                        decoration: const BoxDecoration(
                          color: CroenixColors.success,
                          shape: BoxShape.circle,
                        ),
                      ),
                      const SizedBox(width: 6),
                      const Text(
                        'Online',
                        style: TextStyle(
                          color: CroenixColors.success,
                          fontSize: 12,
                          fontWeight: FontWeight.w600,
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            _buildInfoRow(context, 'Uptime', device.uptimeFormatted),
          ],
        ),
      ),
    );
  }

  Widget _buildConnectionCard(BuildContext context, ConnectionInfo connection) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Network',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            const SizedBox(height: 16),
            _buildInfoRow(
              context,
              'Active Interface',
              connection.activeInterface.toUpperCase(),
            ),
            const Divider(height: 24),
            if (connection.routerConnected) ...[
              _buildInfoRow(context, 'Router SSID', connection.routerSsid),
              _buildInfoRow(context, 'Router IP', connection.routerIp),
              _buildInfoRow(context, 'Signal', '${connection.routerRssi} dBm (${connection.signalStrength})'),
            ] else ...[
              _buildInfoRow(context, 'Router', 'Not connected'),
            ],
            const Divider(height: 24),
            _buildInfoRow(
              context,
              'Hotspot',
              connection.hotspotActive ? 'Active (${connection.hotspotClients} clients)' : 'Inactive',
            ),
            const SizedBox(height: 12),
            SizedBox(
              width: double.infinity,
              child: OutlinedButton.icon(
                onPressed: () => WifiSetupDialog.show(context),
                icon: const Icon(Icons.settings_input_antenna, size: 18),
                label: const Text('Configure Home Wi-Fi / Switch Network'),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildAudioCard(BuildContext context, AudioInfo audio) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Audio',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            const SizedBox(height: 16),
            if (audio.hasActiveSource) ...[
              _buildInfoRow(context, 'Source', audio.source.toUpperCase()),
              _buildInfoRow(context, 'Status', audio.status.toUpperCase()),
              _buildInfoRow(context, 'Volume', '${audio.volume}%'),
              _buildInfoRow(context, 'Muted', audio.muted ? 'Yes' : 'No'),
            ] else ...[
              Center(
                child: Column(
                  children: [
                    Icon(
                      Icons.music_off,
                      size: 48,
                      color: CroenixColors.textTertiary,
                    ),
                    const SizedBox(height: 8),
                    Text(
                      'No active audio source',
                      style: Theme.of(context).textTheme.bodyMedium,
                    ),
                  ],
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildSystemCard(BuildContext context, SystemInfo system) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'System',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            const SizedBox(height: 16),
            _buildInfoRow(
              context,
              'Heap Memory',
              '${(system.freeHeap / 1024).toStringAsFixed(1)} KB free',
            ),
            _buildInfoRow(
              context,
              'Heap Usage',
              '${system.heapUsagePercent}%',
            ),
            _buildInfoRow(context, 'Restart Reason', system.restartReason),
          ],
        ),
      ),
    );
  }

  Widget _buildStorageCard(BuildContext context, StorageInfo storage) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Storage',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            const SizedBox(height: 16),
            _buildInfoRow(
              context,
              'SD Card',
              storage.sdInserted ? 'Inserted' : 'Not inserted',
            ),
            if (storage.sdInserted && storage.totalBytes > 0) ...[
              _buildInfoRow(
                context,
                'Capacity',
                '${(storage.totalBytes / (1024 * 1024 * 1024)).toStringAsFixed(2)} GB',
              ),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildInfoRow(BuildContext context, String label, String value) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 12.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: Theme.of(context).textTheme.bodyMedium,
          ),
          Text(
            value,
            style: Theme.of(context).textTheme.bodyLarge?.copyWith(
              fontWeight: FontWeight.w600,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildError(BuildContext context, String error) {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(24.0),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(
              Icons.error_outline,
              size: 64,
              color: CroenixColors.error,
            ),
            const SizedBox(height: 16),
            Text(
              'Connection Lost',
              style: Theme.of(context).textTheme.headlineMedium,
            ),
            const SizedBox(height: 8),
            Text(
              error,
              style: Theme.of(context).textTheme.bodyMedium,
              textAlign: TextAlign.center,
            ),
            const SizedBox(height: 24),
            ElevatedButton(
              onPressed: () {
                // Navigate back to connection screen
                // Context is not available here, would need navigation service
              },
              child: const Text('Reconnect'),
            ),
          ],
        ),
      ),
    );
  }
}
