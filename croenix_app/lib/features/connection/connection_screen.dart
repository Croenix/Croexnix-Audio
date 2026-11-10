import 'package:flutter/material.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/services/websocket_service.dart';
import '../../data/services/discovery_service.dart';
import '../../data/models/discovered_device.dart';

class ConnectionScreen extends StatefulWidget {
  const ConnectionScreen({super.key});

  @override
  State<ConnectionScreen> createState() => _ConnectionScreenState();
}

class _ConnectionScreenState extends State<ConnectionScreen> {
  final WebSocketService _wsService = WebSocketService();
  final DiscoveryService _discoveryService = DiscoveryService();
  final TextEditingController _ipController = TextEditingController();

  List<DiscoveredDevice> _devices = [];
  bool _isScanning = false;
  bool _isConnecting = false;
  String? _errorMessage;

  @override
  void initState() {
    super.initState();
    _ipController.text = _wsService.deviceIP ?? WebSocketService.defaultApIP;

    _discoveryService.devicesStream.listen((devices) {
      if (mounted) {
        setState(() => _devices = devices);
      }
    });

    _startScan();
  }

  @override
  void dispose() {
    _discoveryService.dispose();
    _ipController.dispose();
    super.dispose();
  }

  Future<void> _startScan() async {
    setState(() {
      _isScanning = true;
      _errorMessage = null;
    });

    try {
      await _discoveryService.startDiscovery();
      await Future.delayed(const Duration(seconds: 4));
    } catch (e) {
      if (mounted) setState(() => _errorMessage = 'Scan error: $e');
    } finally {
      if (mounted) setState(() => _isScanning = false);
    }
  }

