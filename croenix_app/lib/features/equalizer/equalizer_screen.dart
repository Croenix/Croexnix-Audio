import 'package:flutter/material.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/models/device_state.dart';
import '../../data/services/websocket_service.dart';

class EqualizerScreen extends StatefulWidget {
  const EqualizerScreen({super.key});

  @override
  State<EqualizerScreen> createState() => _EqualizerScreenState();
}

class _EqualizerScreenState extends State<EqualizerScreen> {
  final WebSocketService _ws = WebSocketService();

  static const List<Map<String, dynamic>> _presets = [
    {'name': 'Flat', 'icon': Icons.horizontal_rule_rounded, 'bass': 0, 'mid': 0, 'treble': 0},
    {'name': 'Rock', 'icon': Icons.electric_bolt_rounded, 'bass': 4, 'mid': -1, 'treble': 3},
    {'name': 'Pop', 'icon': Icons.star_rounded, 'bass': 2, 'mid': 2, 'treble': 3},
    {'name': 'Jazz', 'icon': Icons.nightlife_rounded, 'bass': 3, 'mid': 1, 'treble': 2},
    {'name': 'Bass Boost', 'icon': Icons.speaker_rounded, 'bass': 6, 'mid': 0, 'treble': -2},
    {'name': 'Treble Boost', 'icon': Icons.graphic_eq_rounded, 'bass': -2, 'mid': 1, 'treble': 6},
    {'name': 'Electronic', 'icon': Icons.album_rounded, 'bass': 5, 'mid': 1, 'treble': 4},
    {'name': 'Vocal', 'icon': Icons.mic_rounded, 'bass': -2, 'mid': 4, 'treble': 1},
  ];

  int _bass = 0;
  int _mid = 0;
  int _treble = 0;
  bool _initialized = false;

