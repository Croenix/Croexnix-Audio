import 'package:flutter/material.dart';
import 'dart:math' as math;
import '../../core/theme/croenix_colors.dart';
import '../../data/services/websocket_service.dart';
import '../../data/models/device_state.dart';
import 'widgets/audio_visualizer.dart';
import 'widgets/control_buttons.dart';
import 'widgets/track_progress_bar.dart';
import 'widgets/volume_control.dart';

class PlayerScreen extends StatefulWidget {
  const PlayerScreen({super.key});

  @override
  State<PlayerScreen> createState() => _PlayerScreenState();
}

class _PlayerScreenState extends State<PlayerScreen>
    with TickerProviderStateMixin {
  final WebSocketService _wsService = WebSocketService();
  DeviceState _deviceState = const DeviceState();

  late AnimationController _rotationController;
  late AnimationController _waveController;

  @override
  void initState() {
    super.initState();

    _rotationController = AnimationController(
      duration: const Duration(seconds: 20),
      vsync: this,
    )..repeat();

    _waveController = AnimationController(
      duration: const Duration(milliseconds: 1500),
      vsync: this,
    )..repeat(reverse: true);

    // Listen to device state updates
    _wsService.stateStream.listen((state) {
      setState(() => _deviceState = state);

      if (state.isPlaying) {
        _rotationController.repeat();
      } else {
        _rotationController.stop();
      }
    });
  }

  @override
  void dispose() {
    _rotationController.dispose();
    _waveController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            CroenixColors.darkBackground,
            const Color(0xFF0F1523),
            CroenixColors.darkBackground,
          ],
          stops: const [0.0, 0.5, 1.0],
        ),
      ),
      child: SafeArea(
        child: CustomScrollView(
          physics: const BouncingScrollPhysics(),
          slivers: [
            // App Bar
            SliverAppBar(
              floating: true,
              backgroundColor: Colors.transparent,
              elevation: 0,
              centerTitle: true,
              title: _buildSourceBadge(),
              leading: IconButton(
                icon: const Icon(Icons.keyboard_arrow_down_rounded),
                onPressed: () => Navigator.pop(context),
              ),
              actions: [
                IconButton(
                  icon: const Icon(Icons.more_vert_rounded),
                  onPressed: _showOptionsMenu,
                ),
              ],
            ),

            SliverPadding(
              padding: const EdgeInsets.symmetric(horizontal: 24),
              sliver: SliverList(
                delegate: SliverChildListDelegate([
                  const SizedBox(height: 20),

                  // Album Art / Visualizer
                  _buildAlbumArtSection(),

                  const SizedBox(height: 40),

                  // Track Info
                  _buildTrackInfo(),

                  const SizedBox(height: 32),

                  // Progress Bar
                  TrackProgressBar(
                    position: _deviceState.trackPosition,
                    duration: _deviceState.trackDuration,
                    onSeek: (position) {
                      // TODO: Implement seek functionality
                    },
                  ),

                  const SizedBox(height: 40),

                  // Playback Controls
                  ControlButtons(
                    isPlaying: _deviceState.isPlaying,
                    onPlay: () => _wsService.play(),
                    onPause: () => _wsService.pause(),
                    onPrevious: () {
                      // TODO: Implement previous track
                    },
                    onNext: () {
                      // TODO: Implement next track
                    },
                  ),

                  const SizedBox(height: 32),

                  // Volume Control
                  VolumeControl(
                    volume: _deviceState.volume,
                    onVolumeChanged: (volume) {
                      _wsService.setVolume(volume.round());
                    },
                  ),

                  const SizedBox(height: 24),

                  // EQ & Source Controls
                  _buildBottomControls(),

                  const SizedBox(height: 100),
                ]),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSourceBadge() {
    String sourceText = '';
    Color sourceColor = CroenixColors.textSecondary;
    IconData sourceIcon = Icons.music_note_rounded;

    switch (_deviceState.currentSource) {
      case AudioSource.bluetooth:
        sourceText = 'Bluetooth';
        sourceColor = CroenixColors.sourceBluetooth;
        sourceIcon = Icons.bluetooth_audio_rounded;
        break;
      case AudioSource.sdCard:
        sourceText = 'SD Card';
        sourceColor = CroenixColors.sourceSDCard;
        sourceIcon = Icons.sd_card_rounded;
        break;
      case AudioSource.fmRadio:
        sourceText = 'FM Radio';
        sourceColor = CroenixColors.sourceFM;
        sourceIcon = Icons.radio_rounded;
        break;
      case AudioSource.webStream:
        sourceText = 'Web Stream';
        sourceColor = CroenixColors.sourceWebStream;
        sourceIcon = Icons.language_rounded;
        break;
      default:
        sourceText = 'No Source';
    }

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      decoration: BoxDecoration(
        color: sourceColor.withValues(alpha: 0.2),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(
          color: sourceColor.withValues(alpha: 0.5),
          width: 1,
        ),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(sourceIcon, size: 16, color: sourceColor),
          const SizedBox(width: 8),
          Text(
            sourceText,
            style: TextStyle(
              fontSize: 12,
              fontWeight: FontWeight.w600,
              color: sourceColor,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildAlbumArtSection() {
    if (_deviceState.isPlaying) {
      return AudioVisualizer(
        isPlaying: _deviceState.isPlaying,
        animationController: _waveController,
      );
    }

    return Center(
      child: Container(
        width: 280,
        height: 280,
        decoration: GlassEffect.elevated,
        child: ClipRRect(
          borderRadius: BorderRadius.circular(20),
          child: Stack(
            alignment: Alignment.center,
            children: [
              // Rotating gradient background
              AnimatedBuilder(
                animation: _rotationController,
                builder: (context, child) {
                  return Transform.rotate(
                    angle: _rotationController.value * 2 * math.pi,
                    child: Container(
                      decoration: const BoxDecoration(
                        gradient: CroenixColors.primaryGradient,
                      ),
                    ),
                  );
                },
              ),

              // Album art placeholder
              Container(
                margin: const EdgeInsets.all(40),
                decoration: BoxDecoration(
                  color: CroenixColors.darkSurface,
                  borderRadius: BorderRadius.circular(12),
                ),
                child: const Icon(
                  Icons.music_note_rounded,
                  size: 100,
                  color: CroenixColors.textSecondary,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildTrackInfo() {
    return Column(
      children: [
        // Track title
        Text(
          _deviceState.trackTitle.isNotEmpty
              ? _deviceState.trackTitle
              : 'No Track Playing',
          textAlign: TextAlign.center,
          maxLines: 2,
          overflow: TextOverflow.ellipsis,
          style: const TextStyle(
            fontSize: 24,
            fontWeight: FontWeight.bold,
            color: CroenixColors.textPrimary,
            height: 1.2,
          ),
        ),

        const SizedBox(height: 12),

        // Artist
        Text(
          _deviceState.trackArtist.isNotEmpty
              ? _deviceState.trackArtist
              : '---',
          textAlign: TextAlign.center,
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
          style: const TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.w500,
            color: CroenixColors.textSecondary,
          ),
        ),

        const SizedBox(height: 8),

        // Bitrate
        if (_deviceState.trackBitrate > 0)
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
            decoration: BoxDecoration(
              color: CroenixColors.glassBackground,
              borderRadius: BorderRadius.circular(12),
              border: Border.all(
                color: CroenixColors.glassBorder,
                width: 0.5,
              ),
            ),
            child: Text(
              '${_deviceState.trackBitrate} kbps',
              style: const TextStyle(
                fontSize: 12,
                fontWeight: FontWeight.w500,
                color: CroenixColors.primaryCyan,
              ),
            ),
          ),
      ],
    );
  }

  Widget _buildBottomControls() {
    final eqNames = ['Flat', 'Rock', 'Pop', 'Classical', 'Jazz', 'Bass'];

    return Container(
      padding: const EdgeInsets.all(20),
      decoration: GlassEffect.standard,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: [
          // EQ Preset
          _buildControlButton(
            icon: Icons.equalizer_rounded,
            label: eqNames[_deviceState.eqPreset],
            onTap: _showEQPresets,
          ),

          Container(
            width: 1,
            height: 40,
            color: CroenixColors.divider,
          ),

          // Source Selector
          _buildControlButton(
            icon: Icons.input_rounded,
            label: 'Source',
            onTap: _showSourceSelector,
          ),
        ],
      ),
    );
  }

  Widget _buildControlButton({
    required IconData icon,
    required String label,
    required VoidCallback onTap,
  }) {
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(12),
      child: Padding(
        padding: const EdgeInsets.all(8),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(icon, size: 24, color: CroenixColors.primaryCyan),
            const SizedBox(height: 4),
            Text(
              label,
              style: const TextStyle(
                fontSize: 12,
                color: CroenixColors.textSecondary,
              ),
            ),
          ],
        ),
      ),
    );
  }

  void _showOptionsMenu() {
    // TODO: Show options menu
  }

  void _showEQPresets() {
    showModalBottomSheet(
      context: context,
      backgroundColor: Colors.transparent,
      builder: (context) => _buildEQBottomSheet(),
    );
  }

  void _showSourceSelector() {
    showModalBottomSheet(
      context: context,
      backgroundColor: Colors.transparent,
      builder: (context) => _buildSourceBottomSheet(),
    );
  }

  Widget _buildEQBottomSheet() {
    final eqPresets = [
      {'name': 'Flat', 'icon': Icons.straighten_rounded},
      {'name': 'Rock', 'icon': Icons.graphic_eq_rounded},
      {'name': 'Pop', 'icon': Icons.music_note_rounded},
      {'name': 'Classical', 'icon': Icons.piano_rounded},
      {'name': 'Jazz', 'icon': Icons.album_rounded},
      {'name': 'Bass Boost', 'icon': Icons.speaker_rounded},
    ];

    return Container(
      margin: const EdgeInsets.all(16),
      padding: const EdgeInsets.all(24),
      decoration: GlassEffect.elevated,
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Equalizer Presets',
            style: TextStyle(
              fontSize: 20,
              fontWeight: FontWeight.bold,
              color: CroenixColors.textPrimary,
            ),
          ),
          const SizedBox(height: 20),
          ...List.generate(eqPresets.length, (index) {
            final preset = eqPresets[index];
            final isSelected = _deviceState.eqPreset == index;

            return ListTile(
              leading: Icon(
                preset['icon'] as IconData,
                color: isSelected
                    ? CroenixColors.primaryCyan
                    : CroenixColors.textSecondary,
              ),
              title: Text(
                preset['name'] as String,
                style: TextStyle(
                  color: isSelected
                      ? CroenixColors.primaryCyan
                      : CroenixColors.textPrimary,
                  fontWeight: isSelected ? FontWeight.w600 : FontWeight.normal,
                ),
              ),
              trailing: isSelected
                  ? const Icon(
                      Icons.check_circle_rounded,
                      color: CroenixColors.primaryCyan,
                    )
                  : null,
              onTap: () {
                _wsService.setEQPreset(index);
                Navigator.pop(context);
              },
            );
          }),
          const SizedBox(height: 20),
        ],
      ),
    );
  }

  Widget _buildSourceBottomSheet() {
    final sources = [
      {
        'name': 'Bluetooth',
        'icon': Icons.bluetooth_audio_rounded,
        'source': AudioSource.bluetooth,
        'color': CroenixColors.sourceBluetooth,
      },
      {
        'name': 'SD Card',
        'icon': Icons.sd_card_rounded,
        'source': AudioSource.sdCard,
        'color': CroenixColors.sourceSDCard,
      },
      {
        'name': 'FM Radio',
        'icon': Icons.radio_rounded,
        'source': AudioSource.fmRadio,
        'color': CroenixColors.sourceFM,
      },
      {
        'name': 'Web Stream',
        'icon': Icons.language_rounded,
        'source': AudioSource.webStream,
        'color': CroenixColors.sourceWebStream,
      },
    ];

    return Container(
      margin: const EdgeInsets.all(16),
      padding: const EdgeInsets.all(24),
      decoration: GlassEffect.elevated,
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Audio Source',
            style: TextStyle(
              fontSize: 20,
              fontWeight: FontWeight.bold,
              color: CroenixColors.textPrimary,
            ),
          ),
          const SizedBox(height: 20),
          ...sources.map((source) {
            final isSelected = _deviceState.currentSource == source['source'];

            return ListTile(
              leading: Icon(
                source['icon'] as IconData,
                color: source['color'] as Color,
              ),
              title: Text(
                source['name'] as String,
                style: TextStyle(
                  color: isSelected
                      ? CroenixColors.primaryCyan
                      : CroenixColors.textPrimary,
                  fontWeight: isSelected ? FontWeight.w600 : FontWeight.normal,
                ),
              ),
              trailing: isSelected
                  ? const Icon(
                      Icons.check_circle_rounded,
                      color: CroenixColors.primaryCyan,
                    )
                  : null,
              onTap: () {
                _wsService.setSource(source['source'] as AudioSource);
                Navigator.pop(context);
              },
            );
          }),
          const SizedBox(height: 20),
        ],
      ),
    );
  }
}
