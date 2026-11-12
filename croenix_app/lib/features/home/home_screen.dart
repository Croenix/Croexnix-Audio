import 'package:flutter/material.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/services/websocket_service.dart';
import '../player/player_screen.dart';
import '../soundcloud/soundcloud_screen.dart';
import '../sd_card/sd_card_screen.dart';
import '../fm_radio/fm_radio_screen.dart';
import '../equalizer/equalizer_screen.dart';
import '../settings/settings_screen.dart';
import '../remote/virtual_remote_screen.dart';
import 'widgets/connection_status_card.dart';
import 'widgets/quick_controls_panel.dart';
import 'widgets/now_playing_card.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  int _selectedIndex = 0;

  @override
  void initState() {
    super.initState();
    // Attempt real-time auto discovery and connection on startup
    WebSocketService().autoConnect();
  }

  late final List<Widget> _screens = [
    _HomeTab(
      onNavigate: (idx) => setState(() => _selectedIndex = idx),
      onOpenEqualizer: () => _openScreen(const EqualizerScreen()),
      onOpenSettings: () => _openScreen(const SettingsScreen()),
      onOpenRemote: () => setState(() => _selectedIndex = 1),
    ),
    const VirtualRemoteScreen(),
    const PlayerScreen(),
    const SDCardScreen(),
    const SoundCloudScreen(),
    const FMRadioScreen(),
  ];

  void _openScreen(Widget screen) {
    Navigator.of(context).push(
      MaterialPageRoute(builder: (_) => Scaffold(body: screen)),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBody: true,
      body: IndexedStack(
        index: _selectedIndex,
        children: _screens,
      ),
      bottomNavigationBar: _buildGlassBottomNav(),
    );
  }

  Widget _buildGlassBottomNav() {
    return Container(
      margin: const EdgeInsets.fromLTRB(10, 0, 10, 14),
      decoration: GlassEffect.elevated,
      child: ClipRRect(
        borderRadius: BorderRadius.circular(22),
        child: BottomNavigationBar(
          currentIndex: _selectedIndex,
          onTap: (index) => setState(() => _selectedIndex = index),
          backgroundColor: Colors.transparent,
          elevation: 0,
          type: BottomNavigationBarType.fixed,
          selectedItemColor: CroenixColors.primaryCyan,
          unselectedItemColor: Colors.white54,
          selectedFontSize: 10,
          unselectedFontSize: 9,
          items: const [
            BottomNavigationBarItem(
              icon: Icon(Icons.dashboard_rounded, size: 20),
              label: 'Home',
            ),
            BottomNavigationBarItem(
              icon: Icon(Icons.settings_remote_rounded, size: 20),
              label: 'Remote',
            ),
            BottomNavigationBarItem(
              icon: Icon(Icons.music_note_rounded, size: 20),
              label: 'Player',
            ),
            BottomNavigationBarItem(
              icon: Icon(Icons.sd_storage_rounded, size: 20),
              label: 'SD Card',
            ),
            BottomNavigationBarItem(
              icon: Icon(Icons.cloud_rounded, size: 20),
              label: 'SoundCloud',
            ),
            BottomNavigationBarItem(
              icon: Icon(Icons.radio_rounded, size: 20),
              label: 'FM Radio',
            ),
          ],
        ),
      ),
    );
  }
}

class _HomeTab extends StatelessWidget {
  final ValueChanged<int> onNavigate;
  final VoidCallback onOpenEqualizer;
  final VoidCallback onOpenSettings;
  final VoidCallback onOpenRemote;

