import 'package:flutter/material.dart';
import '../theme/app_theme.dart';
import 'waveform_widget.dart';

class AlbumArtCard extends StatelessWidget {
  final bool isPlaying;
  final double size;

  const AlbumArtCard({
    super.key,
    required this.isPlaying,
    this.size = 280.0,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      width: size,
      height: size,
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(28.0),
        border: Border.all(
          color: AppTheme.borderGlow.withOpacity(0.65),
          width: 2.2,
        ),
        boxShadow: [
          BoxShadow(
            color: AppTheme.borderGlow.withOpacity(0.25),
            blurRadius: 24,
            spreadRadius: 2,
          ),
          BoxShadow(
            color: Colors.black.withOpacity(0.4),
            blurRadius: 16,
            offset: const Offset(0, 8),
          ),
        ],
      ),
      child: ClipRRect(
        borderRadius: BorderRadius.circular(25.8),
        child: Stack(
          alignment: Alignment.center,
          children: [
            // Background Artwork Gradient / Vector
            Container(
              decoration: const BoxDecoration(
                gradient: LinearGradient(
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                  colors: [
                    Color(0xFF2E1A47),
                    Color(0xFF1B112E),
                    Color(0xFF0F081D),
                  ],
                ),
              ),
            ),

            // Subtle neon geometric artwork elements
            Positioned(
              top: -30,
              right: -30,
              child: Container(
                width: 140,
                height: 140,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  color: AppTheme.primaryNeon.withOpacity(0.12),
                ),
              ),
            ),

            // Vinyl / Modern Abstract Centerpiece
            Center(
              child: Container(
                width: size * 0.62,
                height: size * 0.62,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  border: Border.all(
                    color: Colors.white.withOpacity(0.08),
                    width: 1.5,
                  ),
                  gradient: RadialGradient(
                    colors: [
                      const Color(0xFF3B1E54).withOpacity(0.6),
                      Colors.transparent,
                    ],
                  ),
                ),
                child: Icon(
                  Icons.album,
                  size: size * 0.35,
                  color: AppTheme.borderGlow.withOpacity(0.4),
                ),
              ),
            ),

            // Ambient dark vignette on the bottom for waveform contrast
            Positioned(
              bottom: 0,
              left: 0,
              right: 0,
              height: size * 0.45,
              child: Container(
                decoration: BoxDecoration(
                  gradient: LinearGradient(
                    begin: Alignment.bottomCenter,
                    end: Alignment.topCenter,
                    colors: [
                      Colors.black.withOpacity(0.65),
                      Colors.transparent,
                    ],
                  ),
                ),
              ),
            ),

            // Soundwave visualizer bar overlay at bottom (as in reference image)
            Positioned(
              bottom: 24,
              child: WaveformWidget(
                isPlaying: isPlaying,
                barCount: 30,
                barColor: const Color(0xFFDDD0F5).withOpacity(0.85),
                maxHeight: 38,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

