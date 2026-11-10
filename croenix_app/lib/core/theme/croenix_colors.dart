import 'package:flutter/material.dart';

/// Croenix Audio Player - Glassmorphism Color Palette
class CroenixColors {
  CroenixColors._();

  // === Dark Mode Primary Colors ===
  static const Color darkBackground = Color(0xFF0A0E1A);
  static const Color darkSurface = Color(0xFF141824);
  static const Color darkSurfaceVariant = Color(0xFF1E2330);

  // === Glassmorphism Effects ===
  static const Color glassBackground = Color(0x1AFFFFFF); // 10% white
  static const Color glassBorder = Color(0x33FFFFFF); // 20% white
  static const Color glassHighlight = Color(0x0DFFFFFF); // 5% white

  // === Accent Colors ===
  static const Color primaryCyan = Color(0xFF00D9FF);
  static const Color primaryCyanDark = Color(0xFF00A8CC);
  static const Color secondaryPurple = Color(0xFF8B5CF6);
  static const Color secondaryPink = Color(0xFFEC4899);

  // === Gradient Colors ===
  static const LinearGradient primaryGradient = LinearGradient(
    begin: Alignment.topLeft,
    end: Alignment.bottomRight,
    colors: [primaryCyan, secondaryPurple],
  );

  static const LinearGradient accentGradient = LinearGradient(
    begin: Alignment.topLeft,
    end: Alignment.bottomRight,
    colors: [secondaryPurple, secondaryPink],
  );

  // === Text Colors ===
  static const Color textPrimary = Color(0xFFFFFFFF);
  static const Color textSecondary = Color(0xB3FFFFFF); // 70% opacity
  static const Color textTertiary = Color(0x66FFFFFF); // 40% opacity
  static const Color textDisabled = Color(0x4DFFFFFF); // 30% opacity

  // === Status Colors ===
  static const Color success = Color(0xFF10B981);
  static const Color warning = Color(0xFFF59E0B);
  static const Color error = Color(0xFFEF4444);
  static const Color info = Color(0xFF3B82F6);

  // === Audio Source Colors ===
  static const Color sourceBluetooth = Color(0xFF3B82F6);
  static const Color sourceSDCard = Color(0xFF10B981);
  static const Color sourceFM = Color(0xFFF59E0B);
  static const Color sourceWebStream = Color(0xFF8B5CF6);

  // === UI Element Colors ===
  static const Color divider = Color(0x1AFFFFFF);
  static const Color overlay = Color(0x80000000);
  static const Color shadow = Color(0x40000000);

  // === Visualizer Colors ===
  static const Color waveformActive = primaryCyan;
  static const Color waveformInactive = Color(0x33FFFFFF);
  static const Color spectrumLow = Color(0xFF10B981);
  static const Color spectrumMid = Color(0xFF00D9FF);
  static const Color spectrumHigh = Color(0xFFEC4899);
}

/// Glass effect decoration helper
class GlassEffect {
  static BoxDecoration get standard => BoxDecoration(
        color: CroenixColors.glassBackground,
        borderRadius: BorderRadius.circular(20),
        border: Border.all(
          color: CroenixColors.glassBorder,
          width: 1,
        ),
        boxShadow: [
          BoxShadow(
            color: CroenixColors.shadow,
            blurRadius: 20,
            offset: const Offset(0, 10),
          ),
        ],
      );

  static BoxDecoration get elevated => BoxDecoration(
        color: CroenixColors.glassBackground,
        borderRadius: BorderRadius.circular(20),
        border: Border.all(
          color: CroenixColors.glassBorder,
          width: 1.5,
        ),
        boxShadow: [
          BoxShadow(
            color: CroenixColors.shadow,
            blurRadius: 30,
            offset: const Offset(0, 15),
          ),
        ],
      );

  static BoxDecoration get subtle => BoxDecoration(
        color: CroenixColors.glassHighlight,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: CroenixColors.glassBorder.withValues(alpha: 0.5),
          width: 0.5,
        ),
      );
}
