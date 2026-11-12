import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'core/theme/croenix_theme.dart';
import 'app.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  // Set system UI overlay style
  CroenixTheme.setSystemUIOverlay();

  // Lock orientation to portrait
  await SystemChrome.setPreferredOrientations([
    DeviceOrientation.portraitUp,
    DeviceOrientation.portraitDown,
  ]);

  runApp(const CroenixApp());
}
