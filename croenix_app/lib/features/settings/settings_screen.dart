import 'package:flutter/material.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/services/websocket_service.dart';
import '../../data/models/device_state.dart';
import 'wifi_config_portal.dart';
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
  ConnectionStatus _connectionStatus = ConnectionStatus.disconnected;

  @override
  void initState() {
    super.initState();
    _deviceState = _wsService.currentState;
    _connectionStatus = _wsService.status;

    _wsService.stateStream.listen((state) {
      if (mounted) setState(() => _deviceState = state);
    });

    _wsService.statusStream.listen((status) {
      if (mounted) setState(() => _connectionStatus = status);
    });

    // Request fresh status on page open
    if (_wsService.isConnected) {
      _wsService.requestStatus();
    }
  }

  void _showChangeIPDialog() {
    final ipController = TextEditingController(
      text: _wsService.deviceIP ?? WebSocketService.defaultApIP,
    );

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: const Color(0xFF141927),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(16),
          side: const BorderSide(color: Color(0x334E65FF)),
        ),
        title: const Text(
          'Connect to Croenix',
          style: TextStyle(color: CroenixColors.textPrimary, fontWeight: FontWeight.bold),
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Enter your ESP32 IP address:',
              style: TextStyle(color: CroenixColors.textSecondary, fontSize: 13),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: ipController,
              style: const TextStyle(color: Colors.white),
              decoration: InputDecoration(
                filled: true,
                fillColor: const Color(0xFF1B2338),
                hintText: '192.168.4.1',
                hintStyle: const TextStyle(color: Colors.white38),
                prefixIcon: const Icon(Icons.lan_rounded, color: CroenixColors.primaryCyan),
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                  borderSide: BorderSide.none,
                ),
              ),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel', style: TextStyle(color: CroenixColors.textSecondary)),
          ),
          OutlinedButton(
            style: OutlinedButton.styleFrom(
              foregroundColor: CroenixColors.primaryCyan,
              side: const BorderSide(color: CroenixColors.primaryCyan),
            ),
            onPressed: () {
              Navigator.pop(context);
              _wsService.autoConnect();
            },
            child: const Text('Auto-Find'),
          ),
          ElevatedButton(
            style: ElevatedButton.styleFrom(
              backgroundColor: CroenixColors.primaryCyan,
              foregroundColor: Colors.black,
            ),
            onPressed: () {
              final ip = ipController.text.trim();
              if (ip.isNotEmpty) {
                Navigator.pop(context);
                _wsService.connect(ip);
              }
            },
            child: const Text('Connect', style: TextStyle(fontWeight: FontWeight.bold)),
          ),
        ],
      ),
    );
  }

  void _showRestartDialog() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: const Color(0xFF141927),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(16),
          side: const BorderSide(color: Color(0x334E65FF)),
        ),
        title: const Text('Restart Croenix', style: TextStyle(color: Colors.white)),
        content: const Text(
          'Are you sure you want to restart the Croenix ESP32 player? Audio playback will stop.',
          style: TextStyle(color: CroenixColors.textSecondary),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel', style: TextStyle(color: CroenixColors.textSecondary)),
          ),
          ElevatedButton(
            style: ElevatedButton.styleFrom(
              backgroundColor: const Color(0xFFFF5252),
              foregroundColor: Colors.white,
            ),
            onPressed: () {
              Navigator.pop(context);
              _wsService.rebootDevice();
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(
                  content: Text('Reboot command sent to Croenix!'),
                  backgroundColor: Color(0xFF1B2338),
                ),
              );
            },
            child: const Text('Restart Now'),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final isConnected = _connectionStatus == ConnectionStatus.connected;
    final isConnecting = _connectionStatus == ConnectionStatus.connecting;

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
            SliverAppBar(
              expandedHeight: 90,
              floating: false,
              pinned: true,
              backgroundColor: Colors.transparent,
              flexibleSpace: FlexibleSpaceBar(
                title: Row(
                  children: [
                    const Text(
                      'Settings',
                      style: TextStyle(
                        fontSize: 22,
                        fontWeight: FontWeight.bold,
                        color: CroenixColors.textPrimary,
                      ),
                    ),
                    const Spacer(),
                    IconButton(
                      icon: const Icon(Icons.sync_rounded, color: CroenixColors.primaryCyan, size: 22),
                      tooltip: 'Refresh Status',
                      onPressed: () {
                        if (isConnected) {
                          _wsService.requestStatus();
                        } else {
                          _wsService.autoConnect();
                        }
                      },
                    ),
                    const SizedBox(width: 8),
                  ],
                ),
                centerTitle: false,
                titlePadding: const EdgeInsets.only(left: 20, bottom: 12),
              ),
            ),

            SliverPadding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              sliver: SliverList(
                delegate: SliverChildListDelegate([
                  // === Live Connection Banner ===
                  _buildConnectionBanner(isConnected, isConnecting),

                  const SizedBox(height: 20),

                  // === Device & Firmware Information ===
                  InfoCard(
                    title: 'Firmware & Device',
                    items: [
                      InfoItem(
                        label: 'Device Name',
                        value: _deviceState.deviceName.isNotEmpty
                            ? _deviceState.deviceName
                            : 'Croenix',
                        icon: Icons.speaker_rounded,
                      ),
                      InfoItem(
                        label: 'Firmware Version',
                        value: _deviceState.firmwareVersion.isNotEmpty
                            ? 'v${_deviceState.firmwareVersion}'
                            : 'v1.0.0',
                        icon: Icons.verified_rounded,
                      ),
                      InfoItem(
                        label: 'ESP32 Chip Model',
                        value: _deviceState.chipModel.isNotEmpty
                            ? '${_deviceState.chipModel} (${_deviceState.chipCores} Cores @ ${_deviceState.cpuFreqMHz}MHz)'
                            : 'ESP32 (2 Cores @ 240MHz)',
                        icon: Icons.developer_board_rounded,
                      ),
                      InfoItem(
                        label: 'ESP-IDF SDK',
                        value: _deviceState.sdkVersion.isNotEmpty
                            ? _deviceState.sdkVersion
                            : 'ESP32 Arduino Core',
                        icon: Icons.integration_instructions_rounded,
                      ),
                      InfoItem(
                        label: 'Flash Memory',
                        value: _deviceState.formattedFlashSize,
                        icon: Icons.memory_rounded,
                      ),
                      if (_deviceState.compileDate.isNotEmpty)
                        InfoItem(
                          label: 'Build Date',
                          value: '${_deviceState.compileDate} ${_deviceState.compileTime}',
                          icon: Icons.calendar_today_rounded,
                        ),
                    ],
                  ),

                  const SizedBox(height: 20),

                  // === Real-time System Usage & Uptime ===
                  _buildSystemUsageCard(isConnected),

                  const SizedBox(height: 20),

                  // === Network & Wi-Fi Settings ===
                  _buildSection(
                    title: 'NETWORK & WI-FI',
                    children: [
                      SettingsTile(
                        title: 'Wi-Fi Network',
                        subtitle: _deviceState.wifiSSID.isNotEmpty
                            ? _deviceState.wifiSSID
                            : (_deviceState.apMode ? 'Croenix AP (Provisioning)' : 'Not Connected'),
                        icon: Icons.wifi_rounded,
                        trailing: Container(
                          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
                          decoration: BoxDecoration(
                            color: _deviceState.wifiConnected
                                ? CroenixColors.success.withValues(alpha: 0.15)
                                : (_deviceState.apMode
                                    ? Colors.amber.withValues(alpha: 0.15)
                                    : Colors.white10),
                            borderRadius: BorderRadius.circular(8),
                          ),
                          child: Text(
                            _deviceState.wifiConnected
                                ? 'STA'
                                : (_deviceState.apMode ? 'AP' : 'OFF'),
                            style: TextStyle(
                              fontSize: 11,
                              fontWeight: FontWeight.bold,
                              color: _deviceState.wifiConnected
                                  ? CroenixColors.success
                                  : (_deviceState.apMode ? Colors.amber : Colors.white54),
                            ),
                          ),
                        ),
                      ),
                      SettingsTile(
                        title: 'IP Address',
                        subtitle: _deviceState.localIP.isNotEmpty
                            ? _deviceState.localIP
                            : 'No IP Assigned',
                        icon: Icons.lan_rounded,
                      ),
                      if (_deviceState.macAddress.isNotEmpty)
                        SettingsTile(
                          title: 'MAC Address',
                          subtitle: _deviceState.macAddress,
                          icon: Icons.fingerprint_rounded,
                        ),
                      SettingsTile(
                        title: 'Wi-Fi Signal Strength',
                        subtitle: _deviceState.wifiConnected
                            ? '${_deviceState.wifiRSSI} dBm (${_getSignalQuality(_deviceState.wifiRSSI)})'
                            : 'N/A',
                        icon: Icons.signal_cellular_alt_rounded,
                      ),
                      SettingsTile(
                        title: 'Configure Wi-Fi Portal',
                        subtitle: 'Dynamic QR Code & Hotspot Mode Switch',
                        icon: Icons.qr_code_2_rounded,
                        onTap: () => WiFiConfigPortal.show(context),
                      ),
                    ],
                  ),

                  const SizedBox(height: 20),

                  // === Bluetooth Settings ===
                  _buildSection(
                    title: 'BLUETOOTH',
                    children: [
                      const SettingsTile(
                        title: 'Device Name',
                        subtitle: 'Croenix',
                        icon: Icons.bluetooth_rounded,
                      ),
                      SettingsTile(
                        title: 'Bluetooth Status',
                        subtitle: _deviceState.bluetoothConnected
                            ? 'Connected to ${_deviceState.bluetoothDevice}'
                            : 'Ready / Discoverable as "Croenix"',
                        icon: Icons.bluetooth_searching_rounded,
                        trailing: Icon(
                          _deviceState.bluetoothConnected
                              ? Icons.bluetooth_connected_rounded
                              : Icons.bluetooth_rounded,
                          color: _deviceState.bluetoothConnected
                              ? CroenixColors.primaryCyan
                              : CroenixColors.textSecondary,
                        ),
                      ),
                    ],
                  ),

                  const SizedBox(height: 20),

                  // === Storage Settings ===
                  _buildSection(
                    title: 'STORAGE (SD CARD)',
                    children: [
                      SettingsTile(
                        title: 'SD Card Status',
                        subtitle: _deviceState.sdCardMounted
                            ? '${(_deviceState.sdCardUsed / (1024 * 1024)).toStringAsFixed(1)} MB / ${(_deviceState.sdCardSize / (1024 * 1024)).toStringAsFixed(1)} MB'
                            : 'SD Card Not Detected / Unmounted',
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
                    ],
                  ),

                  const SizedBox(height: 20),

                  // === Maintenance & Reboot ===
                  _buildSection(
                    title: 'MAINTENANCE',
                    children: [
                      SettingsTile(
                        title: 'Reboot Croenix ESP32',
                        subtitle: 'Restart hardware processor',
                        icon: Icons.restart_alt_rounded,
                        onTap: _showRestartDialog,
                      ),
                    ],
                  ),

                  const SizedBox(height: 120),
                ]),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildConnectionBanner(bool isConnected, bool isConnecting) {
    final statusColor = isConnected
        ? CroenixColors.success
        : (isConnecting ? Colors.amber : const Color(0xFFFF5252));

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: const Color(0xFF141927),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: statusColor.withValues(alpha: 0.35),
          width: 1.5,
        ),
      ),
      child: Column(
        children: [
          Row(
            children: [
              Container(
                width: 10,
                height: 10,
                decoration: BoxDecoration(
                  color: statusColor,
                  shape: BoxShape.circle,
                  boxShadow: [
                    BoxShadow(
                      color: statusColor.withValues(alpha: 0.5),
                      blurRadius: 8,
                      spreadRadius: 2,
                    ),
                  ],
                ),
              ),
              const SizedBox(width: 10),
              Text(
                isConnected
                    ? 'CONNECTED TO CROENIX'
                    : (isConnecting ? 'CONNECTING...' : 'OFFLINE'),
                style: TextStyle(
                  fontSize: 12,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 0.8,
                  color: statusColor,
                ),
              ),
              const Spacer(),
              if (_wsService.deviceIP != null)
                Text(
                  '${_wsService.deviceIP}:${_wsService.port}',
                  style: const TextStyle(
                    fontSize: 12,
                    color: CroenixColors.textSecondary,
                  ),
                ),
            ],
          ),
          const SizedBox(height: 14),
          Row(
            children: [
              Expanded(
                child: OutlinedButton.icon(
                  style: OutlinedButton.styleFrom(
                    foregroundColor: Colors.white,
                    side: const BorderSide(color: Colors.white24),
                    padding: const EdgeInsets.symmetric(vertical: 10),
                    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
                  ),
                  onPressed: _showChangeIPDialog,
                  icon: const Icon(Icons.settings_input_component_rounded, size: 16),
                  label: Text(
                    isConnected ? 'Change IP' : 'Set IP',
                    style: const TextStyle(fontSize: 12),
                  ),
                ),
              ),
              const SizedBox(width: 10),
              Expanded(
                child: isConnected
                    ? OutlinedButton.icon(
                        style: OutlinedButton.styleFrom(
                          foregroundColor: const Color(0xFFFF5252),
                          side: const BorderSide(color: Color(0x66FF5252)),
                          padding: const EdgeInsets.symmetric(vertical: 10),
                          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
                        ),
                        onPressed: () => _wsService.disconnect(),
                        icon: const Icon(Icons.power_off_rounded, size: 16),
                        label: const Text('Disconnect', style: TextStyle(fontSize: 12)),
                      )
                    : ElevatedButton.icon(
                        style: ElevatedButton.styleFrom(
                          backgroundColor: CroenixColors.primaryCyan,
                          foregroundColor: Colors.black,
                          padding: const EdgeInsets.symmetric(vertical: 10),
                          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
                        ),
                        onPressed: () => _wsService.autoConnect(),
                        icon: const Icon(Icons.power_settings_new_rounded, size: 16),
                        label: const Text('Connect', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold)),
                      ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildSystemUsageCard(bool isConnected) {
    final heapPercent = _deviceState.heapUsagePercent.clamp(0, 100);

    return Container(
      padding: const EdgeInsets.all(20),
      decoration: GlassEffect.standard,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              const Text(
                'Live System Metrics',
                style: TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.bold,
                  color: CroenixColors.textPrimary,
                ),
              ),
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
                decoration: BoxDecoration(
                  color: isConnected
                      ? CroenixColors.success.withValues(alpha: 0.15)
                      : Colors.white10,
                  borderRadius: BorderRadius.circular(6),
                ),
                child: Text(
                  isConnected ? 'REAL-TIME' : 'CACHED',
                  style: TextStyle(
                    fontSize: 10,
                    fontWeight: FontWeight.bold,
                    color: isConnected ? CroenixColors.success : Colors.white38,
                    letterSpacing: 0.5,
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),

          // Uptime
          Row(
            children: [
              const Icon(Icons.timer_outlined, color: CroenixColors.primaryCyan, size: 18),
              const SizedBox(width: 8),
              const Text('Device Uptime', style: TextStyle(color: CroenixColors.textSecondary, fontSize: 13)),
              const Spacer(),
              Text(
                _deviceState.formattedUptime,
                style: const TextStyle(
                  color: CroenixColors.textPrimary,
                  fontSize: 14,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ],
          ),

          const SizedBox(height: 16),
          const Divider(color: CroenixColors.divider),
          const SizedBox(height: 12),

          // RAM / Heap Usage Progress Bar
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              const Text('RAM / Heap Usage', style: TextStyle(color: CroenixColors.textSecondary, fontSize: 13)),
              Text(
                '$heapPercent% (${_deviceState.formattedFreeHeap} free)',
                style: const TextStyle(
                  color: CroenixColors.textPrimary,
                  fontSize: 12,
                  fontWeight: FontWeight.w600,
                ),
              ),
            ],
          ),
          const SizedBox(height: 8),
          ClipRRect(
            borderRadius: BorderRadius.circular(4),
            child: LinearProgressIndicator(
              value: heapPercent / 100.0,
              minHeight: 6,
              backgroundColor: Colors.white10,
              valueColor: AlwaysStoppedAnimation<Color>(
                heapPercent > 85 ? Colors.redAccent : CroenixColors.primaryCyan,
              ),
            ),
          ),
          const SizedBox(height: 6),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'Total: ${_deviceState.formattedTotalHeap}',
                style: const TextStyle(color: Colors.white38, fontSize: 11),
              ),
              if (_deviceState.minFreeHeap > 0)
                Text(
                  'Min Free: ${(_deviceState.minFreeHeap / 1024).toStringAsFixed(1)} KB',
                  style: const TextStyle(color: Colors.white38, fontSize: 11),
                ),
            ],
          ),

          const SizedBox(height: 16),
          const Divider(color: CroenixColors.divider),
          const SizedBox(height: 12),

          // CPU Cores Usage
          Row(
            children: [
              const Icon(Icons.speed_rounded, color: CroenixColors.primaryCyan, size: 18),
              const SizedBox(width: 8),
              const Text('Core 0 / Core 1 CPU', style: TextStyle(color: CroenixColors.textSecondary, fontSize: 13)),
              const Spacer(),
              Text(
                '${_deviceState.cpuUsageCore0}% / ${_deviceState.cpuUsageCore1}%',
                style: const TextStyle(
                  color: CroenixColors.textPrimary,
                  fontSize: 13,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ],
          ),
        ],
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
          padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 8),
          child: Text(
            title,
            style: const TextStyle(
              fontSize: 12,
              fontWeight: FontWeight.bold,
              color: CroenixColors.textSecondary,
              letterSpacing: 0.8,
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

  String _getSignalQuality(int rssi) {
    if (rssi >= -60) return 'Excellent';
    if (rssi >= -70) return 'Good';
    if (rssi >= -80) return 'Fair';
    return 'Weak';
  }
}
