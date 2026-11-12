import 'package:flutter/material.dart';
import 'core/theme/croenix_theme.dart';
import 'features/home/home_screen.dart';

class CroenixApp extends StatelessWidget {
  const CroenixApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Croenix Audio',
      theme: CroenixTheme.darkTheme,
      debugShowCheckedModeBanner: false,
      home: const HomeScreen(),
    );
  }
}
