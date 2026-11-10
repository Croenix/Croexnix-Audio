import 'package:flutter/material.dart';
import '../../../core/theme/croenix_colors.dart';
import '../../../data/models/device_state.dart';

class QuickControlsPanel extends StatelessWidget {
  const QuickControlsPanel({super.key});

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Padding(
          padding: EdgeInsets.symmetric(horizontal: 8, vertical: 8),
          child: Text(
            'Quick Controls',
            style: TextStyle(
              fontSize: 13,
              fontWeight: FontWeight.w600,
              color: CroenixColors.textSecondary,
              letterSpacing: 0.5,
            ),
          ),
        ),
        Container(
          padding: const EdgeInsets.all(20),
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
                  ),
                  _buildSourceButton(
                    icon: Icons.sd_card_rounded,
                    label: 'SD Card',
                    color: CroenixColors.sourceSDCard,
                    source: AudioSource.sdCard,
                  ),
                  _buildSourceButton(
                    icon: Icons.radio_rounded,
                    label: 'FM Radio',
                    color: CroenixColors.sourceFM,
                    source: AudioSource.fmRadio,
                  ),
                  _buildSourceButton(
                    icon: Icons.language_rounded,
                    label: 'Stream',
                    color: CroenixColors.sourceWebStream,
                    source: AudioSource.webStream,
                  ),
                ],
              ),

              const SizedBox(height: 20),
              const Divider(color: CroenixColors.divider),
              const SizedBox(height: 20),

              // Quick Action Buttons
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                children: [
                  _buildActionButton(
                    icon: Icons.shuffle_rounded,
                    label: 'Shuffle',
                  ),
                  _buildActionButton(
                    icon: Icons.repeat_rounded,
                    label: 'Repeat',
                  ),
                  _buildActionButton(
                    icon: Icons.queue_music_rounded,
                    label: 'Queue',
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
  }) {
    return GestureDetector(
      onTap: () {
        // TODO: Switch audio source
      },
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 56,
            height: 56,
            decoration: BoxDecoration(
              color: color.withValues(alpha: 0.15),
              borderRadius: BorderRadius.circular(16),
              border: Border.all(
                color: color.withValues(alpha: 0.3),
                width: 1,
              ),
            ),
            child: Icon(
              icon,
              color: color,
              size: 28,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            label,
            style: TextStyle(
              fontSize: 11,
              fontWeight: FontWeight.w500,
              color: color,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildActionButton({
    required IconData icon,
    required String label,
  }) {
    return GestureDetector(
      onTap: () {
        // TODO: Handle action
      },
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 48,
            height: 48,
            decoration: BoxDecoration(
              color: CroenixColors.glassBackground,
              shape: BoxShape.circle,
              border: Border.all(
                color: CroenixColors.glassBorder,
                width: 1,
              ),
            ),
            child: Icon(
              icon,
              color: CroenixColors.textSecondary,
              size: 24,
            ),
          ),
          const SizedBox(height: 6),
          Text(
            label,
            style: const TextStyle(
              fontSize: 11,
              color: CroenixColors.textSecondary,
            ),
          ),
        ],
      ),
    );
  }
}
