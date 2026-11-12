import 'package:flutter/material.dart';

class AppTheme {
  // Color palette matching the reference image
  static const Color background = Color(0xFF0D0A1A);
  static const Color surface = Color(0xFF15102A);
  static const Color surfaceHighlight = Color(0xFF221A42);
  static const Color borderCard = Color(0xFF8843B8);
  static const Color borderGlow = Color(0xFFC04EFA);
  
  static const Color primaryNeon = Color(0xFFFF2E93); // Electric Magenta
  static const Color secondaryNeon = Color(0xFF9933FF); // Bright Purple
  static const Color accentCyan = Color(0xFF00E5FF); // Electric Cyan
  
  static const Color textPrimary = Color(0xFFFFFFFF);
  static const Color textSecondary = Color(0xFFB0A4D0);
  static const Color textMuted = Color(0xFF6F6690);
  
  static const Color progressTrack = Color(0xFF2B224C);
  static const Color progressActive = Color(0xFFFFFFFF); // White active slider as in reference image

  static ThemeData get themeData {
    return ThemeData.dark().copyWith(
      scaffoldBackgroundColor: background,
      primaryColor: primaryNeon,
      colorScheme: const ColorScheme.dark(
        primary: primaryNeon,
        secondary: secondaryNeon,
        surface: surface,
      ),
      sliderTheme: const SliderThemeData(
        activeTrackColor: progressActive,
        inactiveTrackColor: progressTrack,
        thumbColor: Colors.white,
        overlayColor: Color(0x33FF2E93),
        trackHeight: 3.5,
        thumbShape: RoundSliderThumbShape(enabledThumbRadius: 6),
      ),
    );
  }
}

