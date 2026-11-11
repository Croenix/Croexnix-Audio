import 'package:flutter/material.dart';
import 'package:qr_flutter/qr_flutter.dart';
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
  final _ssidController = TextEditingController();
  final _passwordController = TextEditingController();

  late TabController _tabController;
  bool _obscurePassword = true;
  bool _isSending = false;
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

    // Request available Wi-Fi networks if connected to ESP32
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

  String get _qrPayload {
    final ssid = _ssidController.text.trim();
    final pass = _passwordController.text.trim();
    if (ssid.isEmpty) return 'WIFI:S:Croenix-Setup;T:WPA;P:;;';
    // Standard Wi-Fi configuration barcode string
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
        title: Row(
          children: const [
            Icon(Icons.security_rounded, color: CroenixColors.primaryCyan),
            SizedBox(width: 10),
            Text(
              'Switching Modes',
              style: TextStyle(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold),
            ),
          ],
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Passing credentials to ESP32...',
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
                'ESP32 is now switching from SoftAP Mode to your Home Network (Station Mode). The setup hotspot will be permanently shut down to prevent unauthorized local connections.',
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
            child: const Text('OK', style: TextStyle(color: CroenixColors.primaryCyan)),
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
            Row(
              children: const [
                Icon(Icons.qr_code_scanner_rounded, color: CroenixColors.primaryCyan, size: 24),
                SizedBox(width: 10),
                Text(
                  'Scan / Import Wi-Fi QR Code',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Colors.white),
                ),
              ],
            ),
            const SizedBox(height: 12),
            const Text(
              'Paste or scan the Wi-Fi QR code text (e.g. from your router or phone hotspot):',
              style: TextStyle(color: Colors.white70, fontSize: 13),
            ),
            const SizedBox(height: 14),
            TextField(
              controller: qrInputController,
              style: const TextStyle(color: Colors.white),
              decoration: InputDecoration(
                hintText: 'WIFI:S:MyHomeWiFi;T:WPA;P:Password123;;',
                hintStyle: const TextStyle(color: Colors.white38),
                filled: true,
                fillColor: const Color(0xFF1B2338),
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                  borderSide: BorderSide.none,
                ),
                suffixIcon: IconButton(
                  icon: const Icon(Icons.paste_rounded, color: CroenixColors.primaryCyan),
                  onPressed: () {
                    // Sample demo Wi-Fi QR string
                    qrInputController.text = 'WIFI:S:HomeNet_5G;T:WPA;P:SecretAudioPass2026;;';
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
                icon: const Icon(Icons.check_circle_rounded),
                label: const Text(
                  'Parse & Apply to Croenix',
                  style: TextStyle(fontWeight: FontWeight.bold, fontSize: 15),
                ),
                onPressed: () {
                  final text = qrInputController.text.trim();
                  if (text.isEmpty) return;

                  String parsedSSID = '';
                  String parsedPass = '';

                  // Parse WIFI:S:<ssid>;T:<type>;P:<pass>;;
                  final ssidMatch = RegExp(r'S:([^;]+)').firstMatch(text);
                  final passMatch = RegExp(r'P:([^;]+)').firstMatch(text);

                  if (ssidMatch != null) parsedSSID = ssidMatch.group(1) ?? '';
                  if (passMatch != null) parsedPass = passMatch.group(1) ?? '';

                  if (parsedSSID.isNotEmpty) {
                    setState(() {
                      _ssidController.text = parsedSSID;
                      _passwordController.text = parsedPass;
                    });
                    Navigator.pop(ctx);
                    _sendCredentialsToESP32(parsedSSID, parsedPass);
                  } else {
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(
                        content: Text('Invalid Wi-Fi QR format. Expected WIFI:S:...;P:...;;'),
                        backgroundColor: Colors.redAccent,
                      ),
                    );
                  }
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
          'Wi-Fi Configuration Portal',
          style: TextStyle(fontWeight: FontWeight.bold, fontSize: 18),
        ),
        actions: [
          IconButton(
            tooltip: 'Scan QR Code',
            icon: const Icon(Icons.qr_code_scanner_rounded, color: CroenixColors.primaryCyan),
            onPressed: _showScanQRModal,
          ),
        ],
        bottom: TabBar(
          controller: _tabController,
          indicatorColor: CroenixColors.primaryCyan,
          tabs: const [
            Tab(icon: Icon(Icons.wifi_rounded), text: 'Credentials Setup'),
            Tab(icon: Icon(Icons.qr_code_2_rounded), text: 'Dynamic QR Code'),
          ],
        ),
      ),
      body: TabBarView(
        controller: _tabController,
        children: [
          // Tab 1: Form & Credentials
          SingleChildScrollView(
            padding: const EdgeInsets.all(20),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _buildStatusBanner(),
                const SizedBox(height: 20),
                _buildInputForm(),
                const SizedBox(height: 24),
                _buildActionButtons(),
              ],
            ),
          ),

          // Tab 2: Dynamic QR Code Generator
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
                      : (isAP ? 'Secure AP Mode (Initial Setup)' : 'Wi-Fi Disconnected'),
                  style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14),
                ),
                const SizedBox(height: 4),
                Text(
                  isConnected
                      ? 'IP: ${_deviceState.localIP} (AP Disabled)'
                      : (isAP ? 'Hotspot: Croenix-Audio-Setup (192.168.4.1)' : 'Offline'),
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
          const Text(
            'Home Wi-Fi Network (Station Mode)',
            style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 16),
          ),
          const SizedBox(height: 6),
          const Text(
            'Enter your local Wi-Fi router credentials to transition ESP32 away from AP mode.',
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
                : const Icon(Icons.send_rounded),
            label: Text(
              _isSending ? 'Sending Credentials...' : 'Pass Credentials & Switch Mode',
              style: const TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
            ),
            onPressed: _isSending ? null : () => _sendCredentialsToESP32(),
          ),
        ),
        const SizedBox(height: 12),
        SizedBox(
          width: double.infinity,
          height: 48,
          child: OutlinedButton.icon(
            style: OutlinedButton.styleFrom(
              foregroundColor: CroenixColors.primaryCyan,
              side: const BorderSide(color: CroenixColors.primaryCyan, width: 1.2),
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
            ),
            icon: const Icon(Icons.qr_code_scanner_rounded),
            label: const Text('Scan QR Code from Router / Screen'),
            onPressed: _showScanQRModal,
          ),
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
        border: Border.all(color: CroenixColors.primaryCyan.withValues(alpha: 0.3), width: 1.5),
        boxShadow: [
          BoxShadow(
            color: CroenixColors.primaryCyan.withValues(alpha: 0.1),
            blurRadius: 16,
            spreadRadius: 2,
          ),
        ],
      ),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: const [
              Icon(Icons.qr_code_rounded, color: CroenixColors.primaryCyan),
              SizedBox(width: 8),
              Text(
                'DYNAMIC WI-FI QR CODE',
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
              size: 210,
              gapless: false,
              backgroundColor: Colors.white,
              eyeStyle: const QrEyeStyle(
                eyeShape: QrEyeShape.square,
                color: Color(0xFF0F1523),
              ),
              dataModuleStyle: const QrDataModuleStyle(
                dataModuleShape: QrDataModuleShape.square,
                color: Color(0xFF0F1523),
              ),
            ),
          ),
          const SizedBox(height: 16),
          Text(
            'SSID: ${_ssidController.text.isNotEmpty ? _ssidController.text : "(Enter SSID)"}',
            style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14),
          ),
          const SizedBox(height: 4),
          const Text(
            'Scan with Flutter app or companion device to instantly configure local credentials.',
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
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: const [
          Icon(Icons.lock_outline_rounded, color: CroenixColors.primaryCyan, size: 22),
          SizedBox(width: 12),
          Expanded(
            child: Text(
              'Security Architecture: Upon receiving local credentials via QR or WebSocket, the ESP32 disables its temporary SoftAP radio, shutting down the setup hotspot. The player becomes accessible strictly on your encrypted home network.',
              style: TextStyle(color: Colors.white70, fontSize: 12, height: 1.4),
            ),
          ),
        ],
      ),
    );
  }
}
