import 'package:flutter/material.dart';
import '../../../core/theme/croenix_colors.dart';

class ControlButtons extends StatelessWidget {
  final bool isPlaying;
  final VoidCallback onPlay;
  final VoidCallback onPause;
  final VoidCallback onPrevious;
  final VoidCallback onNext;

  const ControlButtons({
    super.key,
    required this.isPlaying,
    required this.onPlay,
    required this.onPause,
    required this.onPrevious,
    required this.onNext,
  });

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        // Previous Button
        _buildControlButton(
          icon: Icons.skip_previous_rounded,
          size: 48,
          onTap: onPrevious,
        ),

        const SizedBox(width: 20),

        // Play/Pause Button
        _buildPlayPauseButton(),

        const SizedBox(width: 20),

        // Next Button
        _buildControlButton(
          icon: Icons.skip_next_rounded,
          size: 48,
          onTap: onNext,
        ),
      ],
    );
  }

  Widget _buildPlayPauseButton() {
    return GestureDetector(
      onTap: isPlaying ? onPause : onPlay,
      child: Container(
        width: 80,
        height: 80,
        decoration: BoxDecoration(
          gradient: CroenixColors.primaryGradient,
          shape: BoxShape.circle,
          boxShadow: [
            BoxShadow(
              color: CroenixColors.primaryCyan.withValues(alpha: 0.3),
              blurRadius: 20,
              offset: const Offset(0, 10),
            ),
          ],
        ),
        child: Icon(
          isPlaying ? Icons.pause_rounded : Icons.play_arrow_rounded,
          size: 40,
          color: Colors.white,
        ),
      ),
    );
  }

  Widget _buildControlButton({
    required IconData icon,
    required double size,
    required VoidCallback onTap,
  }) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        width: size,
        height: size,
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
          size: size * 0.5,
          color: CroenixColors.textPrimary,
        ),
      ),
    );
  }
}
