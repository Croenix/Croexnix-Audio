import 'package:flutter/material.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/services/websocket_service.dart';
import '../../data/services/discovery_service.dart';
import '../../data/models/discovered_device.dart';
import 'widgets/network_list_item.dart';

class WiFiSetupScreen extends StatefulWidget {
  const WiFiSetupScreen({super.key});

  @override
  State<WiFiSetupScreen> createState() => _WiFiSetupScreenState();
}

class _WiFiSetupScreenState extends State<WiFiSetupScreen> {
  final DiscoveryService _discoveryService = DiscoveryService();
  final WebSocketService _wsService = WebSocketService();

  List<DiscoveredDevice> _discoveredDevices = [];
  DiscoveredDevice? _selectedDevice;

  bool _isScanning = false;
  bool _isConnecting = false;
  String _statusMessage = '';

  @override
  void initState() {
    super.initState();
    _startDeviceDiscovery();

    _discoveryService.devicesStream.listen((devices) {
      setState(() => _discoveredDevices = devices);
    });
  }

  @override
  void dispose() {
    _discoveryService.dispose();
    super.dispose();
  }

  Future<void> _startDeviceDiscovery() async {
    setState(() {
      _isScanning = true;
      _statusMessage = 'Scanning for ESP32 devices...';
    });

    await _discoveryService.startDiscovery();

    await Future.delayed(const Duration(seconds: 5));

    setState(() {
      _isScanning = false;
      _statusMessage = _discoveredDevices.isEmpty
          ? 'No devices found. Make sure your ESP32 is powered on.'
          : 'Found ${_discoveredDevices.length} device(s)';
    });
  }

