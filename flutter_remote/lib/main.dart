import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'services/esp32_service.dart';
import 'theme/app_theme.dart';
import 'screens/now_playing_screen.dart';
import 'screens/sd_files_screen.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setPreferredOrientations([
    DeviceOrientation.portraitUp,
    DeviceOrientation.portraitDown,
  ]);
  runApp(const CroenixMusicPlayerApp());
}

class CroenixMusicPlayerApp extends StatelessWidget {
  const CroenixMusicPlayerApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (_) => Esp32Service(),
      child: MaterialApp(
        title: 'Croenix Audio - ESP32 Remote',
        debugShowCheckedModeBanner: false,
        theme: AppTheme.themeData,
        home: const MainNavigationShell(),
      ),
    );
  }
}

class MainNavigationShell extends StatefulWidget {
  const MainNavigationShell({super.key});

  @override
  State<MainNavigationShell> createState() => _MainNavigationShellState();
}

class _MainNavigationShellState extends State<MainNavigationShell> {
  int _currentIndex = 0;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: IndexedStack(
        index: _currentIndex,
        children: [
          NowPlayingScreen(
            onOpenSdBrowser: () {
              setState(() {
                _currentIndex = 1;
              });
            },
          ),
          SdFilesScreen(
            onBackToPlayer: () {
              setState(() {
                _currentIndex = 0;
              });
            },
          ),
        ],
      ),
      bottomNavigationBar: Container(
        decoration: BoxDecoration(
          color: AppTheme.surface,
          border: Border(
            top: BorderSide(
              color: AppTheme.borderCard.withOpacity(0.25),
              width: 1,
            ),
          ),
        ),
        child: BottomNavigationBar(
          currentIndex: _currentIndex,
          onTap: (index) {
            setState(() {
              _currentIndex = index;
            });
          },
          backgroundColor: AppTheme.surface,
          selectedItemColor: AppTheme.primaryNeon,
          unselectedItemColor: AppTheme.textMuted,
          selectedLabelStyle: const TextStyle(fontWeight: FontWeight.bold, fontSize: 12),
          unselectedLabelStyle: const TextStyle(fontSize: 12),
          type: BottomNavigationBarType.fixed,
          items: const [
            BottomNavigationBarItem(
              icon: Icon(Icons.play_circle_fill_rounded),
              label: 'Now Playing',
            ),
            BottomNavigationBarItem(
              icon: Icon(Icons.sd_storage_rounded),
              label: 'SD Card Files',
            ),
          ],
        ),
      ),
    );
  }
}
