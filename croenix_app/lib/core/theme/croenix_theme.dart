import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'croenix_colors.dart';

class CroenixTheme {
  CroenixTheme._();

  static ThemeData get darkTheme {
    return ThemeData(
      useMaterial3: true,
      brightness: Brightness.dark,
      scaffoldBackgroundColor: CroenixColors.darkBackground,
      colorScheme: const ColorScheme.dark(
        primary: CroenixColors.primaryCyan,
        secondary: CroenixColors.secondaryPurple,
        surface: CroenixColors.darkSurface,
        error: CroenixColors.error,
        onPrimary: Colors.white,
        onSecondary: Colors.white,
        onSurface: CroenixColors.textPrimary,
        onError: Colors.white,
      ),

      // === Typography ===
      textTheme: const TextTheme(
        displayLarge: TextStyle(
          fontSize: 32,
          fontWeight: FontWeight.bold,
          color: CroenixColors.textPrimary,
          letterSpacing: -0.5,
        ),
        displayMedium: TextStyle(
          fontSize: 28,
          fontWeight: FontWeight.bold,
          color: CroenixColors.textPrimary,
        ),
        displaySmall: TextStyle(
          fontSize: 24,
          fontWeight: FontWeight.w600,
          color: CroenixColors.textPrimary,
        ),
        headlineLarge: TextStyle(
          fontSize: 20,
          fontWeight: FontWeight.w600,
          color: CroenixColors.textPrimary,
        ),
        headlineMedium: TextStyle(
          fontSize: 18,
          fontWeight: FontWeight.w600,
          color: CroenixColors.textPrimary,
        ),
        titleLarge: TextStyle(
          fontSize: 16,
          fontWeight: FontWeight.w500,
          color: CroenixColors.textPrimary,
        ),
        titleMedium: TextStyle(
          fontSize: 14,
          fontWeight: FontWeight.w500,
          color: CroenixColors.textSecondary,
        ),
        bodyLarge: TextStyle(
          fontSize: 16,
          fontWeight: FontWeight.normal,
          color: CroenixColors.textPrimary,
        ),
        bodyMedium: TextStyle(
          fontSize: 14,
          fontWeight: FontWeight.normal,
          color: CroenixColors.textSecondary,
        ),
        labelLarge: TextStyle(
          fontSize: 14,
          fontWeight: FontWeight.w500,
          color: CroenixColors.textPrimary,
        ),
        labelMedium: TextStyle(
          fontSize: 12,
          fontWeight: FontWeight.w500,
          color: CroenixColors.textSecondary,
        ),
      ),

      // === AppBar ===
      appBarTheme: const AppBarTheme(
        backgroundColor: Colors.transparent,
        elevation: 0,
        systemOverlayStyle: SystemUiOverlayStyle.light,
        iconTheme: IconThemeData(color: CroenixColors.textPrimary),
        titleTextStyle: TextStyle(
          fontSize: 18,
          fontWeight: FontWeight.w600,
          color: CroenixColors.textPrimary,
        ),
      ),

      // === Cards ===
      cardTheme: CardThemeData(
        color: CroenixColors.glassBackground,
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(20),
          side: const BorderSide(
            color: CroenixColors.glassBorder,
            width: 1,
          ),
        ),
      ),

      // === Elevated Button ===
      elevatedButtonTheme: ElevatedButtonThemeData(
        style: ElevatedButton.styleFrom(
          backgroundColor: CroenixColors.primaryCyan,
          foregroundColor: Colors.white,
          elevation: 0,
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(16),
          ),
          textStyle: const TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.w600,
          ),
        ),
      ),

      // === Icon Button ===
      iconButtonTheme: IconButtonThemeData(
        style: IconButton.styleFrom(
          foregroundColor: CroenixColors.textPrimary,
          iconSize: 24,
        ),
      ),

      // === Slider ===
      sliderTheme: const SliderThemeData(
        activeTrackColor: CroenixColors.primaryCyan,
        inactiveTrackColor: CroenixColors.glassBackground,
        thumbColor: CroenixColors.primaryCyan,
        overlayColor: Color(0x3300D9FF),
        trackHeight: 4,
        thumbShape: RoundSliderThumbShape(enabledThumbRadius: 8),
      ),

      // === Bottom Navigation ===
      bottomNavigationBarTheme: const BottomNavigationBarThemeData(
        backgroundColor: CroenixColors.darkSurface,
        selectedItemColor: CroenixColors.primaryCyan,
        unselectedItemColor: CroenixColors.textTertiary,
        type: BottomNavigationBarType.fixed,
        elevation: 0,
      ),

      // === Divider ===
      dividerTheme: const DividerThemeData(
        color: CroenixColors.divider,
        thickness: 1,
        space: 1,
      ),
    );
  }

  // === System UI Overlay ===
  static void setSystemUIOverlay() {
    SystemChrome.setSystemUIOverlayStyle(
      const SystemUiOverlayStyle(
        statusBarColor: Colors.transparent,
        statusBarIconBrightness: Brightness.light,
        systemNavigationBarColor: CroenixColors.darkBackground,
        systemNavigationBarIconBrightness: Brightness.light,
      ),
    );
  }
}