  const _HomeTab({
    required this.onNavigate,
    required this.onOpenEqualizer,
    required this.onOpenSettings,
    required this.onOpenRemote,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: const BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            CroenixColors.darkBackground,
            Color(0xFF0F1523),
          ],
        ),
      ),
      child: SafeArea(
        child: CustomScrollView(
          physics: const BouncingScrollPhysics(),
          slivers: [
            // App Bar with Quick Studio Controls
            SliverAppBar(
              expandedHeight: 90,
              floating: false,
              pinned: true,
              backgroundColor: Colors.transparent,
              actions: [
                IconButton(
                  icon: const Icon(Icons.settings_remote_rounded,
                      color: CroenixColors.primaryCyan),
                  tooltip: 'Virtual Remote',
                  onPressed: onOpenRemote,
                ),
                IconButton(
                  icon: const Icon(Icons.tune_rounded, color: CroenixColors.secondaryPurple),
                  tooltip: 'Equalizer',
                  onPressed: onOpenEqualizer,
                ),
                IconButton(
                  icon: const Icon(Icons.settings_rounded, color: Colors.white70),
                  tooltip: 'Settings',
                  onPressed: onOpenSettings,
                ),
                const SizedBox(width: 8),
              ],
              flexibleSpace: FlexibleSpaceBar(
                title: ShaderMask(
                  shaderCallback: (bounds) => CroenixColors.primaryGradient.createShader(bounds),
                  child: const Text(
                    'CROENIX AUDIO',
                    style: TextStyle(
                      fontSize: 22,
                      fontWeight: FontWeight.bold,
                      letterSpacing: 1.5,
                      color: Colors.white,
                    ),
                  ),
                ),
                centerTitle: false,
                titlePadding: const EdgeInsets.only(left: 20, bottom: 14),
              ),
            ),

            // Content
            SliverPadding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              sliver: SliverList(
                delegate: SliverChildListDelegate([
                  const SizedBox(height: 4),
                  const ConnectionStatusCard(),
                  const SizedBox(height: 14),
                  const NowPlayingCard(),
                  const SizedBox(height: 14),
                  _buildVirtualRemoteCard(context),
                  const SizedBox(height: 14),
                  _buildSourceShortcuts(context),
                  const SizedBox(height: 14),
                  const QuickControlsPanel(),
                  const SizedBox(height: 100), // Bottom nav clearance
                ]),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildVirtualRemoteCard(BuildContext context) {
    return GestureDetector(
      onTap: onOpenRemote,
      child: Container(
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: const Color(0xFF111728),
          borderRadius: BorderRadius.circular(20),
          border: Border.all(
              color: CroenixColors.primaryCyan.withOpacity(0.4), width: 1.5),
          boxShadow: [
            BoxShadow(
              color: CroenixColors.primaryCyan.withOpacity(0.08),
              blurRadius: 16,
              spreadRadius: 1,
            ),
          ],
        ),
        child: Row(
          children: [
            Container(
              width: 50,
              height: 50,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                gradient: const LinearGradient(
                  colors: [
                    CroenixColors.primaryCyan,
                    CroenixColors.secondaryPurple
                  ],
                ),
                boxShadow: [
                  BoxShadow(
                    color: CroenixColors.primaryCyan.withOpacity(0.35),
                    blurRadius: 12,
                  ),
                ],
              ),
              child: const Icon(Icons.settings_remote_rounded,
                  color: Colors.white, size: 26),
            ),
            const SizedBox(width: 14),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      const Text(
                        'VIRTUAL REMOTE',
                        style: TextStyle(
                          color: Colors.white,
                          fontWeight: FontWeight.bold,
                          fontSize: 14,
                          letterSpacing: 1.1,
                        ),
                      ),
                      const SizedBox(width: 6),
                      Container(
                        padding: const EdgeInsets.symmetric(
                            horizontal: 6, vertical: 2),
                        decoration: BoxDecoration(
                          color: CroenixColors.primaryCyan.withOpacity(0.2),
                          borderRadius: BorderRadius.circular(6),
                        ),
                        child: const Text(
                          'HARDWARE',
                          style: TextStyle(
                              color: CroenixColors.primaryCyan,
                              fontSize: 8,
                              fontWeight: FontWeight.bold),
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 4),
                  const Text(
                    'Rotary Jog Wheel • Screen Switcher • Master Volume',
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(color: Colors.white54, fontSize: 11),
                  ),
                ],
              ),
            ),
            const Icon(Icons.arrow_forward_ios_rounded,
                color: CroenixColors.primaryCyan, size: 16),
          ],
        ),
      ),
    );
  }

  Widget _buildSourceShortcuts(BuildContext context) {
    return Row(
      children: [
        _buildNavCard(
          title: 'SD Card',
          subtitle: 'MP3/FLAC',
          icon: Icons.sd_storage_rounded,
          color: CroenixColors.sourceSDCard,
          onTap: () => onNavigate(3),
        ),
        const SizedBox(width: 10),
        _buildNavCard(
          title: 'SoundCloud',
          subtitle: 'Web Playlists',
          icon: Icons.cloud_rounded,
          color: CroenixColors.soundCloudOrange,
          onTap: () => onNavigate(4),
        ),
        const SizedBox(width: 10),
        _buildNavCard(
          title: 'FM Radio',
          subtitle: 'Si4703 Tuner',
          icon: Icons.radio_rounded,
          color: CroenixColors.sourceFM,
          onTap: () => onNavigate(5),
        ),
      ],
    );
  }

  Widget _buildNavCard({
    required String title,
    required String subtitle,
    required IconData icon,
    required Color color,
    required VoidCallback onTap,
  }) {
    return Expanded(
      child: GestureDetector(
        onTap: onTap,
        child: Container(
          padding: const EdgeInsets.symmetric(vertical: 14, horizontal: 8),
          decoration: BoxDecoration(
            color: color.withOpacity(0.08),
            borderRadius: BorderRadius.circular(16),
            border: Border.all(color: color.withOpacity(0.25)),
          ),
          child: Column(
            children: [
              Icon(icon, color: color, size: 24),
              const SizedBox(height: 6),
              Text(
                title,
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
                style: const TextStyle(color: Colors.white, fontSize: 12, fontWeight: FontWeight.bold),
              ),
              const SizedBox(height: 2),
              Text(
                subtitle,
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
                style: const TextStyle(color: Colors.white38, fontSize: 10),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
