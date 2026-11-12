import 'package:flutter/material.dart';
import '../../../core/theme/croenix_colors.dart';
import '../../../data/services/websocket_service.dart';
import '../../../data/models/device_state.dart';
import '../../settings/settings_screen.dart';

class ConnectionStatusCard extends StatefulWidget {
  const ConnectionStatusCard({super.key});

  @override
  State<ConnectionStatusCard> createState() => _ConnectionStatusCardState();
}

class _ConnectionStatusCardState extends State<ConnectionStatusCard> {
  final WebSocketService _wsService = WebSocketService();
  ConnectionStatus _status = ConnectionStatus.disconnected;
  DeviceState _deviceState = const DeviceState();

  @override
  void initState() {
    super.initState();
    _status = _wsService.status;
    _deviceState = _wsService.currentState;

    _wsService.statusStream.listen((status) {
      if (mounted) setState(() => _status = status);
    });

    _wsService.stateStream.listen((state) {
      if (mounted) setState(() => _deviceState = state);
    });
  }

  void _showQuickConnectDialog() {
    final ipController = TextEditingController(
      text: _wsService.deviceIP ?? WebSocketService.defaultApIP,
    );

    showModalBottomSheet(
      context: context,
      backgroundColor: Colors.transparent,
      isScrollControlled: true,
      builder: (context) {
        return Padding(
          padding: EdgeInsets.only(
            bottom: MediaQuery.of(context).viewInsets.bottom,
          ),
          child: Container(
            padding: const EdgeInsets.all(24),
            decoration: const BoxDecoration(
              color: Color(0xFF141927),
              borderRadius: BorderRadius.vertical(top: Radius.circular(24)),
              border: Border(
                top: BorderSide(color: Color(0x334E65FF), width: 1.5),
              ),
            ),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text(
                      'Connect to Croenix',
                      style: TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.bold,
                        color: CroenixColors.textPrimary,
                      ),
                    ),
                    IconButton(
                      icon: const Icon(Icons.close_rounded, color: CroenixColors.textSecondary),
                      onPressed: () => Navigator.pop(context),
                    ),
                  ],
                ),
                const SizedBox(height: 8),
                const Text(
                  'Enter the IP address of your ESP32 audio player (e.g. 192.168.4.1 in AP mode, or your home Wi-Fi IP).',
                  style: TextStyle(fontSize: 13, color: CroenixColors.textSecondary),
                ),
                const SizedBox(height: 16),
                TextField(
                  controller: ipController,
                  style: const TextStyle(color: Colors.white, fontSize: 16),
                  keyboardType: TextInputType.number,
                  decoration: InputDecoration(
                    prefixIcon: const Icon(Icons.lan_rounded, color: CroenixColors.primaryCyan),
                    hintText: '192.168.4.1',
                    hintStyle: const TextStyle(color: Colors.white30),
                    filled: true,
                    fillColor: const Color(0xFF1B2338),
                    border: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(12),
                      borderSide: BorderSide.none,
                    ),
                  ),
                ),
                const SizedBox(height: 12),
                Wrap(
                  spacing: 8,
                  runSpacing: 8,
                  children: [
                    ActionChip(
                      backgroundColor: Colors.white.withOpacity(0.06),
                      avatar: const Icon(Icons.wifi_tethering_rounded, size: 16, color: CroenixColors.primaryCyan),
                      label: const Text('ESP32 Hotspot (192.168.4.1)', style: TextStyle(color: Colors.white70, fontSize: 11)),
                      onPressed: () {
                        ipController.text = '192.168.4.1';
                      },
                    ),
                    ActionChip(
                      backgroundColor: Colors.white.withOpacity(0.06),
                      avatar: const Icon(Icons.usb_rounded, size: 16, color: CroenixColors.secondaryPurple),
                      label: const Text('USB/Localhost (127.0.0.1)', style: TextStyle(color: Colors.white70, fontSize: 11)),
                      onPressed: () {
                        ipController.text = '127.0.0.1';
                      },
                    ),
                    ActionChip(
                      backgroundColor: Colors.white.withOpacity(0.06),
                      avatar: const Icon(Icons.home_rounded, size: 16, color: CroenixColors.success),
                      label: const Text('mDNS (croenix-audio.local)', style: TextStyle(color: Colors.white70, fontSize: 11)),
                      onPressed: () {
                        ipController.text = 'croenix-audio.local';
                      },
                    ),
                  ],
                ),
                const SizedBox(height: 16),
                Row(
                  children: [
                    Expanded(
                      child: OutlinedButton.icon(
                        style: OutlinedButton.styleFrom(
                          foregroundColor: CroenixColors.primaryCyan,
                          side: const BorderSide(color: CroenixColors.primaryCyan),
                          padding: const EdgeInsets.symmetric(vertical: 14),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(12),
                          ),
                        ),
                        onPressed: () {
                          Navigator.pop(context);
                          _wsService.autoConnect();
                        },
                        icon: const Icon(Icons.refresh_rounded, size: 18),
                        label: const Text('Auto-Find'),
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      flex: 2,
                      child: ElevatedButton.icon(
                        style: ElevatedButton.styleFrom(
                          backgroundColor: CroenixColors.primaryCyan,
                          foregroundColor: Colors.black,
                          padding: const EdgeInsets.symmetric(vertical: 14),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(12),
                          ),
                        ),
                        onPressed: () {
                          final ip = ipController.text.trim();
                          if (ip.isNotEmpty) {
                            Navigator.pop(context);
                            _wsService.connect(ip);
                          }
                        },
                        icon: const Icon(Icons.power_settings_new_rounded, size: 18),
                        label: const Text('Connect', style: TextStyle(fontWeight: FontWeight.bold)),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 12),
              ],
            ),
          ),
        );
      },
    );
  }

  @override
  Widget build(BuildContext context) {
    final isConnected = _status == ConnectionStatus.connected;
    final isConnecting = _status == ConnectionStatus.connecting;

    Color statusColor;
    String statusTitle;
    String statusSubtitle;
    IconData statusIcon;

    if (isConnected) {
      statusColor = CroenixColors.success;
      statusTitle = 'Connected to ${_deviceState.deviceName}';
      if (_deviceState.localIP.isNotEmpty) {
        statusSubtitle = '${_deviceState.localIP} • ${_deviceState.wifiSSID.isNotEmpty ? _deviceState.wifiSSID : "Croenix AP"}';
      } else {
        statusSubtitle = 'Online • Croenix Audio';
      }
      statusIcon = Icons.wifi_rounded;
    } else if (isConnecting) {
      statusColor = Colors.amber;
      statusTitle = 'Connecting to Croenix...';
      statusSubtitle = _wsService.deviceIP != null
          ? 'Contacting ${_wsService.deviceIP}'
          : 'Searching network for device';
      statusIcon = Icons.wifi_find_rounded;
    } else {
      statusColor = const Color(0xFFFF5252);
      statusTitle = 'Croenix Disconnected';
      statusSubtitle = 'Tap to connect to ESP32';
      statusIcon = Icons.wifi_off_rounded;
    }

    return GestureDetector(
      onTap: () {
        if (!isConnected) {
          _showQuickConnectDialog();
        } else {
          // Open settings to view full details
          Navigator.push(
            context,
            MaterialPageRoute(builder: (context) => const SettingsScreen()),
          );
        }
      },
      child: Container(
        padding: const EdgeInsets.all(16),
        decoration: GlassEffect.standard,
        child: Row(
          children: [
            Container(
              width: 48,
              height: 48,
              decoration: BoxDecoration(
                gradient: isConnected
                    ? CroenixColors.primaryGradient
                    : LinearGradient(
                        colors: [
                          statusColor.withValues(alpha: 0.3),
                          statusColor.withValues(alpha: 0.1),
                        ],
                      ),
                shape: BoxShape.circle,
                boxShadow: isConnected
                    ? [
                        BoxShadow(
                          color: CroenixColors.primaryCyan.withValues(alpha: 0.3),
                          blurRadius: 15,
                          offset: const Offset(0, 5),
                        ),
                      ]
                    : null,
              ),
              child: isConnecting
                  ? const Center(
                      child: SizedBox(
                        width: 22,
                        height: 22,
                        child: CircularProgressIndicator(
                          strokeWidth: 2,
                          color: Colors.amber,
                        ),
                      ),
                    )
                  : Icon(
                      statusIcon,
                      color: isConnected ? Colors.white : statusColor,
                      size: 24,
                    ),
            ),
            const SizedBox(width: 16),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    statusTitle,
                    style: const TextStyle(
                      fontSize: 15,
                      fontWeight: FontWeight.bold,
                      color: CroenixColors.textPrimary,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    statusSubtitle,
                    style: TextStyle(
                      fontSize: 12,
                      color: isConnected ? CroenixColors.textSecondary : statusColor.withValues(alpha: 0.8),
                    ),
                  ),
                ],
              ),
            ),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
              decoration: BoxDecoration(
                color: statusColor.withValues(alpha: 0.15),
                borderRadius: BorderRadius.circular(12),
                border: Border.all(color: statusColor.withValues(alpha: 0.4), width: 1),
              ),
              child: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Container(
                    width: 7,
                    height: 7,
                    decoration: BoxDecoration(
                      color: statusColor,
                      shape: BoxShape.circle,
                    ),
                  ),
                  const SizedBox(width: 6),
                  Text(
                    isConnected
                        ? 'LIVE'
                        : isConnecting
                            ? 'WAIT'
                            : 'OFFLINE',
                    style: TextStyle(
                      fontSize: 10,
                      fontWeight: FontWeight.bold,
                      color: statusColor,
                      letterSpacing: 0.5,
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
