import 'package:flutter/material.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/models/device_state.dart';
import '../../data/services/websocket_service.dart';

class FMRadioScreen extends StatefulWidget {
  const FMRadioScreen({super.key});

  @override
  State<FMRadioScreen> createState() => _FMRadioScreenState();
}

class _FMRadioScreenState extends State<FMRadioScreen> {
  final WebSocketService _ws = WebSocketService();

  static const List<Map<String, dynamic>> _defaultPresets = [
    {'name': 'Radio Mirchi', 'freq': 98.3},
    {'name': 'Red FM', 'freq': 93.5},
    {'name': 'Radio City', 'freq': 91.1},
    {'name': 'AIR Vividh Bharati', 'freq': 100.1},
    {'name': 'Fever FM', 'freq': 104.0},
    {'name': 'All India Radio', 'freq': 102.6},
  ];

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: const BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            CroenixColors.darkBackground,
            Color(0xFF23180F),
            Color(0xFF0F1523),
          ],
        ),
      ),
      child: SafeArea(
        child: StreamBuilder<DeviceState>(
          stream: _ws.stateStream,
          initialData: _ws.currentState,
          builder: (context, snapshot) {
            final state = snapshot.data ?? _ws.currentState;
            final isFMSource = state.currentSource == AudioSource.fmRadio;

            return SingleChildScrollView(
              physics: const BouncingScrollPhysics(),
              padding: const EdgeInsets.only(left: 16, right: 16, top: 12, bottom: 100),
              child: Column(
                children: [
                  _buildHeader(state),
                  const SizedBox(height: 16),
                  _buildFrequencyTuner(state),
                  const SizedBox(height: 20),
                  _buildControls(state),
                  const SizedBox(height: 24),
                  _buildPresetsSection(state),
                  const SizedBox(height: 20),
                  _buildHardwareDiagnosticCard(state),
                ],
              ),
            );
          },
        ),
      ),
    );
  }

  Widget _buildHeader(DeviceState state) {
    return Row(
      children: [
        Container(
          padding: const EdgeInsets.all(10),
          decoration: BoxDecoration(
            color: CroenixColors.sourceFM.withOpacity(0.15),
            borderRadius: BorderRadius.circular(12),
            border: Border.all(color: CroenixColors.sourceFM.withOpacity(0.3)),
          ),
          child: const Icon(Icons.radio_rounded, color: CroenixColors.sourceFM, size: 26),
        ),
        const SizedBox(width: 14),
        Expanded(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text(
                'FM BROADCAST RADIO',
                style: TextStyle(
                  fontSize: 20,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 1.1,
                  color: Colors.white,
                ),
              ),
              Text(
                'Si4703 Tuner • 87.5 - 108.0 MHz FM Band',
                style: TextStyle(fontSize: 12, color: Colors.white.withOpacity(0.6)),
              ),
            ],
          ),
        ),
        if (state.currentSource != AudioSource.fmRadio)
          TextButton.icon(
            style: TextButton.styleFrom(
              backgroundColor: CroenixColors.sourceFM.withOpacity(0.15),
              foregroundColor: CroenixColors.sourceFM,
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
            ),
            icon: const Icon(Icons.power_settings_new_rounded, size: 16),
            label: const Text('ACTIVATE', style: TextStyle(fontSize: 11, fontWeight: FontWeight.bold)),
            onPressed: () => _ws.setSource(AudioSource.fmRadio),
          ),
      ],
    );
  }

  Widget _buildFrequencyTuner(DeviceState state) {
    final freq = state.fmFrequency.clamp(87.5, 108.0);
    final rssi = state.fmSignalStrength;

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(24),
      decoration: GlassEffect.elevated,
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                decoration: BoxDecoration(
                  color: Colors.white.withOpacity(0.06),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Row(
                  children: [
                    Icon(
                      rssi > 50 ? Icons.signal_cellular_alt_rounded : Icons.signal_cellular_alt_2_bar_rounded,
                      size: 16,
                      color: rssi > 40 ? CroenixColors.success : Colors.amber,
                    ),
                    const SizedBox(width: 6),
                    Text(
                      'RSSI: $rssi%',
                      style: const TextStyle(color: Colors.white70, fontSize: 12, fontWeight: FontWeight.bold),
                    ),
                  ],
                ),
              ),
              Text(
                state.fmStationName.isNotEmpty ? state.fmStationName : 'FM STEREO',
                style: const TextStyle(
                  color: CroenixColors.sourceFM,
                  fontSize: 12,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 1.2,
                ),
              ),
            ],
          ),
          const SizedBox(height: 24),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.baseline,
            textBaseline: TextBaseline.alphabetic,
            children: [
              Text(
                freq.toStringAsFixed(1),
                style: const TextStyle(
                  fontSize: 64,
                  fontWeight: FontWeight.w900,
                  color: Colors.white,
                  letterSpacing: -1,
                ),
              ),
              const SizedBox(width: 8),
              const Text(
                'MHz',
                style: TextStyle(
                  fontSize: 20,
                  fontWeight: FontWeight.bold,
                  color: CroenixColors.sourceFM,
                ),
              ),
            ],
          ),
          const SizedBox(height: 20),
          SliderTheme(
            data: SliderThemeData(
              activeTrackColor: CroenixColors.sourceFM,
              inactiveTrackColor: Colors.white12,
              thumbColor: CroenixColors.sourceFM,
              trackHeight: 6,
              thumbShape: const RoundSliderThumbShape(enabledThumbRadius: 10),
              overlayShape: const RoundSliderOverlayShape(overlayRadius: 20),
            ),
            child: Slider(
              value: freq,
              min: 87.5,
              max: 108.0,
              divisions: 205, // 0.1 MHz increments
              onChanged: (val) {
                final rounded = (val * 10).round() / 10.0;
                _ws.setFMFrequency(rounded);
              },
            ),
          ),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 8),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: const [
                Text('87.5', style: TextStyle(color: Colors.white38, fontSize: 11)),
                Text('95.0', style: TextStyle(color: Colors.white38, fontSize: 11)),
                Text('100.0', style: TextStyle(color: Colors.white38, fontSize: 11)),
                Text('108.0', style: TextStyle(color: Colors.white38, fontSize: 11)),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildControls(DeviceState state) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
      children: [
        _buildActionButton(
          icon: Icons.keyboard_double_arrow_left_rounded,
          label: '-0.1 MHz',
          onTap: () {
            final newFreq = ((state.fmFrequency - 0.1) * 10).round() / 10.0;
            if (newFreq >= 87.5) _ws.setFMFrequency(newFreq);
          },
        ),
        _buildActionButton(
          icon: Icons.search_rounded,
          label: 'AUTO-SEEK',
          color: CroenixColors.sourceFM,
          highlight: true,
          onTap: () {
            _ws.setSource(AudioSource.fmRadio);
            _ws.seekBestFMStation();
            ScaffoldMessenger.of(context).showSnackBar(
              const SnackBar(
                content: Text('Auto-seeking strongest broadcast signal...'),
                duration: Duration(milliseconds: 1500),
              ),
            );
          },
        ),
        _buildActionButton(
          icon: Icons.keyboard_double_arrow_right_rounded,
          label: '+0.1 MHz',
          onTap: () {
            final newFreq = ((state.fmFrequency + 0.1) * 10).round() / 10.0;
            if (newFreq <= 108.0) _ws.setFMFrequency(newFreq);
          },
        ),
      ],
    );
  }

  Widget _buildActionButton({
    required IconData icon,
    required String label,
    required VoidCallback onTap,
    Color color = Colors.white,
    bool highlight = false,
  }) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 18, vertical: 12),
        decoration: BoxDecoration(
          color: highlight ? color.withOpacity(0.2) : Colors.white.withOpacity(0.06),
          borderRadius: BorderRadius.circular(16),
          border: Border.all(
            color: highlight ? color : Colors.white.withOpacity(0.1),
            width: highlight ? 1.5 : 1,
          ),
        ),
        child: Column(
          children: [
            Icon(icon, color: highlight ? color : Colors.white70, size: 26),
            const SizedBox(height: 4),
            Text(
              label,
              style: TextStyle(
                color: highlight ? color : Colors.white70,
                fontSize: 11,
                fontWeight: FontWeight.bold,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildPresetsSection(DeviceState state) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text(
          'STATION PRESETS',
          style: TextStyle(
            fontSize: 14,
            fontWeight: FontWeight.bold,
            letterSpacing: 1.2,
            color: Colors.white70,
          ),
        ),
        const SizedBox(height: 12),
        GridView.builder(
          shrinkWrap: true,
          physics: const NeverScrollableScrollPhysics(),
          gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
            crossAxisCount: 2,
            childAspectRatio: 2.4,
            crossAxisSpacing: 10,
            mainAxisSpacing: 10,
          ),
          itemCount: _defaultPresets.length,
          itemBuilder: (context, index) {
            final preset = _defaultPresets[index];
            final pFreq = preset['freq'] as double;
            final isTuned = (state.fmFrequency - pFreq).abs() < 0.05;

            return GestureDetector(
              onTap: () {
                _ws.setSource(AudioSource.fmRadio);
                _ws.setFMFrequency(pFreq);
              },
              child: Container(
                padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
                decoration: BoxDecoration(
                  color: isTuned ? CroenixColors.sourceFM.withOpacity(0.2) : Colors.white.withOpacity(0.05),
                  borderRadius: BorderRadius.circular(14),
                  border: Border.all(
                    color: isTuned ? CroenixColors.sourceFM : Colors.white.withOpacity(0.08),
                    width: isTuned ? 1.5 : 1,
                  ),
                ),
                child: Row(
                  children: [
                    Container(
                      width: 8,
                      height: 8,
                      decoration: BoxDecoration(
                        color: isTuned ? CroenixColors.sourceFM : Colors.white24,
                        shape: BoxShape.circle,
                      ),
                    ),
                    const SizedBox(width: 10),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Text(
                            preset['name'] as String,
                            maxLines: 1,
                            overflow: TextOverflow.ellipsis,
                            style: TextStyle(
                              color: isTuned ? CroenixColors.sourceFM : Colors.white,
                              fontWeight: FontWeight.bold,
                              fontSize: 12,
                            ),
                          ),
                          Text(
                            '$pFreq MHz',
                            style: const TextStyle(color: Colors.white54, fontSize: 11),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),
            );
          },
        ),
      ],
    );
  }

  Widget _buildHardwareDiagnosticCard(DeviceState state) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.03),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.white.withOpacity(0.06)),
      ),
      child: Row(
        children: [
          const Icon(Icons.developer_board_rounded, color: Colors.white38, size: 24),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: const [
                Text(
                  'Si4703 Tuner Wiring',
                  style: TextStyle(color: Colors.white70, fontSize: 12, fontWeight: FontWeight.bold),
                ),
                SizedBox(height: 2),
                Text(
                  'SDA: GPIO 21  •  SCL: GPIO 27  •  RST: GPIO 16',
                  style: TextStyle(color: Colors.white38, fontSize: 11),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