  void _syncFromState(DeviceState state) {
    if (!_initialized) {
      _bass = state.eqBass;
      _mid = state.eqMid;
      _treble = state.eqTreble;
      _initialized = true;
    }
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: const BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            CroenixColors.darkBackground,
            Color(0xFF140F24),
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
            _syncFromState(state);

            return SingleChildScrollView(
              physics: const BouncingScrollPhysics(),
              padding: const EdgeInsets.only(left: 16, right: 16, top: 12, bottom: 100),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  _buildHeader(state),
                  const SizedBox(height: 20),
                  _buildHardwareSlidersCard(state),
                  const SizedBox(height: 24),
                  _buildPresetsGrid(state),
                  const SizedBox(height: 24),
                  _buildDACArchitectureCard(),
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
            color: CroenixColors.secondaryPurple.withOpacity(0.15),
            borderRadius: BorderRadius.circular(12),
            border: Border.all(color: CroenixColors.secondaryPurple.withOpacity(0.3)),
          ),
          child: const Icon(Icons.tune_rounded, color: CroenixColors.secondaryPurple, size: 26),
        ),
        const SizedBox(width: 14),
        const Expanded(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                'STUDIO EQUALIZER',
                style: TextStyle(
                  fontSize: 20,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 1.1,
                  color: Colors.white,
                ),
              ),
              Text(
                '3-Band Hardware Tone Filter + 8 Presets',
                style: TextStyle(fontSize: 12, color: Colors.white54),
              ),
            ],
          ),
        ),
        TextButton(
          onPressed: () {
            setState(() {
              _bass = 0;
              _mid = 0;
              _treble = 0;
            });
            _ws.setEQPreset(0);
            _ws.setHardwareTone(0, 0, 0);
          },
          child: const Text('RESET', style: TextStyle(color: CroenixColors.primaryCyan, fontWeight: FontWeight.bold)),
        ),
      ],
    );
  }

  Widget _buildHardwareSlidersCard(DeviceState state) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(20),
      decoration: GlassEffect.elevated,
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              const Expanded(
                child: Text(
                  'HARDWARE TONE',
                  style: TextStyle(
                    color: Colors.white70,
                    fontSize: 12,
                    fontWeight: FontWeight.bold,
                    letterSpacing: 1.1,
                  ),
                  overflow: TextOverflow.ellipsis,
                ),
              ),
              const SizedBox(width: 8),
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
                decoration: BoxDecoration(
                  color: CroenixColors.primaryCyan.withOpacity(0.15),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Text(
                  'B:${_bass >= 0 ? "+$_bass" : "$_bass"}dB M:${_mid >= 0 ? "+$_mid" : "$_mid"}dB T:${_treble >= 0 ? "+$_treble" : "$_treble"}dB',
                  style: const TextStyle(color: CroenixColors.primaryCyan, fontSize: 11, fontWeight: FontWeight.bold),
                ),
              ),
            ],
          ),
          const SizedBox(height: 20),
          _buildToneSlider(
            label: 'BASS (100 Hz)',
            value: _bass,
            color: const Color(0xFF00D9FF),
            onChanged: (val) {
              setState(() => _bass = val.round());
              _ws.setHardwareTone(_bass, _mid, _treble);
            },
          ),
          const SizedBox(height: 14),
          _buildToneSlider(
            label: 'MIDRANGE (1 kHz)',
            value: _mid,
            color: const Color(0xFF8B5CF6),
            onChanged: (val) {
              setState(() => _mid = val.round());
              _ws.setHardwareTone(_bass, _mid, _treble);
            },
          ),
          const SizedBox(height: 14),
          _buildToneSlider(
            label: 'TREBLE (10 kHz)',
            value: _treble,
            color: const Color(0xFFEC4899),
            onChanged: (val) {
              setState(() => _treble = val.round());
              _ws.setHardwareTone(_bass, _mid, _treble);
            },
          ),
        ],
      ),
    );
  }

  Widget _buildToneSlider({
    required String label,
    required int value,
    required Color color,
    required ValueChanged<double> onChanged,
  }) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text(label, style: const TextStyle(color: Colors.white70, fontSize: 12, fontWeight: FontWeight.w600)),
            Text(
              '${value >= 0 ? "+$value" : "$value"} dB',
              style: TextStyle(color: color, fontSize: 12, fontWeight: FontWeight.bold),
            ),
          ],
        ),
        SliderTheme(
          data: SliderThemeData(
            activeTrackColor: color,
            inactiveTrackColor: Colors.white12,
            thumbColor: color,
            trackHeight: 5,
            thumbShape: const RoundSliderThumbShape(enabledThumbRadius: 8),
          ),
          child: Slider(
            value: value.toDouble().clamp(-10.0, 10.0),
            min: -10,
            max: 10,
            divisions: 20,
            onChanged: onChanged,
          ),
        ),
      ],
    );
  }

  Widget _buildPresetsGrid(DeviceState state) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text(
          'STUDIO SOUND PROFILES',
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
            childAspectRatio: 2.3,
            crossAxisSpacing: 10,
            mainAxisSpacing: 10,
          ),
          itemCount: _presets.length,
          itemBuilder: (context, index) {
            final p = _presets[index];
            final isSelected = state.eqPreset == index;

            return GestureDetector(
              onTap: () {
                setState(() {
                  _bass = p['bass'] as int;
                  _mid = p['mid'] as int;
                  _treble = p['treble'] as int;
                });
                _ws.setEQPreset(index);
                _ws.setHardwareTone(_bass, _mid, _treble);
              },
              child: Container(
                padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
                decoration: BoxDecoration(
                  color: isSelected ? CroenixColors.secondaryPurple.withOpacity(0.2) : Colors.white.withOpacity(0.04),
                  borderRadius: BorderRadius.circular(14),
                  border: Border.all(
                    color: isSelected ? CroenixColors.secondaryPurple : Colors.white.withOpacity(0.07),
                    width: isSelected ? 1.5 : 1,
                  ),
                ),
                child: Row(
                  children: [
                    Icon(
                      p['icon'] as IconData,
                      color: isSelected ? CroenixColors.secondaryPurple : Colors.white54,
                      size: 24,
                    ),
                    const SizedBox(width: 10),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Text(
                            p['name'] as String,
                            style: TextStyle(
                              color: isSelected ? Colors.white : Colors.white70,
                              fontWeight: isSelected ? FontWeight.bold : FontWeight.w600,
                              fontSize: 13,
                            ),
                          ),
                          Text(
                            'B:${p['bass']} M:${p['mid']} T:${p['treble']}',
                            style: TextStyle(
                              color: isSelected ? CroenixColors.secondaryPurple : Colors.white38,
                              fontSize: 10,
                            ),
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

  Widget _buildDACArchitectureCard() {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.03),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.white.withOpacity(0.06)),
      ),
      child: const Row(
        children: [
          Icon(Icons.memory_rounded, color: CroenixColors.primaryCyan, size: 24),
          SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'PCM5102A 32-bit / 384kHz DAC',
                  style: TextStyle(color: Colors.white70, fontSize: 12, fontWeight: FontWeight.bold),
                ),
                SizedBox(height: 2),
                Text(
                  'BCK: GPIO 26  •  DIN: GPIO 25  •  LCK: GPIO 22  •  XMT: 3.3V',
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
