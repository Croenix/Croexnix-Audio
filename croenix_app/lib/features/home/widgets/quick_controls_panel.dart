import 'package:flutter/material.dart';
import '../../../core/theme/croenix_colors.dart';
import '../../../data/models/device_state.dart';
import '../../../data/services/websocket_service.dart';

class QuickControlsPanel extends StatefulWidget {
  const QuickControlsPanel({super.key});

  @override
  State<QuickControlsPanel> createState() => _QuickControlsPanelState();
}

class _QuickControlsPanelState extends State<QuickControlsPanel> {
  final WebSocketService _wsService = WebSocketService();
  DeviceState _deviceState = const DeviceState();

  @override
  void initState() {
    super.initState();
    _deviceState = _wsService.currentState;
    _wsService.stateStream.listen((state) {
      if (mounted) setState(() => _deviceState = state);
    });
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Padding(
          padding: EdgeInsets.symmetric(horizontal: 8, vertical: 8),
          child: Text(
            'AUDIO SOURCES',
            style: TextStyle(
              fontSize: 12,
              fontWeight: FontWeight.w700,
              color: CroenixColors.textSecondary,
              letterSpacing: 1.0,
            ),
          ),
        ),
        Container(
          padding: const EdgeInsets.all(16),
          decoration: GlassEffect.standard,
          child: Column(
            children: [
              // Source Selector Row
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                children: [
                  _buildSourceButton(
                    icon: Icons.bluetooth_audio_rounded,
                    label: 'Bluetooth',
                    color: CroenixColors.sourceBluetooth,
                    source: AudioSource.bluetooth,
                    isActive: _deviceState.currentSource == AudioSource.bluetooth,
                  ),
                  _buildSourceButton(
                    icon: Icons.sd_card_rounded,
                    label: 'SD Card',
                    color: CroenixColors.sourceSDCard,
                    source: AudioSource.sdCard,
                    isActive: _deviceState.currentSource == AudioSource.sdCard,
                  ),
                  _buildSourceButton(
                    icon: Icons.radio_rounded,
                    label: 'FM Radio',
                    color: CroenixColors.sourceFM,
                    source: AudioSource.fmRadio,
                    isActive: _deviceState.currentSource == AudioSource.fmRadio,
                  ),
                  _buildSourceButton(
                    icon: Icons.language_rounded,
                    label: 'Stream',
                    color: CroenixColors.sourceWebStream,
                    source: AudioSource.webStream,
                    isActive: _deviceState.currentSource == AudioSource.webStream,
                  ),
                ],
              ),
              const SizedBox(height: 16),
              const Divider(color: CroenixColors.divider),
              const SizedBox(height: 12),

              // Volume Quick Slider
              Row(
                children: [
                  const Icon(Icons.volume_down_rounded, color: CroenixColors.textSecondary, size: 20),
                  Expanded(
                    child: SliderTheme(
                      data: SliderTheme.of(context).copyWith(
                        activeTrackColor: CroenixColors.primaryCyan,
                        inactiveTrackColor: Colors.white12,
                        thumbColor: Colors.white,
                        thumbShape: const RoundSliderThumbShape(enabledThumbRadius: 6),
                        trackHeight: 3,
                      ),
                      child: Slider(
                        value: _deviceState.volume.toDouble().clamp(0.0, 100.0),
                        min: 0,
                        max: 100,
                        onChanged: (val) {
                          _wsService.setVolume(val.toInt());
                        },
                      ),
                    ),
                  ),
                  const Icon(Icons.volume_up_rounded, color: CroenixColors.textSecondary, size: 20),
                  const SizedBox(width: 8),
                  SizedBox(
                    width: 36,
                    child: Text(
                      '${_deviceState.volume}%',
                      style: const TextStyle(
                        fontSize: 12,
                        fontWeight: FontWeight.bold,
                        color: CroenixColors.textPrimary,
                      ),
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildSourceButton({
    required IconData icon,
    required String label,
    required Color color,
    required AudioSource source,
    required bool isActive,
  }) {
    return GestureDetector(
      onTap: () {
        _wsService.setSource(source);
      },
      child: Column(
        children: [
          AnimatedContainer(
            duration: const Duration(milliseconds: 200),
            width: 54,
            height: 54,
            decoration: BoxDecoration(
              color: isActive ? color.withValues(alpha: 0.25) : Colors.white.withValues(alpha: 0.05),
              borderRadius: BorderRadius.circular(16),
              border: Border.all(
                color: isActive ? color : Colors.white.withValues(alpha: 0.1),
                width: isActive ? 2 : 1,
              ),
              boxShadow: isActive
                  ? [
                      BoxShadow(
                        color: color.withValues(alpha: 0.35),
                        blurRadius: 12,
                        offset: const Offset(0, 4),
                      ),
                    ]
                  : null,
            ),
            child: Icon(
              icon,
              color: isActive ? color : CroenixColors.textSecondary,
              size: 26,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            label,
            style: TextStyle(
              fontSize: 11,
              fontWeight: isActive ? FontWeight.bold : FontWeight.normal,
              color: isActive ? CroenixColors.textPrimary : CroenixColors.textSecondary,
            ),
          ),
        ],
      ),
    );
  }
}
