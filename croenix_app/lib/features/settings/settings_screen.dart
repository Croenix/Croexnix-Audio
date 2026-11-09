import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../core/theme/croenix_colors.dart';
import '../../core/constants/app_constants.dart';
import '../connection/connection_screen.dart';
import '../security/passlock_screen.dart';
import 'wifi_setup_dialog.dart';

class SettingsScreen extends ConsumerStatefulWidget {
  const SettingsScreen({super.key});

  @override
  ConsumerState<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends ConsumerState<SettingsScreen> {
  bool _passlockEnabled = false;
  bool _isLoading = true;

  @override
  void initState() {
    super.initState();
    _loadPasslockStatus();
  }

  Future<void> _loadPasslockStatus() async {
    final service = ref.read(passlockServiceProvider);
    final enabled = await service.isPasslockEnabled();
    if (mounted) {
      setState(() {
        _passlockEnabled = enabled;
        _isLoading = false;
      });
    }
  }

  Future<void> _handlePasslockToggle() async {
    final service = ref.read(passlockServiceProvider);
    if (_passlockEnabled) {
      // Prompt for current PIN to disable
      final unlocked = await Navigator.push<bool>(
        context,
        MaterialPageRoute(
          builder: (context) => PasslockScreen(
            mode: PasslockMode.unlock,
            onSuccess: () => Navigator.pop(context, true),
          ),
        ),
      );

      if (unlocked == true) {
        await service.disablePasslock();
        await _loadPasslockStatus();
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('Security Passlock disabled'),
              backgroundColor: CroenixColors.warning,
            ),
          );
        }
      }
    } else {
      // Setup new PIN
      final setupComplete = await Navigator.push<bool>(
        context,
        MaterialPageRoute(
          builder: (context) => PasslockScreen(
            mode: PasslockMode.setup,
            onSuccess: () => Navigator.pop(context, true),
          ),
        ),
      );

      if (setupComplete == true) {
        await _loadPasslockStatus();
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final connectionState = ref.watch(connectionStateProvider).valueOrNull;

    return Scaffold(
      appBar: AppBar(
        title: const Text('Settings & Security'),
      ),
      body: ListView(
        padding: const EdgeInsets.all(20),
        children: [
          // Section: Network & Wi-Fi
          Text(
            'NETWORK & CONNECTIVITY',
            style: Theme.of(context).textTheme.labelMedium?.copyWith(
                  color: CroenixColors.primary,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 1.2,
                ),
          ),
          const SizedBox(height: 12),
          Card(
            child: Column(
              children: [
                ListTile(
                  leading: const Icon(Icons.wifi, color: CroenixColors.primary),
                  title: const Text('Configure Home Wi-Fi'),
                  subtitle: const Text('Connect Croenix to your home router network'),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: () => WifiSetupDialog.show(context),
                ),
                const Divider(height: 1),
                ListTile(
                  leading: const Icon(Icons.wifi_tethering, color: CroenixColors.info),
                  title: const Text('Direct Hotspot Details'),
                  subtitle: const Text('SSID: Croenix-Audio | Pass: 12345678 (192.168.4.1)'),
                  trailing: TextButton(
                    onPressed: () => WifiSetupDialog.show(context),
                    child: const Text('Switch'),
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(height: 24),

          // Section: Security & Passlock
          Text(
            'SECURITY & ACCESS CONTROL',
            style: Theme.of(context).textTheme.labelMedium?.copyWith(
                  color: CroenixColors.primary,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 1.2,
                ),
          ),
          const SizedBox(height: 12),
          Card(
            child: Column(
              children: [
                SwitchListTile(
                  secondary: Icon(
                    _passlockEnabled ? Icons.lock : Icons.lock_open,
                    color: _passlockEnabled ? CroenixColors.success : CroenixColors.textSecondary,
                  ),
                  title: const Text('App Security Passlock'),
                  subtitle: Text(
                    _passlockEnabled
                        ? 'PIN protection active: unauthorized access locked'
                        : 'No PIN set: anyone with app can control player',
                  ),
                  value: _passlockEnabled,
                  onChanged: _isLoading ? null : (_) => _handlePasslockToggle(),
                ),
                if (_passlockEnabled) ...[
                  const Divider(height: 1),
                  ListTile(
                    leading: const Icon(Icons.pin, color: CroenixColors.primary),
                    title: const Text('Change Security PIN'),
                    trailing: const Icon(Icons.chevron_right),
                    onTap: () async {
                      final nav = Navigator.of(context);
                      // Verify current PIN first
                      final verified = await nav.push<bool>(
                        MaterialPageRoute(
                          builder: (context) => PasslockScreen(
                            mode: PasslockMode.unlock,
                            onSuccess: () => Navigator.pop(context, true),
                          ),
                        ),
                      );

                      if (verified == true && mounted) {
                        await nav.push<bool>(
                          MaterialPageRoute(
                            builder: (context) => PasslockScreen(
                              mode: PasslockMode.setup,
                              onSuccess: () => Navigator.pop(context, true),
                            ),
                          ),
                        );
                        if (mounted) {
                          await _loadPasslockStatus();
                        }
                      }
                    },
                  ),
                ],
              ],
            ),
          ),
          const SizedBox(height: 24),

          // Section: Active Connection
          Text(
            'ACTIVE CONNECTION',
            style: Theme.of(context).textTheme.labelMedium?.copyWith(
                  color: CroenixColors.primary,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 1.2,
                ),
          ),
          const SizedBox(height: 12),
          Card(
            child: Column(
              children: [
                ListTile(
                  leading: const Icon(Icons.devices, color: CroenixColors.textSecondary),
                  title: Text(connectionState?.device?.name ?? AppConstants.deviceName),
                  subtitle: Text('IP: ${connectionState?.device?.ip ?? "Disconnected"}'),
                ),
                const Divider(height: 1),
                ListTile(
                  leading: const Icon(Icons.power_settings_new, color: CroenixColors.error),
                  title: const Text('Disconnect & Change Device'),
                  onTap: () {
                    context.go('/connection');
                  },
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
