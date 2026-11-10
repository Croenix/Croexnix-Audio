import 'package:flutter/material.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/services/websocket_service.dart';
import '../../data/models/device_state.dart';
import '../wifi_setup/wifi_setup_screen.dart';
import 'widgets/settings_tile.dart';
import 'widgets/info_card.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  final WebSocketService _wsService = WebSocketService();
  DeviceState _deviceState = const DeviceState();

  @override
  void initState() {
    super.initState();
    _wsService.stateStream.listen((state) {
      setState(() => _deviceState = state);
    });
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: const BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            CroenixColors.darkBackground,
            Color(0xFF0F1523),
          ],
        ),
      ),
      child: SafeArea(
        child: CustomScrollView(
          physics: const BouncingScrollPhysics(),
          slivers: [
            // App Bar
            const SliverAppBar(
              expandedHeight: 100,
              floating: false,
              pinned: true,
              backgroundColor: Colors.transparent,
              flexibleSpace: FlexibleSpaceBar(
                title: Text(
                  'Settings',
                  style: TextStyle(
                    fontSize: 24,
                    fontWeight: FontWeight.bold,
                    color: CroenixColors.textPrimary,
                  ),
                ),
                centerTitle: false,
                titlePadding: EdgeInsets.only(left: 24, bottom: 16),
              ),
            ),

            SliverPadding(
              padding: const EdgeInsets.all(16),
              sliver: SliverList(
                delegate: SliverChildListDelegate([
                  // Device Info Card
                  InfoCard(
                    title: 'Device Information',
                    items: [
                      InfoItem(
                        label: 'Device Name',
                        value: 'Croenix Audio Player',
                        icon: Icons.speaker_rounded,
                      ),
                      InfoItem(
                        label: 'IP Address',
                        value: _deviceState.localIP.isNotEmpty
                            ? _deviceState.localIP
                            : 'Not Connected',
                        icon: Icons.lan_rounded,
                      ),
                      InfoItem(
                        label: 'Wi-Fi Network',
                        value: _deviceState.wifiSSID.isNotEmpty
                            ? _deviceState.wifiSSID
                            : 'Not Connected',
                        icon: Icons.wifi_rounded,
                      ),
                      InfoItem(
                        label: 'Signal Strength',
                        value: _deviceState.wifiRSSI != 0
                            ? '${_deviceState.wifiRSSI} dBm'
                            : '---',
                        icon: Icons.signal_cellular_alt_rounded,
                      ),
                    ],
                  ),

                  const SizedBox(height: 16),

                  // Network Settings
                  _buildSection(
                    title: 'Network',
                    children: [
                      SettingsTile(
                        title: 'Wi-Fi Configuration',
                        subtitle: 'Configure network connection',
                        icon: Icons.wifi_rounded,
                        onTap: () {
                          Navigator.push(
                            context,
                            MaterialPageRoute(
                              builder: (context) => const WiFiSetupScreen(),
                            ),
                          );
                        },
                      ),
                      SettingsTile(
                        title: 'Access Point Mode',
                        subtitle: _deviceState.apMode
                            ? 'Active - Visible to nearby devices'
                            : 'Inactive',
                        icon: Icons.router_rounded,
                        trailing: Switch(
                          value: _deviceState.apMode,
                          onChanged: (value) {
                            // TODO: Toggle AP mode
                          },
                        ),
                      ),
                    ],
                  ),

                  const SizedBox(height: 16),

                  // Storage Settings
                  _buildSection(
                    title: 'Storage',
                    children: [
                      SettingsTile(
                        title: 'SD Card',
                        subtitle: _deviceState.sdCardMounted
                            ? '${(_deviceState.sdCardUsed / 1024 / 1024).toStringAsFixed(1)} MB / ${(_deviceState.sdCardSize / 1024 / 1024).toStringAsFixed(1)} MB'
                            : 'Not Mounted',
                        icon: Icons.sd_card_rounded,
                        trailing: Icon(
                          _deviceState.sdCardMounted
                              ? Icons.check_circle_rounded
                              : Icons.error_outline_rounded,
                          color: _deviceState.sdCardMounted
                              ? CroenixColors.success
                              : CroenixColors.error,
                        ),
                      ),
                      SettingsTile(
                        title: 'Browse Files',
                        subtitle: 'View audio files on SD card',
                        icon: Icons.folder_rounded,
                        onTap: () {
                          // TODO: Navigate to file browser
                        },
                      ),
                    ],
                  ),

                  const SizedBox(height: 16),

                  // System Settings
                  _buildSection(
                    title: 'System',
                    children: [
                      SettingsTile(
                        title: 'System Info',
                        subtitle: 'Uptime: ${_formatUptime(_deviceState.uptime)}',
                        icon: Icons.info_outline_rounded,
                      ),
                      SettingsTile(
                        title: 'CPU Usage',
                        subtitle: 'Core 0: ${_deviceState.cpuUsageCore0}% | Core 1: ${_deviceState.cpuUsageCore1}%',
                        icon: Icons.memory_rounded,
                      ),
                      SettingsTile(
                        title: 'Free Memory',
                        subtitle: '${(_deviceState.freeHeap / 1024).toStringAsFixed(1)} KB',
                        icon: Icons.storage_rounded,
                      ),
                      SettingsTile(
                        title: 'Restart Device',
                        subtitle: 'Reboot the ESP32',
                        icon: Icons.restart_alt_rounded,
                        onTap: _showRestartDialog,
                      ),
                    ],
                  ),

                  const SizedBox(height: 16),

                  // About
                  _buildSection(
                    title: 'About',
                    children: [
                      const SettingsTile(
                        title: 'App Version',
                        subtitle: '1.0.0',
                        icon: Icons.app_settings_alt_rounded,
                      ),
                      const SettingsTile(
                        title: 'Firmware Version',
                        subtitle: 'ESP32 v1.0.0',
                        icon: Icons.developer_board_rounded,
                      ),
                      SettingsTile(
                        title: 'Open Source Licenses',
                        subtitle: 'View licenses',
                        icon: Icons.code_rounded,
                        onTap: () {
                          showLicensePage(context: context);
                        },
                      ),
                    ],
                  ),

                  const SizedBox(height: 100),
                ]),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSection({
    required String title,
    required List<Widget> children,
  }) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 8),
          child: Text(
            title,
            style: const TextStyle(
              fontSize: 13,
              fontWeight: FontWeight.w600,
              color: CroenixColors.textSecondary,
              letterSpacing: 0.5,
            ),
          ),
        ),
        Container(
          decoration: GlassEffect.standard,
          child: Column(children: children),
        ),
      ],
    );
  }

  String _formatUptime(int seconds) {
    final hours = seconds ~/ 3600;
    final minutes = (seconds % 3600) ~/ 60;
    return '${hours}h ${minutes}m';
  }

  void _showRestartDialog() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Restart Device'),
        content: const Text(
          'Are you sure you want to restart the ESP32? This will interrupt audio playback.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              // TODO: Send restart command
              Navigator.pop(context);
            },
            child: const Text('Restart'),
          ),
        ],
      ),
    );
  }
}
