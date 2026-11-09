import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'croenix_colors.dart';

class CroenixTheme {
  CroenixTheme._();

  static ThemeData get darkTheme {
    return ThemeData(
      useMaterial3: true,
      brightness: Brightness.dark,

      // Color Scheme
      colorScheme: ColorScheme.dark(
        brightness: Brightness.dark,
        primary: CroenixColors.primary,
        onPrimary: CroenixColors.background,
        secondary: CroenixColors.secondary,
        onSecondary: CroenixColors.textPrimary,
        error: CroenixColors.error,
        onError: CroenixColors.textPrimary,
        surface: CroenixColors.surface,
        onSurface: CroenixColors.textPrimary,
        surfaceContainerHighest: CroenixColors.surfaceVariant,
        outline: CroenixColors.border,
      ),

      // Scaffold
      scaffoldBackgroundColor: CroenixColors.background,

      // App Bar
      appBarTheme: AppBarTheme(
        backgroundColor: CroenixColors.background,
        elevation: 0,
        centerTitle: true,
        titleTextStyle: GoogleFonts.inter(
          color: CroenixColors.textPrimary,
          fontSize: 20,
          fontWeight: FontWeight.w600,
          letterSpacing: -0.5,
        ),
        iconTheme: const IconThemeData(
          color: CroenixColors.textPrimary,
        ),
      ),

      // Card
      cardTheme: CardThemeData(
        color: CroenixColors.surface,
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(16),
          side: const BorderSide(
            color: CroenixColors.borderSubtle,
            width: 1,
          ),
        ),
      ),

      // Elevated Button
      elevatedButtonTheme: ElevatedButtonThemeData(
        style: ElevatedButton.styleFrom(
          backgroundColor: CroenixColors.primary,
          foregroundColor: CroenixColors.background,
          elevation: 0,
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(12),
          ),
          textStyle: GoogleFonts.inter(
            fontSize: 16,
            fontWeight: FontWeight.w600,
            letterSpacing: -0.5,
          ),
        ),
      ),

      // Outlined Button
      outlinedButtonTheme: OutlinedButtonThemeData(
        style: OutlinedButton.styleFrom(
          foregroundColor: CroenixColors.primary,
          side: const BorderSide(color: CroenixColors.primary, width: 1.5),
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(12),
          ),
          textStyle: GoogleFonts.inter(
            fontSize: 16,
            fontWeight: FontWeight.w600,
            letterSpacing: -0.5,
          ),
        ),
      ),

      // Text Button
      textButtonTheme: TextButtonThemeData(
        style: TextButton.styleFrom(
          foregroundColor: CroenixColors.primary,
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
          textStyle: GoogleFonts.inter(
            fontSize: 14,
            fontWeight: FontWeight.w600,
            letterSpacing: -0.3,
          ),
        ),
      ),

      // Input Decoration
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        fillColor: CroenixColors.surfaceVariant,
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: CroenixColors.border),
        ),
        enabledBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: CroenixColors.border),
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: CroenixColors.primary, width: 2),
        ),
        errorBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: CroenixColors.error),
        ),
        contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 16),
        hintStyle: GoogleFonts.inter(
          color: CroenixColors.textTertiary,
          fontSize: 14,
        ),
        labelStyle: GoogleFonts.inter(
          color: CroenixColors.textSecondary,
          fontSize: 14,
        ),
      ),

      // Text Theme
      textTheme: GoogleFonts.interTextTheme(
        ThemeData.dark().textTheme.copyWith(
          displayLarge: GoogleFonts.inter(
            fontSize: 32,
            fontWeight: FontWeight.w700,
            color: CroenixColors.textPrimary,
            letterSpacing: -1,
          ),
          displayMedium: GoogleFonts.inter(
            fontSize: 28,
            fontWeight: FontWeight.w600,
            color: CroenixColors.textPrimary,
            letterSpacing: -0.8,
          ),
          displaySmall: GoogleFonts.inter(
            fontSize: 24,
            fontWeight: FontWeight.w600,
            color: CroenixColors.textPrimary,
            letterSpacing: -0.7,
          ),
          headlineMedium: GoogleFonts.inter(
            fontSize: 20,
            fontWeight: FontWeight.w600,
            color: CroenixColors.textPrimary,
            letterSpacing: -0.5,
          ),
          titleLarge: GoogleFonts.inter(
            fontSize: 18,
            fontWeight: FontWeight.w600,
            color: CroenixColors.textPrimary,
            letterSpacing: -0.4,
          ),
          titleMedium: GoogleFonts.inter(
            fontSize: 16,
            fontWeight: FontWeight.w600,
            color: CroenixColors.textPrimary,
            letterSpacing: -0.3,
          ),
          bodyLarge: GoogleFonts.inter(
            fontSize: 16,
            fontWeight: FontWeight.w400,
            color: CroenixColors.textPrimary,
            letterSpacing: -0.2,
          ),
          bodyMedium: GoogleFonts.inter(
            fontSize: 14,
            fontWeight: FontWeight.w400,
            color: CroenixColors.textSecondary,
            letterSpacing: -0.1,
          ),
          bodySmall: GoogleFonts.inter(
            fontSize: 12,
            fontWeight: FontWeight.w400,
            color: CroenixColors.textTertiary,
          ),
          labelLarge: GoogleFonts.inter(
            fontSize: 14,
            fontWeight: FontWeight.w600,
            color: CroenixColors.textPrimary,
            letterSpacing: -0.2,
          ),
        ),
      ),

      // Icon Theme
      iconTheme: const IconThemeData(
        color: CroenixColors.textPrimary,
        size: 24,
      ),

      // Divider
      dividerTheme: const DividerThemeData(
        color: CroenixColors.divider,
        thickness: 1,
        space: 1,
      ),

      // Progress Indicator
      progressIndicatorTheme: const ProgressIndicatorThemeData(
        color: CroenixColors.primary,
      ),
    );
  }
}
