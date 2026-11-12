import 'package:flutter/material.dart';
import '../../../core/theme/croenix_colors.dart';

class VolumeControl extends StatelessWidget {
  final int volume; // 0-100
  final Function(double) onVolumeChanged;

  const VolumeControl({
    super.key,
    required this.volume,
    required this.onVolumeChanged,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: GlassEffect.standard,
      child: Row(
        children: [
          Icon(
            volume == 0 ? Icons.volume_mute_rounded : Icons.volume_up_rounded,
            color: CroenixColors.primaryCyan,
            size: 24,
          ),
          const SizedBox(width: 16),
          Expanded(
            child: SliderTheme(
              data: SliderThemeData(
                trackHeight: 4,
                thumbShape: const RoundSliderThumbShape(enabledThumbRadius: 8),
                overlayShape: const RoundSliderOverlayShape(overlayRadius: 18),
                activeTrackColor: CroenixColors.primaryCyan,
                inactiveTrackColor: CroenixColors.glassBackground,
                thumbColor: CroenixColors.primaryCyan,
                overlayColor: CroenixColors.primaryCyan.withValues(alpha: 0.2),
              ),
              child: Slider(
                value: volume.toDouble(),
                min: 0,
                max: 100,
                divisions: 20,
                onChanged: onVolumeChanged,
              ),
            ),
          ),
          const SizedBox(width: 16),
          SizedBox(
            width: 32,
            child: Text(
              '$volume',
              textAlign: TextAlign.center,
              style: const TextStyle(
                fontSize: 14,
                fontWeight: FontWeight.w600,
                color: CroenixColors.textPrimary,
              ),
            ),
          ),
        ],
      ),
    );
  }
}
