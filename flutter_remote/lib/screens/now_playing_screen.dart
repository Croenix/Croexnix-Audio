import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/player_state.dart';
import '../services/esp32_service.dart';
import '../theme/app_theme.dart';
import '../widgets/album_art_card.dart';
import 'connection_sheet.dart';

class NowPlayingScreen extends StatefulWidget {
  final VoidCallback onOpenSdBrowser;

  const NowPlayingScreen({
    super.key,
    required this.onOpenSdBrowser,
  });

  @override
  State<NowPlayingScreen> createState() => _NowPlayingScreenState();
}

class _NowPlayingScreenState extends State<NowPlayingScreen> {
  double? _dragSeekValue;
  bool _isDraggingSeek = false;

  @override
  Widget build(BuildContext context) {
    final espService = context.watch<Esp32Service>();
    final state = espService.playerState;
    final isConnected = espService.isConnected;

    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final isLandscape = screenWidth > screenHeight;

    final albumSize = isLandscape ? (screenHeight * 0.58) : (screenWidth * 0.72).clamp(220.0, 320.0);

    return Scaffold(
      body: SafeArea(
        child: Container(
          decoration: const BoxDecoration(
            gradient: RadialGradient(
              center: Alignment(0.0, -0.4),
              radius: 1.1,
              colors: [
                Color(0xFF22133D),
                AppTheme.background,
              ],
            ),
          ),
          child: Column(
            children: [
              // Header Status Bar
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 20.0, vertical: 12.0),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    // Connection Status Pill
                    GestureDetector(
                      onTap: () => ConnectionSheet.show(context),
                      child: Container(
                        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                        decoration: BoxDecoration(
                          color: AppTheme.surfaceHighlight.withOpacity(0.85),
                          borderRadius: BorderRadius.circular(20),
                          border: Border.all(
                            color: isConnected
                                ? AppTheme.borderGlow.withOpacity(0.6)
                                : Colors.orangeAccent.withOpacity(0.5),
                          ),
                        ),
                        child: Row(
                          mainAxisSize: MainAxisSize.min,
                          children: [
                            Icon(
                              Icons.wifi,
                              size: 14,
                              color: isConnected ? AppTheme.accentCyan : Colors.orangeAccent,
                            ),
                            const SizedBox(width: 8),
                            Text(
                              espService.espIp,
                              style: const TextStyle(
                                fontSize: 12,
                                fontWeight: FontWeight.w600,
                                color: Colors.white,
                              ),
                            ),
                          ],
                        ),
                      ),
                    ),

                    // Title
                    const Text(
                      'CROENIX AUDIO',
                      style: TextStyle(
                        fontSize: 13,
                        fontWeight: FontWeight.w700,
                        letterSpacing: 2.2,
                        color: AppTheme.textSecondary,
                      ),
                    ),

                    // SD Card Files Icon Button
                    IconButton(
                      icon: const Icon(Icons.folder_open, color: AppTheme.primaryNeon),
                      tooltip: 'SD Card Files',
                      onPressed: widget.onOpenSdBrowser,
                    ),
                  ],
                ),
              ),

              // Main Body Content
              Expanded(
                child: SingleChildScrollView(
                  physics: const BouncingScrollPhysics(),
                  padding: const EdgeInsets.symmetric(horizontal: 24.0),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      const SizedBox(height: 12),

                      // Album Art Container with Glowing Rounded Border and Waveform Bar Overlay
                      AlbumArtCard(
                        isPlaying: state.isPlaying,
                        size: albumSize,
                      ),

                      const SizedBox(height: 28),

                      // Song Title & Artist (Exact typography as reference image)
                      Padding(
                        padding: const EdgeInsets.symmetric(horizontal: 8.0),
                        child: Column(
                          children: [
                            Text(
                              state.title,
                              textAlign: TextAlign.center,
                              maxLines: 1,
                              overflow: TextOverflow.ellipsis,
                              style: const TextStyle(
                                fontSize: 26,
                                fontWeight: FontWeight.w700,
                                letterSpacing: -0.5,
                                color: AppTheme.textPrimary,
                              ),
                            ),
                            const SizedBox(height: 6),
                            Text(
                              state.artist,
                              textAlign: TextAlign.center,
                              maxLines: 1,
                              overflow: TextOverflow.ellipsis,
                              style: TextStyle(
                                fontSize: 16,
                                fontWeight: FontWeight.w400,
                                color: AppTheme.textSecondary.withOpacity(0.9),
                              ),
                            ),
                          ],
                        ),
                      ),

                      const SizedBox(height: 20),

                      // Progress Bar & Elapsed / Negative Remaining Time
                      Padding(
                        padding: const EdgeInsets.symmetric(horizontal: 4.0),
                        child: Column(
                          children: [
                            // Time row (0:51 on left, -1:42 on right, as in reference image)
                            Row(
                              mainAxisAlignment: MainAxisAlignment.spaceBetween,
                              children: [
                                Text(
                                  _isDraggingSeek && _dragSeekValue != null
                                      ? PlayerStateModel.formatSeconds(_dragSeekValue!.toInt())
                                      : state.formattedCurrentTime,
                                  style: TextStyle(
                                    fontSize: 13,
                                    fontWeight: FontWeight.w500,
                                    color: Colors.white.withOpacity(0.85),
                                  ),
                                ),
                                Text(
                                  _isDraggingSeek && _dragSeekValue != null && state.duration > 0
                                      ? '-${PlayerStateModel.formatSeconds((state.duration - _dragSeekValue!.toInt()).clamp(0, state.duration))}'
                                      : state.formattedRemainingTime,
                                  style: TextStyle(
                                    fontSize: 13,
                                    fontWeight: FontWeight.w500,
                                    color: Colors.white.withOpacity(0.85),
                                  ),
                                ),
                              ],
                            ),
                            const SizedBox(height: 4),

                            // Seek Slider
                            SliderTheme(
                              data: SliderTheme.of(context).copyWith(
                                trackHeight: 3.5,
                                thumbShape: const RoundSliderThumbShape(enabledThumbRadius: 6),
                                overlayShape: const RoundSliderOverlayShape(overlayRadius: 14),
                                activeTrackColor: Colors.white,
                                inactiveTrackColor: const Color(0xFF332A52),
                                thumbColor: Colors.white,
                              ),
                              child: Slider(
                                value: _isDraggingSeek && _dragSeekValue != null
                                    ? _dragSeekValue!
                                    : (state.currentTime.toDouble()).clamp(0.0, state.duration > 0 ? state.duration.toDouble() : 1.0),
                                min: 0.0,
                                max: state.duration > 0 ? state.duration.toDouble() : 1.0,
                                onChangeStart: (val) {
                                  setState(() {
                                    _isDraggingSeek = true;
                                    _dragSeekValue = val;
                                  });
                                },
                                onChanged: (val) {
                                  setState(() {
                                    _dragSeekValue = val;
                                  });
                                },
                                onChangeEnd: (val) {
                                  setState(() {
                                    _isDraggingSeek = false;
                                  });
                                  espService.seekTo(val.toInt());
                                },
                              ),
                            ),
                          ],
                        ),
                      ),

                      const SizedBox(height: 12),

                      // Virtual Remote Transport Controls Dock
                      Container(
                        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                        decoration: BoxDecoration(
                          color: AppTheme.surface.withOpacity(0.6),
                          borderRadius: BorderRadius.circular(24),
                          border: Border.all(
                            color: AppTheme.borderCard.withOpacity(0.35),
                          ),
                        ),
                        child: Row(
                          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                          children: [
                            // Previous Track
                            IconButton(
                              iconSize: 34,
                              icon: const Icon(Icons.skip_previous_rounded),
                              color: Colors.white,
                              tooltip: 'Previous Track',
                              onPressed: () => espService.prevTrack(),
                            ),

                            // Play / Pause Glowing Button
                            GestureDetector(
                              onTap: () => espService.togglePlayPause(),
                              child: Container(
                                width: 68,
                                height: 68,
                                decoration: BoxDecoration(
                                  shape: BoxShape.circle,
                                  gradient: const LinearGradient(
                                    colors: [
                                      AppTheme.primaryNeon,
                                      AppTheme.secondaryNeon,
                                    ],
                                    begin: Alignment.topLeft,
                                    end: Alignment.bottomRight,
                                  ),
                                  boxShadow: [
                                    BoxShadow(
                                      color: AppTheme.primaryNeon.withOpacity(0.4),
                                      blurRadius: 18,
                                      spreadRadius: 2,
                                    ),
                                  ],
                                ),
                                child: Icon(
                                  state.isPlaying ? Icons.pause_rounded : Icons.play_arrow_rounded,
                                  size: 38,
                                  color: Colors.white,
                                ),
                              ),
                            ),

                            // Next Track
                            IconButton(
                              iconSize: 34,
                              icon: const Icon(Icons.skip_next_rounded),
                              color: Colors.white,
                              tooltip: 'Next Track',
                              onPressed: () => espService.nextTrack(),
                            ),
                          ],
                        ),
                      ),

                      const SizedBox(height: 20),

                      // Volume Slider Card
                      Container(
                        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                        decoration: BoxDecoration(
                          color: AppTheme.surfaceHighlight.withOpacity(0.5),
                          borderRadius: BorderRadius.circular(16),
                        ),
                        child: Row(
                          children: [
                            const Icon(
                              Icons.volume_down_rounded,
                              size: 20,
                              color: AppTheme.textSecondary,
                            ),
                            Expanded(
                              child: Slider(
                                value: state.volume.toDouble().clamp(0.0, 100.0),
                                min: 0.0,
                                max: 100.0,
                                activeColor: AppTheme.primaryNeon,
                                inactiveColor: const Color(0xFF332A52),
                                onChanged: (val) {
                                  espService.setVolume(val.toInt());
                                },
                              ),
                            ),
                            const Icon(
                              Icons.volume_up_rounded,
                              size: 20,
                              color: AppTheme.textSecondary,
                            ),
                            const SizedBox(width: 8),
                            SizedBox(
                              width: 36,
                              child: Text(
                                '${state.volume}%',
                                style: const TextStyle(
                                  fontSize: 12,
                                  fontWeight: FontWeight.w600,
                                  color: AppTheme.textSecondary,
                                ),
                              ),
                            ),
                          ],
                        ),
                      ),

                      const SizedBox(height: 24),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
