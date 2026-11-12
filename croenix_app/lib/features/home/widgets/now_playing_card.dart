import 'package:flutter/material.dart';
import '../../../core/theme/croenix_colors.dart';
import '../../../data/services/websocket_service.dart';
import '../../../data/models/device_state.dart';
import '../../player/player_screen.dart';

class NowPlayingCard extends StatefulWidget {
  const NowPlayingCard({super.key});

  @override
  State<NowPlayingCard> createState() => _NowPlayingCardState();
}

class _NowPlayingCardState extends State<NowPlayingCard> {
  final WebSocketService _wsService = WebSocketService();
  DeviceState _deviceState = const DeviceState();

  @override
  void initState() {
    super.initState();
    _deviceState = _wsService.currentState;
    _wsService.stateStream.listen((state) {
      if (mounted) setState(() => _deviceState = state);
    });
  }

  @override
  Widget build(BuildContext context) {
    final hasTrack = _deviceState.trackTitle.isNotEmpty;
    final title = hasTrack
        ? _deviceState.trackTitle
        : (_deviceState.currentSource == AudioSource.fmRadio
            ? 'FM ${_deviceState.fmFrequency.toStringAsFixed(1)} MHz'
            : (_deviceState.currentSource == AudioSource.bluetooth
                ? 'Bluetooth Audio'
                : 'No Track Playing'));

    final artist = hasTrack
        ? (_deviceState.trackArtist.isNotEmpty ? _deviceState.trackArtist : _deviceState.deviceName)
        : (_deviceState.currentSource == AudioSource.fmRadio
            ? (_deviceState.fmStationName.isNotEmpty ? _deviceState.fmStationName : 'Radio Mode')
            : 'Croenix Audio');

    return GestureDetector(
      onTap: () {
        Navigator.push(
          context,
          MaterialPageRoute(builder: (context) => const PlayerScreen()),
        );
      },
      child: Container(
        padding: const EdgeInsets.all(16),
        decoration: GlassEffect.elevated,
        child: Row(
          children: [
            // Album Art
            Container(
              width: 56,
              height: 56,
              decoration: BoxDecoration(
                gradient: CroenixColors.primaryGradient,
                borderRadius: BorderRadius.circular(12),
              ),
              child: Icon(
                _deviceState.currentSource == AudioSource.fmRadio
                    ? Icons.radio_rounded
                    : (_deviceState.currentSource == AudioSource.bluetooth
                        ? Icons.bluetooth_audio_rounded
                        : Icons.music_note_rounded),
                color: Colors.white,
                size: 28,
              ),
            ),
            const SizedBox(width: 14),

            // Track Info
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    _deviceState.isPlaying ? 'NOW PLAYING' : 'PAUSED',
                    style: TextStyle(
                      fontSize: 11,
                      fontWeight: FontWeight.w600,
                      letterSpacing: 0.8,
                      color: _deviceState.isPlaying
                          ? CroenixColors.primaryCyan
                          : CroenixColors.textSecondary,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    title,
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: const TextStyle(
                      fontSize: 15,
                      fontWeight: FontWeight.bold,
                      color: CroenixColors.textPrimary,
                    ),
                  ),
                  const SizedBox(height: 2),
                  Text(
                    artist,
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: const TextStyle(
                      fontSize: 12,
                      color: CroenixColors.textSecondary,
                    ),
                  ),
                ],
              ),
            ),

            const SizedBox(width: 10),

            // Play / Pause Button
            GestureDetector(
              onTap: () {
                if (_deviceState.isPlaying) {
                  _wsService.pause();
                } else {
                  _wsService.play();
                }
              },
              child: Container(
                width: 44,
                height: 44,
                decoration: const BoxDecoration(
                  gradient: CroenixColors.primaryGradient,
                  shape: BoxShape.circle,
                ),
                child: Icon(
                  _deviceState.isPlaying ? Icons.pause_rounded : Icons.play_arrow_rounded,
                  color: Colors.white,
                  size: 26,
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
