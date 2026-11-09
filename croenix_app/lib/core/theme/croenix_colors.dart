import 'package:flutter/material.dart';

/// Croenix color palette - futuristic dark theme
class CroenixColors {
  CroenixColors._();

  // Primary Colors
  static const Color background = Color(0xFF090D14);
  static const Color surface = Color(0xFF172231);
  static const Color surfaceVariant = Color(0xFF111925);

  // Accent Colors
  static const Color primary = Color(0xFF00D9FF);
  static const Color secondary = Color(0xFF7C5CFF);
  static const Color accent = Color(0xFF00FFA3);

  // Text Colors
  static const Color textPrimary = Color(0xFFF2F6FC);
  static const Color textSecondary = Color(0xFF8B9BAF);
  static const Color textTertiary = Color(0xFF5A6A7F);

  // Status Colors
  static const Color success = Color(0xFF00FFA3);
  static const Color warning = Color(0xFFFFB800);
  static const Color error = Color(0xFFFF4757);
  static const Color info = Color(0xFF00D9FF);

  // UI Elements
  static const Color border = Color(0xFF2A3544);
  static const Color borderSubtle = Color(0xFF1A2230);
  static const Color divider = Color(0xFF1A2230);

  // Overlays
  static const Color overlay = Color(0x1A000000);
  static const Color overlayLight = Color(0x0D000000);

  // Gradients
  static const List<Color> primaryGradient = [
    Color(0xFF00D9FF),
    Color(0xFF7C5CFF),
  ];

  static const List<Color> surfaceGradient = [
    Color(0xFF172231),
    Color(0xFF0F1821),
  ];
}