  Future<void> _connect(String ip, {int port = 8080}) async {
    setState(() {
      _isConnecting = true;
      _errorMessage = null;
    });

    final success = await _wsService.connect(ip, port: port);

    if (mounted) {
      setState(() => _isConnecting = false);
      if (success) {
        Navigator.pop(context);
      } else {
        setState(() => _errorMessage = 'Could not connect to $ip:$port. Verify ESP32 is powered on.');
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: CroenixColors.darkBackground,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        title: const Text('Connect to Croenix', style: TextStyle(fontWeight: FontWeight.bold)),
        actions: [
          IconButton(
            icon: _isScanning
                ? const SizedBox(
                    width: 20,
                    height: 20,
                    child: CircularProgressIndicator(strokeWidth: 2, color: CroenixColors.primaryCyan),
                  )
                : const Icon(Icons.refresh_rounded, color: CroenixColors.primaryCyan),
            onPressed: _isScanning ? null : _startScan,
          ),
        ],
      ),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // Manual IP Input
              Container(
                padding: const EdgeInsets.all(16),
                decoration: GlassEffect.standard,
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      'Direct IP Connection',
                      style: TextStyle(
                        fontSize: 15,
                        fontWeight: FontWeight.bold,
                        color: CroenixColors.textPrimary,
                      ),
                    ),
                    const SizedBox(height: 6),
                    const Text(
                      'Default AP mode IP is 192.168.4.1',
                      style: TextStyle(fontSize: 12, color: CroenixColors.textSecondary),
                    ),
                    const SizedBox(height: 12),
                    Row(
                      children: [
                        Expanded(
                          child: TextField(
                            controller: _ipController,
                            style: const TextStyle(color: Colors.white, fontSize: 14),
                            keyboardType: TextInputType.number,
                            decoration: InputDecoration(
                              filled: true,
                              fillColor: const Color(0xFF1B2338),
                              hintText: '192.168.4.1',
                              hintStyle: const TextStyle(color: Colors.white38),
                              prefixIcon: const Icon(Icons.lan_rounded, color: CroenixColors.primaryCyan, size: 20),
                              border: OutlineInputBorder(
                                borderRadius: BorderRadius.circular(10),
                                borderSide: BorderSide.none,
                              ),
                              contentPadding: const EdgeInsets.symmetric(horizontal: 12, vertical: 12),
                            ),
                          ),
                        ),
                        const SizedBox(width: 10),
                        ElevatedButton(
                          style: ElevatedButton.styleFrom(
                            backgroundColor: CroenixColors.primaryCyan,
                            foregroundColor: Colors.black,
                            padding: const EdgeInsets.symmetric(horizontal: 18, vertical: 14),
                            shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
                          ),
                          onPressed: _isConnecting
                              ? null
                              : () => _connect(_ipController.text.trim()),
                          child: _isConnecting
                              ? const SizedBox(
                                  width: 16,
                                  height: 16,
                                  child: CircularProgressIndicator(strokeWidth: 2, color: Colors.black),
                                )
                              : const Text('Connect', style: TextStyle(fontWeight: FontWeight.bold)),
                        ),
                      ],
                    ),
                  ],
                ),
              ),

              if (_errorMessage != null) ...[
                const SizedBox(height: 12),
                Container(
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: Colors.red.withValues(alpha: 0.15),
                    borderRadius: BorderRadius.circular(10),
                    border: Border.all(color: Colors.red.withValues(alpha: 0.4)),
                  ),
                  child: Row(
                    children: [
                      const Icon(Icons.error_outline_rounded, color: Colors.redAccent, size: 20),
                      const SizedBox(width: 10),
                      Expanded(
                        child: Text(_errorMessage!, style: const TextStyle(color: Colors.white70, fontSize: 12)),
                      ),
                    ],
                  ),
                ),
              ],

              const SizedBox(height: 24),

              // Discovered Devices List
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  const Text(
                    'DISCOVERED DEVICES',
                    style: TextStyle(
                      fontSize: 12,
                      fontWeight: FontWeight.bold,
                      letterSpacing: 0.8,
                      color: CroenixColors.textSecondary,
                    ),
                  ),
                  if (_isScanning)
                    const Text(
                      'Scanning...',
                      style: TextStyle(fontSize: 12, color: CroenixColors.primaryCyan),
                    ),
                ],
              ),
              const SizedBox(height: 12),

              Expanded(
                child: _devices.isEmpty
                    ? Center(
                        child: Column(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Icon(
                              _isScanning ? Icons.radar_rounded : Icons.search_off_rounded,
                              size: 48,
                              color: CroenixColors.textSecondary.withValues(alpha: 0.5),
                            ),
                            const SizedBox(height: 12),
                            Text(
                              _isScanning ? 'Searching network for Croenix...' : 'No devices found',
                              style: const TextStyle(color: CroenixColors.textSecondary, fontSize: 14),
                            ),
                            const SizedBox(height: 6),
                            const Text(
                              'Ensure ESP32 is powered on and connected to the same Wi-Fi.',
                              textAlign: TextAlign.center,
                              style: TextStyle(color: Colors.white38, fontSize: 12),
                            ),
                          ],
                        ),
                      )
                    : ListView.builder(
                        itemCount: _devices.length,
                        itemBuilder: (context, index) {
                          final device = _devices[index];
                          return Container(
                            margin: const EdgeInsets.only(bottom: 10),
                            decoration: GlassEffect.standard,
                            child: ListTile(
                              leading: Container(
                                padding: const EdgeInsets.all(10),
                                decoration: BoxDecoration(
                                  color: CroenixColors.primaryCyan.withValues(alpha: 0.15),
                                  shape: BoxShape.circle,
                                ),
                                child: const Icon(Icons.speaker_rounded, color: CroenixColors.primaryCyan, size: 20),
                              ),
                              title: Text(
                                device.displayName.isEmpty ? 'Croenix' : device.displayName,
                                style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.white),
                              ),
                              subtitle: Text(
                                '${device.ipAddress}:${device.port}',
                                style: const TextStyle(color: CroenixColors.textSecondary, fontSize: 12),
                              ),
                              trailing: ElevatedButton(
                                style: ElevatedButton.styleFrom(
                                  backgroundColor: CroenixColors.primaryCyan,
                                  foregroundColor: Colors.black,
                                  shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
                                ),
                                onPressed: () => _connect(device.ipAddress, port: device.port),
                                child: const Text('Connect'),
                              ),
                            ),
                          );
                        },
                      ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

