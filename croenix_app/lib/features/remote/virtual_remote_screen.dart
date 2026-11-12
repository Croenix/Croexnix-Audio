import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/models/device_state.dart';
import '../../data/services/websocket_service.dart';

class VirtualRemoteScreen extends StatefulWidget {
  const VirtualRemoteScreen({super.key});

  @override
  State<VirtualRemoteScreen> createState() => _VirtualRemoteScreenState();
}

class _VirtualRemoteScreenState extends State<VirtualRemoteScreen>
    with SingleTickerProviderStateMixin {
  final WebSocketService _ws = WebSocketService();
  bool _irTransmitting = false;
  Timer? _irTimer;

  // Animation controller for glowing IR blast indicator
  late AnimationController _irAnimController;

  @override
  void initState() {
    super.initState();
    _irAnimController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 300),
    );
  }

  @override
  void dispose() {
    _irTimer?.cancel();
    _irAnimController.dispose();
    super.dispose();
  }

  void _triggerHapticAndIR() {
    HapticFeedback.mediumImpact();
    setState(() => _irTransmitting = true);
    _irAnimController.forward(from: 0.0);
    _irTimer?.cancel();
    _irTimer = Timer(const Duration(milliseconds: 250), () {
      if (mounted) setState(() => _irTransmitting = false);
    });
  }

  // 16 Hardware TFT Screen definitions matching the ESP32 firmware
  final List<Map<String, dynamic>> _tftScreens = [
    {'id': 0, 'name': 'Boot Splash', 'icon': Icons.flash_on_rounded},
    {'id': 1, 'name': 'Home Dashboard', 'icon': Icons.dashboard_rounded},
    {'id': 2, 'name': 'Now Playing Player', 'icon': Icons.music_note_rounded},
    {'id': 3, 'name': 'SD Card Browser', 'icon': Icons.folder_rounded},
    {'id': 4, 'name': 'Song ID3 Metadata', 'icon': Icons.info_outline_rounded},
    {'id': 5, 'name': 'FM Radio Tuner', 'icon': Icons.radio_rounded},
    {'id': 6, 'name': 'Web FM Stations', 'icon': Icons.cell_tower_rounded},
    {'id': 7, 'name': 'SoundCloud Player', 'icon': Icons.cloud_rounded},
    {'id': 8, 'name': 'Bluetooth Audio', 'icon': Icons.bluetooth_rounded},
    {'id': 9, 'name': 'USB Flash Drive', 'icon': Icons.usb_rounded},
    {'id': 10, 'name': 'AUX In External', 'icon': Icons.input_rounded},
    {'id': 11, 'name': '5-Band Equalizer', 'icon': Icons.equalizer_rounded},
    {'id': 12, 'name': 'System Settings', 'icon': Icons.settings_rounded},
    {'id': 13, 'name': 'Device Hardware Info', 'icon': Icons.memory_rounded},
    {'id': 14, 'name': 'Mini Audio Player', 'icon': Icons.play_circle_fill_rounded},
    {'id': 15, 'name': 'Master Volume HUD', 'icon': Icons.volume_up_rounded},
  ];

  void _showScreenSelector(BuildContext context) {
    _triggerHapticAndIR();
    showModalBottomSheet(
      context: context,
      backgroundColor: const Color(0xFF0F1523),
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(24)),
      ),
      builder: (ctx) {
        return SafeArea(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Container(
                margin: const EdgeInsets.symmetric(vertical: 10),
                width: 40,
                height: 4,
                decoration: BoxDecoration(
                  color: Colors.white24,
                  borderRadius: BorderRadius.circular(2),
                ),
              ),
              const Padding(
                padding: EdgeInsets.symmetric(horizontal: 20, vertical: 8),
                child: Row(
                  children: [
                    Icon(Icons.tv_rounded, color: CroenixColors.primaryCyan, size: 20),
                    SizedBox(width: 10),
                    Text(
                      'ESP32 TFT SCREEN SELECTOR',
                      style: TextStyle(
                        color: Colors.white,
                        fontWeight: FontWeight.bold,
                        fontSize: 14,
                        letterSpacing: 1.2,
                      ),
                    ),
                  ],
                ),
              ),
              const Divider(color: Colors.white10),
              Flexible(
                child: ListView.builder(
                  shrinkWrap: true,
                  itemCount: _tftScreens.length,
                  itemBuilder: (context, idx) {
                    final s = _tftScreens[idx];
                    return ListTile(
                      dense: true,
                      leading: Icon(s['icon'] as IconData, color: CroenixColors.primaryCyan, size: 20),
                      title: Text(
                        s['name'] as String,
                        style: const TextStyle(color: Colors.white, fontSize: 13),
                      ),
                      trailing: Text(
                        '#${s['id']}',
                        style: const TextStyle(color: Colors.white38, fontSize: 11, fontFamily: 'monospace'),
                      ),
                      onTap: () {
                        _ws.setTftScreen(s['id'] as int);
                        _triggerHapticAndIR();
                        Navigator.pop(ctx);
                        ScaffoldMessenger.of(context).showSnackBar(
                          SnackBar(
                            content: Text('Switched ESP32 display to: ${s['name']}'),
                            duration: const Duration(seconds: 1),
                            backgroundColor: CroenixColors.darkSurfaceVariant,
                          ),
                        );
                      },
                    );
                  },
                ),
              ),
            ],
          ),
        );
      },
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF070A11),
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        centerTitle: true,
        leading: IconButton(
          icon: const Icon(Icons.arrow_back_ios_new_rounded, color: Colors.white70, size: 18),
          onPressed: () => Navigator.of(context).pop(),
        ),
        title: const Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.settings_remote_rounded, color: CroenixColors.primaryCyan, size: 18),
            SizedBox(width: 8),
            Text(
              'CROENIX REMOTE',
              style: TextStyle(
                color: Colors.white,
                fontWeight: FontWeight.bold,
                fontSize: 15,
                letterSpacing: 1.5,
              ),
            ),
          ],
        ),
        actions: [
          // Simulated IR / RF Transmit indicator LED
          Padding(
            padding: const EdgeInsets.only(right: 18),
            child: Center(
              child: AnimatedContainer(
                duration: const Duration(milliseconds: 200),
                width: 12,
                height: 12,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  color: _irTransmitting ? CroenixColors.primaryCyan : Colors.white12,
                  boxShadow: _irTransmitting
                      ? [
                          BoxShadow(
                            color: CroenixColors.primaryCyan.withOpacity(0.9),
                            blurRadius: 10,
                            spreadRadius: 3,
                          ),
                        ]
                      : [],
                ),
              ),
            ),
          ),
        ],
      ),
      body: StreamBuilder<DeviceState>(
        stream: _ws.stateStream,
        initialData: _ws.currentState,
        builder: (context, snapshot) {
          final state = snapshot.data ?? const DeviceState();
          return SafeArea(
            child: SingleChildScrollView(
              physics: const BouncingScrollPhysics(),
              padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
              child: Column(
                children: [
                  // 1. Sleek OLED Display Window
                  _buildOledDisplay(state),
                  const SizedBox(height: 16),

                  // 2. Remote Body Container (Chassis)
                  _buildRemoteChassis(context, state),
                  const SizedBox(height: 30),
                ],
              ),
            ),
          );
        },
      ),
    );
  }

  // --- OLED Screen Simulator ---
  Widget _buildOledDisplay(DeviceState state) {
    String sourceName = 'IDLE';
    Color sourceColor = CroenixColors.primaryCyan;
    switch (state.currentSource) {
      case AudioSource.sdCard:
        sourceName = 'SD CARD';
        sourceColor = CroenixColors.sourceSDCard;
        break;
      case AudioSource.bluetooth:
        sourceName = 'BLUETOOTH';
        sourceColor = CroenixColors.sourceBluetooth;
        break;
      case AudioSource.fmRadio:
        sourceName = 'FM RADIO (${state.fmFrequency.toStringAsFixed(1)} MHz)';
        sourceColor = CroenixColors.sourceFM;
        break;
      case AudioSource.webStream:
        sourceName = 'SOUNDCLOUD';
        sourceColor = CroenixColors.soundCloudOrange;
        break;
      default:
        sourceName = 'CROENIX CORE';
        sourceColor = CroenixColors.primaryCyan;
    }

    final isOnline = _ws.isConnected;

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(14),
      decoration: BoxDecoration(
        color: const Color(0xFF03070E),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.cyanAccent.withOpacity(0.35), width: 1.5),
        boxShadow: [
          BoxShadow(
            color: Colors.cyanAccent.withOpacity(0.08),
            blurRadius: 16,
            spreadRadius: 2,
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Top status row
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Row(
                children: [
                  Container(
                    width: 7,
                    height: 7,
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      color: isOnline ? CroenixColors.success : Colors.redAccent,
                    ),
                  ),
                  const SizedBox(width: 6),
                  Text(
                    isOnline ? 'ONLINE • ESP32' : 'OFFLINE',
                    style: TextStyle(
                      fontFamily: 'monospace',
                      fontSize: 10,
                      fontWeight: FontWeight.bold,
                      color: isOnline ? CroenixColors.success : Colors.redAccent,
                    ),
                  ),
                ],
              ),
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
                decoration: BoxDecoration(
                  color: sourceColor.withOpacity(0.2),
                  borderRadius: BorderRadius.circular(6),
                  border: Border.all(color: sourceColor.withOpacity(0.5)),
                ),
                child: Text(
                  sourceName,
                  style: TextStyle(
                    fontFamily: 'monospace',
                    fontSize: 9,
                    fontWeight: FontWeight.bold,
                    color: sourceColor,
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 10),

          // Track Title ticker
          Text(
            state.trackTitle.isNotEmpty ? state.trackTitle : 'Croenix Lossless Receiver',
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
            style: const TextStyle(
              fontFamily: 'monospace',
              fontSize: 14,
              fontWeight: FontWeight.bold,
              color: Colors.white,
              letterSpacing: 0.5,
            ),
          ),
          const SizedBox(height: 3),
          Text(
            state.trackArtist.isNotEmpty ? state.trackArtist : 'Dual-Mode BT + WiFi',
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
            style: const TextStyle(
              fontFamily: 'monospace',
              fontSize: 11,
              color: Colors.cyanAccent,
            ),
          ),
          const SizedBox(height: 10),

          // Bottom telemetry bar
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'VOL: ${state.volume}%',
                style: const TextStyle(
                  fontFamily: 'monospace',
                  fontSize: 10,
                  color: Colors.white70,
                ),
              ),
              Text(
                state.isPlaying ? '▶ PLAYING' : '❚❚ PAUSED',
                style: TextStyle(
                  fontFamily: 'monospace',
                  fontSize: 10,
                  fontWeight: FontWeight.bold,
                  color: state.isPlaying ? Colors.greenAccent : Colors.amberAccent,
                ),
              ),
              const Text(
                '44.1kHz • 16-bit',
                style: TextStyle(
                  fontFamily: 'monospace',
                  fontSize: 10,
                  color: Colors.white38,
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  // --- Remote Hardware Chassis Container ---
  Widget _buildRemoteChassis(BuildContext context, DeviceState state) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 20),
      decoration: BoxDecoration(
        color: const Color(0xFF101420),
        borderRadius: BorderRadius.circular(30),
        border: Border.all(color: Colors.white.withOpacity(0.12), width: 1.5),
        boxShadow: const [
          BoxShadow(
            color: Colors.black87,
            blurRadius: 25,
            offset: Offset(0, 10),
          ),
        ],
      ),
      child: Column(
        children: [
          // --- ROW 1: Power, TFT Screen Selector, Mute ---
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildRoundButton(
                icon: Icons.power_settings_new_rounded,
                color: Colors.redAccent,
                label: 'POWER',
                onPressed: () {
                  _triggerHapticAndIR();
                  _ws.togglePlayPause();
                },
              ),
              _buildRoundButton(
                icon: Icons.tv_rounded,
                color: CroenixColors.primaryCyan,
                label: 'SCREEN',
                onPressed: () => _showScreenSelector(context),
              ),
              _buildRoundButton(
                icon: Icons.volume_off_rounded,
                color: Colors.amberAccent,
                label: 'MUTE',
                onPressed: () {
                  _triggerHapticAndIR();
                  _ws.toggleMute();
                },
              ),
            ],
          ),
          const SizedBox(height: 22),

          // --- SECTION 2: Tactile Rotary Encoder D-Pad ---
          _buildTactileDpad(context, state),
          const SizedBox(height: 22),

          // --- SECTION 3: Volume Stepper & Tone Controls ---
          _buildVolumeAndToneDeck(state),
          const SizedBox(height: 22),

          // --- SECTION 4: Media Transport Buttons ---
          _buildMediaTransport(state),
          const SizedBox(height: 24),

          // --- SECTION 5: Source Selector Matrix ---
          _buildSourceGrid(state),
          const SizedBox(height: 20),

          // --- SECTION 6: Numeric Preset Keypad (1 - 6) ---
          _buildPresetKeypad(state),
        ],
      ),
    );
  }

  // --- Tactile D-Pad Controller ---
  Widget _buildTactileDpad(BuildContext context, DeviceState state) {
    return Column(
      children: [
        const Text(
          'ROTARY JOG CONTROLLER',
          style: TextStyle(
            fontSize: 10,
            fontWeight: FontWeight.bold,
            letterSpacing: 1.5,
            color: Colors.white38,
          ),
        ),
        const SizedBox(height: 12),
        Container(
          width: 210,
          height: 210,
          decoration: BoxDecoration(
            shape: BoxShape.circle,
            color: const Color(0xFF0A0D16),
            border: Border.all(color: Colors.cyanAccent.withOpacity(0.3), width: 2),
            boxShadow: [
              BoxShadow(
                color: Colors.cyanAccent.withOpacity(0.08),
                blurRadius: 18,
                spreadRadius: 2,
              ),
            ],
          ),
          child: Stack(
            alignment: Alignment.center,
            children: [
              // Up Button
              Positioned(
                top: 8,
                child: IconButton(
                  icon: const Icon(Icons.keyboard_arrow_up_rounded, color: Colors.white, size: 36),
                  onPressed: () {
                    _triggerHapticAndIR();
                    _ws.sendEncoderStep(-1);
                    _ws.stepVolume(5);
                  },
                ),
              ),
              // Down Button
              Positioned(
                bottom: 8,
                child: IconButton(
                  icon: const Icon(Icons.keyboard_arrow_down_rounded, color: Colors.white, size: 36),
                  onPressed: () {
                    _triggerHapticAndIR();
                    _ws.sendEncoderStep(1);
                    _ws.stepVolume(-5);
                  },
                ),
              ),
              // Left Button
              Positioned(
                left: 8,
                child: IconButton(
                  icon: const Icon(Icons.keyboard_arrow_left_rounded, color: Colors.white, size: 36),
                  onPressed: () {
                    _triggerHapticAndIR();
                    if (state.currentSource == AudioSource.fmRadio) {
                      _ws.seekFMDown();
                    } else {
                      _ws.previousTrack();
                    }
                  },
                ),
              ),
              // Right Button
              Positioned(
                right: 8,
                child: IconButton(
                  icon: const Icon(Icons.keyboard_arrow_right_rounded, color: Colors.white, size: 36),
                  onPressed: () {
                    _triggerHapticAndIR();
                    if (state.currentSource == AudioSource.fmRadio) {
                      _ws.seekFMUp();
                    } else {
                      _ws.nextTrack();
                    }
                  },
                ),
              ),
              // Center OK / Encoder Click Button
              GestureDetector(
                onTap: () {
                  _triggerHapticAndIR();
                  _ws.sendEncoderClick();
                  _ws.togglePlayPause();
                },
                child: Container(
                  width: 74,
                  height: 74,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    gradient: const LinearGradient(
                      begin: Alignment.topLeft,
                      end: Alignment.bottomRight,
                      colors: [Color(0xFF00D9FF), Color(0xFF8B5CF6)],
                    ),
                    boxShadow: [
                      BoxShadow(
                        color: CroenixColors.primaryCyan.withOpacity(0.4),
                        blurRadius: 14,
                      ),
                    ],
                  ),
                  child: const Center(
                    child: Text(
                      'OK',
                      style: TextStyle(
                        color: Colors.white,
                        fontWeight: FontWeight.bold,
                        fontSize: 16,
                        letterSpacing: 1.2,
                      ),
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }

  // --- Volume Stepper & Tone Controls ---
  Widget _buildVolumeAndToneDeck(DeviceState state) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 12),
      decoration: BoxDecoration(
        color: const Color(0xFF0C101A),
        borderRadius: BorderRadius.circular(18),
        border: Border.all(color: Colors.white10),
      ),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              const Text(
                'MASTER VOLUME',
                style: TextStyle(color: Colors.white54, fontSize: 11, fontWeight: FontWeight.bold),
              ),
              Text(
                '${state.volume}%',
                style: const TextStyle(
                  color: CroenixColors.primaryCyan,
                  fontSize: 14,
                  fontWeight: FontWeight.bold,
                  fontFamily: 'monospace',
                ),
              ),
            ],
          ),
          const SizedBox(height: 8),
          Row(
            children: [
              Expanded(
                child: ElevatedButton.icon(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: const Color(0xFF192032),
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.symmetric(vertical: 12),
                    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                  ),
                  icon: const Icon(Icons.remove_rounded, size: 18),
                  label: const Text('VOL -', style: TextStyle(fontWeight: FontWeight.bold)),
                  onPressed: () {
                    _triggerHapticAndIR();
                    _ws.stepVolume(-5);
                  },
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: ElevatedButton.icon(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: CroenixColors.primaryCyanDark,
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.symmetric(vertical: 12),
                    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                  ),
                  icon: const Icon(Icons.add_rounded, size: 18),
                  label: const Text('VOL +', style: TextStyle(fontWeight: FontWeight.bold)),
                  onPressed: () {
                    _triggerHapticAndIR();
                    _ws.stepVolume(5);
                  },
                ),
              ),
            ],
          ),
          const SizedBox(height: 10),
          // Tone Presets
          Row(
            children: [
              _buildToneChip('FLAT', () => _ws.setHardwareTone(0, 0, 0)),
              const SizedBox(width: 6),
              _buildToneChip('BASS +6', () => _ws.setHardwareTone(6, 0, 0)),
              const SizedBox(width: 6),
              _buildToneChip('VOCAL', () => _ws.setHardwareTone(-2, 4, 1)),
              const SizedBox(width: 6),
              _buildToneChip('TREBLE +', () => _ws.setHardwareTone(0, 0, 5)),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildToneChip(String label, VoidCallback onTap) {
    return Expanded(
      child: GestureDetector(
        onTap: () {
          _triggerHapticAndIR();
          onTap();
        },
        child: Container(
          padding: const EdgeInsets.symmetric(vertical: 6),
          decoration: BoxDecoration(
            color: const Color(0xFF151C2C),
            borderRadius: BorderRadius.circular(8),
            border: Border.all(color: Colors.white12),
          ),
          child: Center(
            child: Text(
              label,
              style: const TextStyle(color: Colors.white70, fontSize: 10, fontWeight: FontWeight.bold),
            ),
          ),
        ),
      ),
    );
  }

  // --- Media Transport Deck ---
  Widget _buildMediaTransport(DeviceState state) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
      children: [
        _buildTransportBtn(
          icon: Icons.skip_previous_rounded,
          size: 26,
          onTap: () {
            _triggerHapticAndIR();
            _ws.previousTrack();
          },
        ),
        _buildTransportBtn(
          icon: Icons.fast_rewind_rounded,
          size: 24,
          onTap: () {
            _triggerHapticAndIR();
            _ws.seekFMDown();
          },
        ),
        // Big Play/Pause
        GestureDetector(
          onTap: () {
            _triggerHapticAndIR();
            _ws.togglePlayPause();
          },
          child: Container(
            width: 64,
            height: 64,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: state.isPlaying ? CroenixColors.secondaryPink : CroenixColors.primaryCyan,
              boxShadow: [
                BoxShadow(
                  color: (state.isPlaying ? CroenixColors.secondaryPink : CroenixColors.primaryCyan).withOpacity(0.5),
                  blurRadius: 16,
                  spreadRadius: 2,
                ),
              ],
            ),
            child: Icon(
              state.isPlaying ? Icons.pause_rounded : Icons.play_arrow_rounded,
              color: Colors.white,
              size: 34,
            ),
          ),
        ),
        _buildTransportBtn(
          icon: Icons.fast_forward_rounded,
          size: 24,
          onTap: () {
            _triggerHapticAndIR();
            _ws.seekFMUp();
          },
        ),
        _buildTransportBtn(
          icon: Icons.skip_next_rounded,
          size: 26,
          onTap: () {
            _triggerHapticAndIR();
            _ws.nextTrack();
          },
        ),
      ],
    );
  }

  Widget _buildTransportBtn({required IconData icon, required double size, required VoidCallback onTap}) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        width: 44,
        height: 44,
        decoration: BoxDecoration(
          color: const Color(0xFF141A28),
          shape: BoxShape.circle,
          border: Border.all(color: Colors.white12),
        ),
        child: Icon(icon, color: Colors.white, size: size),
      ),
    );
  }

  // --- Source Selection Matrix ---
  Widget _buildSourceGrid(DeviceState state) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text(
          'INPUT SOURCE SELECT',
          style: TextStyle(fontSize: 10, fontWeight: FontWeight.bold, letterSpacing: 1.2, color: Colors.white38),
        ),
        const SizedBox(height: 10),
        Row(
          children: [
            _buildSourceBtn(
              label: 'SD CARD',
              icon: Icons.sd_storage_rounded,
              color: CroenixColors.sourceSDCard,
              selected: state.currentSource == AudioSource.sdCard,
              onTap: () => _ws.setSource(AudioSource.sdCard),
            ),
            const SizedBox(width: 8),
            _buildSourceBtn(
              label: 'BLUETOOTH',
              icon: Icons.bluetooth_rounded,
              color: CroenixColors.sourceBluetooth,
              selected: state.currentSource == AudioSource.bluetooth,
              onTap: () => _ws.setSource(AudioSource.bluetooth),
            ),
            const SizedBox(width: 8),
            _buildSourceBtn(
              label: 'FM RADIO',
              icon: Icons.radio_rounded,
              color: CroenixColors.sourceFM,
              selected: state.currentSource == AudioSource.fmRadio,
              onTap: () => _ws.setSource(AudioSource.fmRadio),
            ),
          ],
        ),
        const SizedBox(height: 8),
        Row(
          children: [
            _buildSourceBtn(
              label: 'SOUNDCLOUD',
              icon: Icons.cloud_rounded,
              color: CroenixColors.soundCloudOrange,
              selected: state.currentSource == AudioSource.webStream,
              onTap: () => _ws.setSource(AudioSource.webStream),
            ),
            const SizedBox(width: 8),
            _buildSourceBtn(
              label: 'USB DRIVE',
              icon: Icons.usb_rounded,
              color: Colors.cyanAccent,
              selected: false,
              onTap: () => _ws.setTftScreen(9),
            ),
            const SizedBox(width: 8),
            _buildSourceBtn(
              label: 'AUX IN',
              icon: Icons.input_rounded,
              color: Colors.purpleAccent,
              selected: false,
              onTap: () => _ws.setTftScreen(10),
            ),
          ],
        ),
      ],
    );
  }

  Widget _buildSourceBtn({
    required String label,
    required IconData icon,
    required Color color,
    required bool selected,
    required VoidCallback onTap,
  }) {
    return Expanded(
      child: GestureDetector(
        onTap: () {
          _triggerHapticAndIR();
          onTap();
        },
        child: Container(
          padding: const EdgeInsets.symmetric(vertical: 10, horizontal: 6),
          decoration: BoxDecoration(
            color: selected ? color.withOpacity(0.2) : const Color(0xFF141926),
            borderRadius: BorderRadius.circular(12),
            border: Border.all(
              color: selected ? color : Colors.white12,
              width: selected ? 1.5 : 1.0,
            ),
          ),
          child: Column(
            children: [
              Icon(icon, color: selected ? color : Colors.white60, size: 20),
              const SizedBox(height: 4),
              Text(
                label,
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
                style: TextStyle(
                  color: selected ? Colors.white : Colors.white54,
                  fontSize: 9,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // --- Numeric Preset Keypad (1 - 6) ---
  Widget _buildPresetKeypad(DeviceState state) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text(
          'QUICK PRESETS / CHANNELS',
          style: TextStyle(fontSize: 10, fontWeight: FontWeight.bold, letterSpacing: 1.2, color: Colors.white38),
        ),
        const SizedBox(height: 10),
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: List.generate(6, (index) {
            final presetNum = index + 1;
            return GestureDetector(
              onTap: () {
                _triggerHapticAndIR();
                if (state.currentSource == AudioSource.fmRadio) {
                  // Common FM presets
                  final double freq = 90.0 + (index * 2.2);
                  _ws.setFMFrequency(freq);
                } else if (state.currentSource == AudioSource.sdCard) {
                  _ws.playSDIndex(index);
                }
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(
                    content: Text('Activated Preset #$presetNum'),
                    duration: const Duration(milliseconds: 700),
                    backgroundColor: CroenixColors.darkSurfaceVariant,
                  ),
                );
              },
              child: Container(
                width: 44,
                height: 44,
                decoration: BoxDecoration(
                  color: const Color(0xFF131826),
                  borderRadius: BorderRadius.circular(10),
                  border: Border.all(color: Colors.white10),
                ),
                child: Center(
                  child: Text(
                    'P$presetNum',
                    style: const TextStyle(
                      color: Colors.white,
                      fontFamily: 'monospace',
                      fontWeight: FontWeight.bold,
                      fontSize: 12,
                    ),
                  ),
                ),
              ),
            );
          }),
        ),
      ],
    );
  }

  Widget _buildRoundButton({
    required IconData icon,
    required Color color,
    required String label,
    required VoidCallback onPressed,
  }) {
    return Column(
      children: [
        GestureDetector(
          onTap: onPressed,
          child: Container(
            width: 52,
            height: 52,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: color.withOpacity(0.12),
              border: Border.all(color: color.withOpacity(0.6), width: 1.5),
              boxShadow: [
                BoxShadow(
                  color: color.withOpacity(0.2),
                  blurRadius: 8,
                ),
              ],
            ),
            child: Icon(icon, color: color, size: 24),
          ),
        ),
        const SizedBox(height: 6),
        Text(
          label,
          style: TextStyle(
            fontSize: 9,
            fontWeight: FontWeight.bold,
            letterSpacing: 1.0,
            color: color.withOpacity(0.9),
          ),
        ),
      ],
    );
  }
}

