import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../core/theme/croenix_colors.dart';
import '../../core/constants/app_constants.dart';
import '../../data/repositories/device_repository.dart';
import '../../data/models/discovered_device.dart';
import '../connection/connection_screen.dart';

class WifiSetupDialog extends ConsumerStatefulWidget {
  const WifiSetupDialog({super.key});

  static Future<void> show(BuildContext context) {
    return showModalBottomSheet(
      context: context,
      isScrollControlled: true,
      backgroundColor: CroenixColors.surface,
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(24)),
      ),
      builder: (context) => const WifiSetupDialog(),
    );
  }

  @override
  ConsumerState<WifiSetupDialog> createState() => _WifiSetupDialogState();
}

class _WifiSetupDialogState extends ConsumerState<WifiSetupDialog> {
  final _ssidController = TextEditingController();
  final _passwordController = TextEditingController();
  bool _obscurePassword = true;
  bool _isLoading = false;
  String? _statusMessage;
  bool _isSuccess = false;

  @override
  void dispose() {
    _ssidController.dispose();
    _passwordController.dispose();
    super.dispose();
  }

  Future<void> _configureHomeWifi() async {
    final ssid = _ssidController.text.trim();
    final password = _passwordController.text;

    if (ssid.isEmpty) {
      setState(() {
        _statusMessage = 'Please enter your Wi-Fi network name';
        _isSuccess = false;
      });
      return;
    }

    setState(() {
      _isLoading = true;
      _statusMessage = 'Sending Wi-Fi credentials to Croenix...';
      _isSuccess = false;
    });

    try {
      final repository = DeviceRepository();
      final connectionService = ref.read(connectionServiceProvider);
      final currentDevice = connectionService.state.device;

      if (currentDevice != null) {
        repository.setBaseUrl(currentDevice.baseUrl);
        repository.setAuthToken(connectionService.state.authToken);
      } else {
        repository.setBaseUrl('http://${AppConstants.defaultHotspotIp}:${AppConstants.defaultPort}');
      }

      await repository.configureWiFi(ssid, password);

      setState(() {
        _isLoading = false;
        _isSuccess = true;
        _statusMessage = 'Credentials saved! Croenix is connecting to "$ssid". It will receive a home network IP.';
      });
    } catch (e) {
      setState(() {
        _isLoading = false;
        _isSuccess = false;
        _statusMessage = 'Failed to configure Wi-Fi: $e';
      });
    }
  }

  void _switchToHotspot() {
    final connectionService = ref.read(connectionServiceProvider);
    connectionService.connectToDevice(
      DiscoveredDevice(
        name: AppConstants.deviceName,
        ip: AppConstants.defaultHotspotIp,
        port: AppConstants.defaultPort,
        method: DiscoveryMethod.hotspot,
      ),
    );
    Navigator.pop(context);
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('Switched connection to Direct Hotspot (192.168.4.1)'),
        backgroundColor: CroenixColors.primary,
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: EdgeInsets.only(
        left: 24,
        right: 24,
        top: 24,
        bottom: MediaQuery.of(context).viewInsets.bottom + 24,
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Row(
            children: [
              Container(
                padding: const EdgeInsets.all(10),
                decoration: BoxDecoration(
                  color: CroenixColors.primary.withAlpha(40),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: const Icon(Icons.wifi, color: CroenixColors.primary),
              ),
              const SizedBox(width: 14),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Wi-Fi Setup & Switch',
                      style: Theme.of(context).textTheme.titleLarge?.copyWith(
                            fontWeight: FontWeight.bold,
                          ),
                    ),
                    Text(
                      'Connect Croenix to Home Router or Hotspot',
                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                            color: CroenixColors.textSecondary,
                          ),
                    ),
                  ],
                ),
              ),
              IconButton(
                icon: const Icon(Icons.close),
                onPressed: () => Navigator.pop(context),
              ),
            ],
          ),
          const SizedBox(height: 20),

          // Hotspot Quick Switch Button
          OutlinedButton.icon(
            onPressed: _switchToHotspot,
            icon: const Icon(Icons.wifi_tethering, color: CroenixColors.primary),
            label: const Text('Switch Connection to Direct Hotspot (192.168.4.1)'),
            style: OutlinedButton.styleFrom(
              padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 16),
            ),
          ),
          const SizedBox(height: 20),

          const Divider(),
          const SizedBox(height: 12),
          Text(
            'Connect to Home Wi-Fi Router',
            style: Theme.of(context).textTheme.titleSmall?.copyWith(
                  fontWeight: FontWeight.w600,
                ),
          ),
          const SizedBox(height: 12),

          // SSID input
          TextField(
            controller: _ssidController,
            decoration: const InputDecoration(
              labelText: 'Home Wi-Fi Name (SSID)',
              hintText: 'e.g. Home_Network_5G',
              prefixIcon: Icon(Icons.router),
            ),
          ),
          const SizedBox(height: 12),

          // Password input
          TextField(
            controller: _passwordController,
            obscureText: _obscurePassword,
            decoration: InputDecoration(
              labelText: 'Wi-Fi Password',
              prefixIcon: const Icon(Icons.lock_outline),
              suffixIcon: IconButton(
                icon: Icon(
                  _obscurePassword ? Icons.visibility_off : Icons.visibility,
                ),
                onPressed: () {
                  setState(() {
                    _obscurePassword = !_obscurePassword;
                  });
                },
              ),
            ),
          ),
          const SizedBox(height: 16),

          if (_statusMessage != null) ...[
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: _isSuccess
                    ? CroenixColors.success.withAlpha(35)
                    : CroenixColors.error.withAlpha(35),
                borderRadius: BorderRadius.circular(8),
                border: Border.all(
                  color: _isSuccess ? CroenixColors.success : CroenixColors.error,
                  width: 1,
                ),
              ),
              child: Text(
                _statusMessage!,
                style: TextStyle(
                  color: _isSuccess ? CroenixColors.success : CroenixColors.error,
                  fontSize: 13,
                ),
              ),
            ),
            const SizedBox(height: 16),
          ],

          // Submit Button
          ElevatedButton(
            onPressed: _isLoading ? null : _configureHomeWifi,
            child: _isLoading
                ? const SizedBox(
                    width: 20,
                    height: 20,
                    child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white),
                  )
                : const Text('Save & Connect Croenix to Home Wi-Fi'),
          ),
        ],
      ),
    );
  }
}

