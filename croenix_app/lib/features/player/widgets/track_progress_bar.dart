import 'package:flutter/material.dart';
import '../../../core/theme/croenix_colors.dart';

class TrackProgressBar extends StatelessWidget {
  final int position; // seconds
  final int duration; // seconds
  final Function(int) onSeek;

  const TrackProgressBar({
    super.key,
    required this.position,
    required this.duration,
    required this.onSeek,
  });

  @override
  Widget build(BuildContext context) {
    final progress = duration > 0 ? position / duration : 0.0;

    return Column(
      children: [
        // Progress slider
        SliderTheme(
          data: SliderThemeData(
            trackHeight: 4,
            thumbShape: const RoundSliderThumbShape(enabledThumbRadius: 6),
            overlayShape: const RoundSliderOverlayShape(overlayRadius: 16),
            activeTrackColor: CroenixColors.primaryCyan,
            inactiveTrackColor: CroenixColors.glassBackground,
            thumbColor: CroenixColors.primaryCyan,
            overlayColor: CroenixColors.primaryCyan.withValues(alpha: 0.2),
          ),
          child: Slider(
            value: progress.clamp(0.0, 1.0),
            onChanged: (value) {
              if (duration > 0) {
                onSeek((value * duration).round());
              }
            },
          ),
        ),

        // Time labels
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 8),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                _formatDuration(position),
                style: const TextStyle(
                  fontSize: 12,
                  color: CroenixColors.textSecondary,
                  fontWeight: FontWeight.w500,
                ),
              ),
              Text(
                _formatDuration(duration),
                style: const TextStyle(
                  fontSize: 12,
                  color: CroenixColors.textSecondary,
                  fontWeight: FontWeight.w500,
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }

  String _formatDuration(int seconds) {
    final minutes = seconds ~/ 60;
    final remainingSeconds = seconds % 60;
    return '${minutes.toString().padLeft(1, '0')}:${remainingSeconds.toString().padLeft(2, '0')}';
  }
}