  Future<void> _connectToDevice(DiscoveredDevice device) async {
    setState(() {
      _selectedDevice = device;
      _isConnecting = true;
      _statusMessage = 'Connecting to ${device.displayName}...';
    });

    final success =
        await _wsService.connect(device.ipAddress, port: device.port);

    setState(() {
      _isConnecting = false;
      if (success) {
        _statusMessage = 'Connected successfully!';
        // Navigate to next step
        Future.delayed(const Duration(seconds: 1), () {
          Navigator.pushReplacement(
            context,
            MaterialPageRoute(
                builder: (context) => const _WiFiConfigurationStep()),
          );
        });
      } else {
        _statusMessage = 'Connection failed. Please try again.';
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
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
          child: Column(
            children: [
              // Header
              Padding(
                padding: const EdgeInsets.all(24),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    ShaderMask(
                      shaderCallback: (bounds) =>
                          CroenixColors.primaryGradient.createShader(bounds),
                      child: const Text(
                        'Setup Device',
                        style: TextStyle(
                          fontSize: 32,
                          fontWeight: FontWeight.bold,
                          color: Colors.white,
                        ),
                      ),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      _statusMessage,
                      style: const TextStyle(
                        fontSize: 14,
                        color: CroenixColors.textSecondary,
                      ),
                    ),
                  ],
                ),
              ),

              // Device List
              Expanded(
                child: _isScanning
                    ? _buildScanningView()
                    : _discoveredDevices.isEmpty
                        ? _buildEmptyView()
                        : _buildDeviceList(),
              ),

              // Rescan Button
              if (!_isScanning)
                Padding(
                  padding: const EdgeInsets.all(24),
                  child: ElevatedButton.icon(
                    onPressed: _startDeviceDiscovery,
                    icon: const Icon(Icons.refresh_rounded),
                    label: const Text('Scan Again'),
                    style: ElevatedButton.styleFrom(
                      minimumSize: const Size(double.infinity, 56),
                    ),
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildScanningView() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Container(
            width: 80,
            height: 80,
            decoration: BoxDecoration(
              gradient: CroenixColors.primaryGradient,
              shape: BoxShape.circle,
              boxShadow: [
                BoxShadow(
                  color: CroenixColors.primaryCyan.withValues(alpha: 0.3),
                  blurRadius: 30,
                  offset: const Offset(0, 10),
                ),
              ],
            ),
            child: const Padding(
              padding: EdgeInsets.all(20),
              child: CircularProgressIndicator(
                color: Colors.white,
                strokeWidth: 3,
              ),
            ),
          ),
          const SizedBox(height: 24),
          const Text(
            'Scanning for devices...',
            style: TextStyle(
              fontSize: 16,
              color: CroenixColors.textPrimary,
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildEmptyView() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.search_off_rounded,
              size: 80,
              color: CroenixColors.textSecondary.withValues(alpha: 0.5),
            ),
            const SizedBox(height: 24),
            const Text(
              'No Devices Found',
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: CroenixColors.textPrimary,
              ),
            ),
            const SizedBox(height: 12),
            const Text(
              'Make sure your ESP32 is powered on and in range.',
              textAlign: TextAlign.center,
              style: TextStyle(
                fontSize: 14,
                color: CroenixColors.textSecondary,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDeviceList() {
    return ListView.builder(
      padding: const EdgeInsets.symmetric(horizontal: 24),
      itemCount: _discoveredDevices.length,
      itemBuilder: (context, index) {
        final device = _discoveredDevices[index];
        final isSelected = _selectedDevice?.ipAddress == device.ipAddress;

        return NetworkListItem(
          title: device.displayName,
          subtitle: device.ipAddress,
          isConnected: isSelected,
          isConnecting: isSelected && _isConnecting,
          onTap: () => _connectToDevice(device),
        );
      },
    );
  }
}

// Wi-Fi Configuration Step
class _WiFiConfigurationStep extends StatefulWidget {
  const _WiFiConfigurationStep();

  @override
  State<_WiFiConfigurationStep> createState() => _WiFiConfigurationStepState();
}

class _WiFiConfigurationStepState extends State<_WiFiConfigurationStep> {
  final WebSocketService _wsService = WebSocketService();
  final _ssidController = TextEditingController();
  final _passwordController = TextEditingController();

  bool _isScanning = false;
  bool _isConnecting = false;
  List<Map<String, dynamic>> _wifiNetworks = [];
  String? _selectedSSID;
  bool _obscurePassword = true;

  @override
  void initState() {
    super.initState();
    _scanWiFiNetworks();
  }

  @override
  void dispose() {
    _ssidController.dispose();
    _passwordController.dispose();
    super.dispose();
  }

  void _scanWiFiNetworks() {
    setState(() => _isScanning = true);
    _wsService.scanNetworks();

    // Listen for scan results
    // TODO: Handle scan results from WebSocket
    Future.delayed(const Duration(seconds: 3), () {
      setState(() {
        _isScanning = false;
        // Mock data for now
        _wifiNetworks = [
          {'ssid': 'Home WiFi', 'rssi': -45},
          {'ssid': 'Office Network', 'rssi': -60},
          {'ssid': 'Guest WiFi', 'rssi': -75},
        ];
      });
    });
  }

  void _configureWiFi() {
    if (_ssidController.text.isEmpty || _passwordController.text.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Please enter SSID and password')),
      );
      return;
    }

    setState(() => _isConnecting = true);

    _wsService.configureWiFi(_ssidController.text, _passwordController.text);

    // TODO: Handle WiFi configuration result
    Future.delayed(const Duration(seconds: 3), () {
      setState(() => _isConnecting = false);
      Navigator.pop(context);
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
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
          child: Column(
            children: [
              // Header
              Padding(
                padding: const EdgeInsets.all(24),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      children: [
                        IconButton(
                          icon: const Icon(Icons.arrow_back_rounded),
                          onPressed: () => Navigator.pop(context),
                        ),
                        const SizedBox(width: 8),
                        const Text(
                          'Wi-Fi Setup',
                          style: TextStyle(
                            fontSize: 28,
                            fontWeight: FontWeight.bold,
                            color: CroenixColors.textPrimary,
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 8),
                    const Padding(
                      padding: EdgeInsets.only(left: 56),
                      child: Text(
                        'Connect your ESP32 to your home Wi-Fi network',
                        style: TextStyle(
                          fontSize: 14,
                          color: CroenixColors.textSecondary,
                        ),
                      ),
                    ),
                  ],
                ),
              ),

              Expanded(
                child: ListView(
                  padding: const EdgeInsets.symmetric(horizontal: 24),
                  children: [
                    // Available Networks
                    Container(
                      padding: const EdgeInsets.all(20),
                      decoration: GlassEffect.standard,
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Row(
                            mainAxisAlignment: MainAxisAlignment.spaceBetween,
                            children: [
                              const Text(
                                'Available Networks',
                                style: TextStyle(
                                  fontSize: 16,
                                  fontWeight: FontWeight.w600,
                                  color: CroenixColors.textPrimary,
                                ),
                              ),
                              if (!_isScanning)
                                IconButton(
                                  icon: const Icon(Icons.refresh_rounded),
                                  onPressed: _scanWiFiNetworks,
                                  color: CroenixColors.primaryCyan,
                                ),
                            ],
                          ),
                          const SizedBox(height: 12),
                          if (_isScanning)
                            const Center(
                              child: Padding(
                                padding: EdgeInsets.all(20),
                                child: CircularProgressIndicator(),
                              ),
                            )
                          else
                            ..._wifiNetworks.map((network) {
                              return ListTile(
                                leading: Icon(
                                  Icons.wifi_rounded,
                                  color: _getSignalColor(network['rssi']),
                                ),
                                title: Text(
                                  network['ssid'],
                                  style: TextStyle(
                                    color: _selectedSSID == network['ssid']
                                        ? CroenixColors.primaryCyan
                                        : CroenixColors.textPrimary,
                                  ),
                                ),
                                trailing: _selectedSSID == network['ssid']
                                    ? const Icon(
                                        Icons.check_circle_rounded,
                                        color: CroenixColors.primaryCyan,
                                      )
                                    : null,
                                onTap: () {
                                  setState(() {
                                    _selectedSSID = network['ssid'];
                                    _ssidController.text = network['ssid'];
                                  });
                                },
                              );
                            }),
                        ],
                      ),
                    ),

                    const SizedBox(height: 20),

                    // Manual Input
                    Container(
                      padding: const EdgeInsets.all(20),
                      decoration: GlassEffect.standard,
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          const Text(
                            'Network Credentials',
                            style: TextStyle(
                              fontSize: 16,
                              fontWeight: FontWeight.w600,
                              color: CroenixColors.textPrimary,
                            ),
                          ),
                          const SizedBox(height: 16),
                          TextField(
                            controller: _ssidController,
                            decoration: InputDecoration(
                              labelText: 'Network Name (SSID)',
                              prefixIcon: const Icon(Icons.wifi_rounded),
                              border: OutlineInputBorder(
                                borderRadius: BorderRadius.circular(12),
                              ),
                            ),
                          ),
                          const SizedBox(height: 16),
                          TextField(
                            controller: _passwordController,
                            obscureText: _obscurePassword,
                            decoration: InputDecoration(
                              labelText: 'Password',
                              prefixIcon: const Icon(Icons.lock_rounded),
                              suffixIcon: IconButton(
                                icon: Icon(
                                  _obscurePassword
                                      ? Icons.visibility_rounded
                                      : Icons.visibility_off_rounded,
                                ),
                                onPressed: () {
                                  setState(() =>
                                      _obscurePassword = !_obscurePassword);
                                },
                              ),
                              border: OutlineInputBorder(
                                borderRadius: BorderRadius.circular(12),
                              ),
                            ),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),

              // Connect Button
              Padding(
                padding: const EdgeInsets.all(24),
                child: ElevatedButton(
                  onPressed: _isConnecting ? null : _configureWiFi,
                  style: ElevatedButton.styleFrom(
                    minimumSize: const Size(double.infinity, 56),
                  ),
                  child: _isConnecting
                      ? const SizedBox(
                          width: 20,
                          height: 20,
                          child: CircularProgressIndicator(
                            strokeWidth: 2,
                            color: Colors.white,
                          ),
                        )
                      : const Text('Connect to Network'),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Color _getSignalColor(int rssi) {
    if (rssi > -50) return CroenixColors.success;
    if (rssi > -70) return CroenixColors.warning;
    return CroenixColors.error;
  }
}
