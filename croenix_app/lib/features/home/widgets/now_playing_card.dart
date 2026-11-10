import 'package:flutter/material.dart';
import '../../../core/theme/croenix_colors.dart';

class NowPlayingCard extends StatelessWidget {
  const NowPlayingCard({super.key});

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () {
        // Navigate to full player screen
      },
      child: Container(
        padding: const EdgeInsets.all(20),
        decoration: GlassEffect.elevated,
        child: Row(
          children: [
            // Album Art
            Container(
              width: 60,
              height: 60,
              decoration: BoxDecoration(
                gradient: CroenixColors.primaryGradient,
                borderRadius: BorderRadius.circular(12),
              ),
              child: const Icon(
                Icons.music_note_rounded,
                color: Colors.white,
                size: 30,
              ),
            ),

            const SizedBox(width: 16),

            // Track Info
            const Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'Now Playing',
                    style: TextStyle(
                      fontSize: 12,
                      color: CroenixColors.textSecondary,
                    ),
                  ),
                  SizedBox(height: 4),
                  Text(
                    'No Track',
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                      color: CroenixColors.textPrimary,
                    ),
                  ),
                  SizedBox(height: 2),
                  Text(
                    '---',
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                      fontSize: 13,
                      color: CroenixColors.textSecondary,
                    ),
                  ),
                ],
              ),
            ),

            const SizedBox(width: 12),

            // Play Button
            Container(
              width: 48,
              height: 48,
              decoration: const BoxDecoration(
                gradient: CroenixColors.primaryGradient,
                shape: BoxShape.circle,
              ),
              child: const Icon(
                Icons.play_arrow_rounded,
                color: Colors.white,
                size: 28,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
