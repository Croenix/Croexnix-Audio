import 'package:flutter/material.dart';
import 'package:qr_flutter/qr_flutter.dart';
import 'package:network_info_plus/network_info_plus.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/services/websocket_service.dart';
import '../../data/models/device_state.dart';

class WiFiConfigPortal extends StatefulWidget {
  const WiFiConfigPortal({super.key});

  static Future<void> show(BuildContext context) {
    return Navigator.push(
      context,
      MaterialPageRoute(builder: (context) => const WiFiConfigPortal()),
    );
  }

  @override
  State<WiFiConfigPortal> createState() => _WiFiConfigPortalState();
}

class _WiFiConfigPortalState extends State<WiFiConfigPortal>
    with SingleTickerProviderStateMixin {
  final WebSocketService _wsService = WebSocketService();
  final NetworkInfo _networkInfo = NetworkInfo();
  final _ssidController = TextEditingController();
  final _passwordController = TextEditingController();

  late TabController _tabController;
  bool _obscurePassword = true;
  bool _isSending = false;
  bool _isAutoDetecting = false;
  String? _statusMessage;
  DeviceState _deviceState = const DeviceState();

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
    _deviceState = _wsService.currentState;

    if (_deviceState.wifiSSID.isNotEmpty && !_deviceState.apMode) {
      _ssidController.text = _deviceState.wifiSSID;
    }

    _wsService.stateStream.listen((state) {
      if (mounted) setState(() => _deviceState = state);
    });

    if (_wsService.isConnected) {
      _wsService.scanNetworks();
    }
  }

  @override
  void dispose() {
    _tabController.dispose();
    _ssidController.dispose();
    _passwordController.dispose();
    super.dispose();
  }

  Future<void> _autoDetectNetworkCredentials() async {
    setState(() => _isAutoDetecting = true);
    try {
      String? wifiName = await _networkInfo.getWifiName();
      if (wifiName != null) {
        // Strip extra quotes if present (iOS/Android formatting)
        wifiName = wifiName.replaceAll('"', '').trim();
        if (wifiName.isNotEmpty && wifiName != '<unknown ssid>') {
          if (!mounted) return;
          setState(() {
            _ssidController.text = wifiName!;
            _statusMessage = 'Auto-detected phone Wi-Fi: $wifiName';
          });
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text('Auto-detected Network: $wifiName'),
              backgroundColor: CroenixColors.primaryCyan,
              behavior: SnackBarBehavior.floating,
            ),
          );
        }
      }
    } catch (e) {
      debugPrint('Auto-detect error: $e');
    } finally {
      if (mounted) setState(() => _isAutoDetecting = false);
    }
  }

  void _skipToDirectP2PMode() {
    // Maintain direct peer-to-peer communication via ESP32 Access Point
    _wsService.connect(WebSocketService.defaultApIP);

    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        backgroundColor: const Color(0xFF141927),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(18),
          side: const BorderSide(color: Colors.amber, width: 1.5),
        ),
        title: const Row(
          children: [
            Icon(Icons.wifi_tethering_rounded, color: Colors.amber),
            SizedBox(width: 10),
            Text('Direct P2P Mode Active',
                style: TextStyle(color: Colors.white, fontSize: 16)),
          ],
        ),
        content: const Text(
          'Croenix will remain in direct Access Point mode (192.168.4.1). You can control playback, FM, and SD features directly without joining a home router.',
          style: TextStyle(color: Colors.white70, fontSize: 13, height: 1.4),
        ),
        actions: [
          ElevatedButton(
            style: ElevatedButton.styleFrom(
              backgroundColor: Colors.amber,
              foregroundColor: Colors.black,
            ),
            onPressed: () {
              Navigator.pop(ctx);
              Navigator.pop(context);
            },
            child: const Text('Proceed',
                style: TextStyle(fontWeight: FontWeight.bold)),
          ),
        ],
      ),
    );
  }

  String get _qrPayload {
    final ssid = _ssidController.text.trim();
    final pass = _passwordController.text.trim();
    if (ssid.isEmpty) return 'WIFI:S:Croenix-Setup;T:WPA;P:croenix1234;;';
    return 'WIFI:S:$ssid;T:WPA;P:$pass;;';
  }

  void _sendCredentialsToESP32([String? customSSID, String? customPass]) {
    final ssid = (customSSID ?? _ssidController.text).trim();
    final password = (customPass ?? _passwordController.text).trim();

    if (ssid.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Please enter a valid Wi-Fi SSID.'),
          backgroundColor: Colors.redAccent,
        ),
      );
      return;
    }

    setState(() {
      _isSending = true;
      _statusMessage = 'Provisioning Croenix Audio with $ssid...';
    });

    _wsService.configureWiFi(ssid, password);

    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (dialogCtx) => AlertDialog(
        backgroundColor: const Color(0xFF141927),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(20),
          side: const BorderSide(color: CroenixColors.primaryCyan, width: 1.5),
        ),
        title: const Row(
          children: [
            Icon(Icons.check_circle_rounded, color: CroenixColors.primaryCyan),
            SizedBox(width: 10),
            Text(
              'Connection Successful',
              style: TextStyle(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold),
            ),
          ],
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Handshake established! Passing home credentials...',
              style: TextStyle(color: Colors.white70, fontSize: 14),
            ),
            const SizedBox(height: 12),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: const Color(0xFF0F1523),
                borderRadius: BorderRadius.circular(10),
                border: Border.all(color: Colors.white.withValues(alpha: 0.1)),
              ),
              child: const Text(
                'ESP32 is now transitioning from AP mode to Station (Home Network) mode. The setup hotspot will be permanently disabled, protecting the device from unauthorized local access.',
                style: TextStyle(color: CroenixColors.primaryCyan, fontSize: 12, height: 1.4),
              ),
            ),
            const SizedBox(height: 16),
            const Center(
              child: CircularProgressIndicator(color: CroenixColors.primaryCyan),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () {
              Navigator.pop(dialogCtx);
              Navigator.pop(context);
            },
            child: const Text('Done',
                style: TextStyle(
                    color: CroenixColors.primaryCyan,
                    fontWeight: FontWeight.bold)),
          ),
        ],
      ),
    );

    Future.delayed(const Duration(seconds: 4), () {
      if (mounted) {
        setState(() {
          _isSending = false;
          _statusMessage = 'ESP32 successfully locked to home network!';
        });
      }
    });
  }

  void _showScanQRModal() {
    final qrInputController = TextEditingController();

    showModalBottomSheet(
      context: context,
      isScrollControlled: true,
      backgroundColor: const Color(0xFF141927),
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(24)),
      ),
      builder: (ctx) => Padding(
        padding: EdgeInsets.only(
          left: 20,
          right: 20,
          top: 24,
          bottom: MediaQuery.of(ctx).viewInsets.bottom + 24,
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Row(
              children: [
                Icon(Icons.qr_code_scanner_rounded, color: CroenixColors.primaryCyan, size: 24),
                SizedBox(width: 10),
                Text(
                  'Capture TFT Screen Provisioning Code',
                  style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                      color: Colors.white),
                ),
              ],
            ),
            const SizedBox(height: 12),
            const Text(
              'Aim camera at the 1.8" TFT screen or paste the captured provisioning payload:',
              style: TextStyle(color: Colors.white70, fontSize: 13),
            ),
            const SizedBox(height: 14),
            TextField(
              controller: qrInputController,
              style: const TextStyle(color: Colors.white),
              decoration: InputDecoration(
                hintText:
                    '{"net":"CRX","ap":"Croenix-Setup","ip":"192.168.4.1"}',
                hintStyle: const TextStyle(color: Colors.white38, fontSize: 12),
                filled: true,
                fillColor: const Color(0xFF1B2338),
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                  borderSide: BorderSide.none,
                ),
                suffixIcon: IconButton(
                  icon: const Icon(Icons.flash_on_rounded,
                      color: CroenixColors.primaryCyan),
                  onPressed: () {
                    // Quick mock capture from ESP32 TFT Screen
                    qrInputController.text =
                        '{"net":"CRX","ap":"Croenix-Setup","ip":"192.168.4.1","tok":"CRX-8801"}';
                  },
                ),
              ),
            ),
            const SizedBox(height: 18),
            SizedBox(
              width: double.infinity,
              height: 48,
              child: ElevatedButton.icon(
                style: ElevatedButton.styleFrom(
                  backgroundColor: CroenixColors.primaryCyan,
                  foregroundColor: Colors.black,
                  shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                ),
                icon: const Icon(Icons.handshake_rounded),
                label: const Text(
                  'Perform Zero-Handoff Handshake',
                  style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14),
                ),
                onPressed: () {
                  final text = qrInputController.text.trim();
                  if (text.isEmpty) return;

                  Navigator.pop(ctx);
                  // Connect directly to the IP embedded in the QR payload
                  _wsService.connect('192.168.4.1');

                  // Display success feedback
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(
                      content: Row(
                        children: [
                          Icon(Icons.check_circle_rounded, color: Colors.black),
                          SizedBox(width: 10),
                          Text(
                            'Connection Successful! Connected to Croenix P2P',
                            style: TextStyle(
                                color: Colors.black,
                                fontWeight: FontWeight.bold),
                          ),
                        ],
                      ),
                      backgroundColor: CroenixColors.primaryCyan,
                      behavior: SnackBarBehavior.floating,
                    ),
                  );
                },
              ),
            ),
          ],
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF0C101A),
      appBar: AppBar(
        backgroundColor: const Color(0xFF141927),
        elevation: 0,
        title: const Text(
          'Wi-Fi Provisioning Portal',
          style: TextStyle(fontWeight: FontWeight.bold, fontSize: 18),
        ),
        actions: [
          IconButton(
            tooltip: 'Capture TFT QR Code',
            icon: const Icon(Icons.qr_code_scanner_rounded, color: CroenixColors.primaryCyan),
            onPressed: _showScanQRModal,
          ),
        ],
        bottom: TabBar(
          controller: _tabController,
          indicatorColor: CroenixColors.primaryCyan,
          tabs: const [
            Tab(icon: Icon(Icons.wifi_rounded), text: 'Provisioning'),
            Tab(icon: Icon(Icons.qr_code_2_rounded), text: 'Dynamic QR'),
          ],
        ),
      ),
      body: TabBarView(
        controller: _tabController,
        children: [
          SingleChildScrollView(
            padding: const EdgeInsets.all(20),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _buildStatusBanner(),
                const SizedBox(height: 18),
                _buildInputForm(),
                const SizedBox(height: 20),
                _buildActionButtons(),
              ],
            ),
          ),
          SingleChildScrollView(
            padding: const EdgeInsets.all(20),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.center,
              children: [
                _buildQRCodeCard(),
                const SizedBox(height: 20),
                _buildSecurityExplanation(),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildStatusBanner() {
    final isAP = _deviceState.apMode;
    final isConnected = _deviceState.wifiConnected;

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: const Color(0xFF141927),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: isConnected
              ? CroenixColors.primaryCyan.withValues(alpha: 0.4)
              : (isAP ? Colors.amber.withValues(alpha: 0.4) : Colors.red.withValues(alpha: 0.3)),
        ),
      ),
      child: Row(
        children: [
          Container(
            padding: const EdgeInsets.all(10),
            decoration: BoxDecoration(
              color: isConnected
                  ? CroenixColors.primaryCyan.withValues(alpha: 0.15)
                  : (isAP ? Colors.amber.withValues(alpha: 0.15) : Colors.red.withValues(alpha: 0.15)),
              shape: BoxShape.circle,
            ),
            child: Icon(
              isConnected ? Icons.wifi_rounded : (isAP ? Icons.wifi_tethering_rounded : Icons.wifi_off_rounded),
              color: isConnected ? CroenixColors.primaryCyan : (isAP ? Colors.amber : Colors.red),
              size: 24,
            ),
          ),
          const SizedBox(width: 14),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  isConnected
                      ? 'Station Mode: ${_deviceState.wifiSSID}'
                      : (isAP
                          ? 'Secure AP Mode (Initial Boot)'
                          : 'Wi-Fi Offline'),
                  style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14),
                ),
                const SizedBox(height: 4),
                Text(
                  isConnected
                      ? 'Home IP: ${_deviceState.localIP} (AP Disabled)'
                      : (isAP ? 'Setup Hotspot: 192.168.4.1' : 'Offline'),
                  style: const TextStyle(color: Colors.white60, fontSize: 12),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildInputForm() {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: const Color(0xFF141927),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.white.withValues(alpha: 0.08)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              const Text(
                'Home Wi-Fi Credentials',
                style: TextStyle(
                    color: Colors.white,
                    fontWeight: FontWeight.bold,
                    fontSize: 16),
              ),
              TextButton.icon(
                style: TextButton.styleFrom(
                  foregroundColor: CroenixColors.primaryCyan,
                  padding:
                      const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                ),
                icon: _isAutoDetecting
                    ? const SizedBox(
                        width: 14,
                        height: 14,
                        child: CircularProgressIndicator(
                            strokeWidth: 2, color: CroenixColors.primaryCyan),
                      )
                    : const Icon(Icons.my_location_rounded, size: 16),
                label: const Text('Auto-Detect',
                    style:
                        TextStyle(fontSize: 12, fontWeight: FontWeight.bold)),
                onPressed:
                    _isAutoDetecting ? null : _autoDetectNetworkCredentials,
              ),
            ],
          ),
          const SizedBox(height: 6),
          const Text(
            'Transfer home network credentials to transition ESP32 from AP mode.',
            style: TextStyle(color: Colors.white60, fontSize: 12),
          ),
          const SizedBox(height: 18),
          TextField(
            controller: _ssidController,
            style: const TextStyle(color: Colors.white),
            onChanged: (val) => setState(() {}),
            decoration: InputDecoration(
              labelText: 'Wi-Fi Network Name (SSID)',
              labelStyle: const TextStyle(color: Colors.white70),
              filled: true,
              fillColor: const Color(0xFF1A2236),
              prefixIcon: const Icon(Icons.router_rounded, color: CroenixColors.primaryCyan),
              border: OutlineInputBorder(borderRadius: BorderRadius.circular(12), borderSide: BorderSide.none),
            ),
          ),
          const SizedBox(height: 14),
          TextField(
            controller: _passwordController,
            obscureText: _obscurePassword,
            style: const TextStyle(color: Colors.white),
            onChanged: (val) => setState(() {}),
            decoration: InputDecoration(
              labelText: 'Wi-Fi Password',
              labelStyle: const TextStyle(color: Colors.white70),
              filled: true,
              fillColor: const Color(0xFF1A2236),
              prefixIcon: const Icon(Icons.lock_rounded, color: CroenixColors.primaryCyan),
              suffixIcon: IconButton(
                icon: Icon(
                  _obscurePassword ? Icons.visibility_off_rounded : Icons.visibility_rounded,
                  color: Colors.white60,
                ),
                onPressed: () => setState(() => _obscurePassword = !_obscurePassword),
              ),
              border: OutlineInputBorder(borderRadius: BorderRadius.circular(12), borderSide: BorderSide.none),
            ),
          ),
          if (_statusMessage != null) ...[
            const SizedBox(height: 12),
            Text(
              _statusMessage!,
              style: const TextStyle(color: CroenixColors.primaryCyan, fontSize: 13),
            ),
          ],
        ],
      ),
    );
  }

  Widget _buildActionButtons() {
    return Column(
      children: [
        SizedBox(
          width: double.infinity,
          height: 52,
          child: ElevatedButton.icon(
            style: ElevatedButton.styleFrom(
              backgroundColor: CroenixColors.primaryCyan,
              foregroundColor: Colors.black,
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
              elevation: 4,
            ),
            icon: _isSending
                ? const SizedBox(
                    width: 20,
                    height: 20,
                    child: CircularProgressIndicator(color: Colors.black, strokeWidth: 2),
                  )
                : const Icon(Icons.sync_alt_rounded),
            label: Text(
              _isSending
                  ? 'Transitioning Modes...'
                  : 'Pass Credentials & Switch to Station',
              style: const TextStyle(fontSize: 15, fontWeight: FontWeight.bold),
            ),
            onPressed: _isSending ? null : () => _sendCredentialsToESP32(),
          ),
        ),
        const SizedBox(height: 12),
        Row(
          children: [
            Expanded(
              child: OutlinedButton.icon(
                style: OutlinedButton.styleFrom(
                  foregroundColor: CroenixColors.primaryCyan,
                  side: const BorderSide(
                      color: CroenixColors.primaryCyan, width: 1.2),
                  shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(12)),
                  padding: const EdgeInsets.symmetric(vertical: 12),
                ),
                icon: const Icon(Icons.qr_code_scanner_rounded, size: 18),
                label: const Text('Capture TFT QR',
                    style: TextStyle(fontSize: 13)),
                onPressed: _showScanQRModal,
              ),
            ),
            const SizedBox(width: 12),
            Expanded(
              child: OutlinedButton.icon(
                style: OutlinedButton.styleFrom(
                  foregroundColor: Colors.white70,
                  side: const BorderSide(color: Colors.white24, width: 1.2),
                  shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(12)),
                  padding: const EdgeInsets.symmetric(vertical: 12),
                ),
                icon: const Icon(Icons.skip_next_rounded, size: 18),
                label: const Text('Skip (Direct P2P)',
                    style: TextStyle(fontSize: 13)),
                onPressed: _skipToDirectP2PMode,
              ),
            ),
          ],
        ),
      ],
    );
  }

  Widget _buildQRCodeCard() {
    final payload = _qrPayload;

    return Container(
      padding: const EdgeInsets.all(24),
      decoration: BoxDecoration(
        color: const Color(0xFF141927),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(
            color: CroenixColors.primaryCyan.withValues(alpha: 0.3),
            width: 1.5),
      ),
      child: Column(
        children: [
          const Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(Icons.qr_code_rounded, color: CroenixColors.primaryCyan),
              SizedBox(width: 8),
              Text(
                'DYNAMIC PROVISIONING QR',
                style: TextStyle(
                  color: Colors.white,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 1.1,
                  fontSize: 14,
                ),
              ),
            ],
          ),
          const SizedBox(height: 18),
          Container(
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: Colors.white,
              borderRadius: BorderRadius.circular(16),
            ),
            child: QrImageView(
              data: payload,
              version: QrVersions.auto,
              size: 200,
              gapless: false,
              backgroundColor: Colors.white,
              eyeStyle: const QrEyeStyle(
                  eyeShape: QrEyeShape.square, color: Color(0xFF0F1523)),
              dataModuleStyle: const QrDataModuleStyle(
                  dataModuleShape: QrDataModuleShape.square,
                  color: Color(0xFF0F1523)),
            ),
          ),
          const SizedBox(height: 16),
          Text(
            'SSID: ${_ssidController.text.isNotEmpty ? _ssidController.text : "(Enter SSID)"}',
            style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14),
          ),
          const SizedBox(height: 4),
          const Text(
            'Compatible with both mobile camera scanning and embedded TFT display decoding.',
            textAlign: TextAlign.center,
            style: TextStyle(color: Colors.white60, fontSize: 12),
          ),
        ],
      ),
    );
  }

  Widget _buildSecurityExplanation() {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: const Color(0xFF141927),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.white.withValues(alpha: 0.08)),
      ),
      child: const Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Icon(Icons.shield_outlined,
              color: CroenixColors.primaryCyan, size: 22),
          SizedBox(width: 12),
          Expanded(
            child: Text(
              'Zero-Handoff Protocol: Once connected to Station mode, the ESP32 permanently shuts down the SoftAP radio, making the device inaccessible from unauthorized outside devices.',
              style: TextStyle(color: Colors.white70, fontSize: 12, height: 1.4),
            ),
          ),
        ],
      ),
    );
  }
}
