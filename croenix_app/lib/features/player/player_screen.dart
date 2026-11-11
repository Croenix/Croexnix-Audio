import 'package:flutter/material.dart';
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

  // Preset FM frequencies
  final List<double> _fmPresets = [88.5, 91.1, 98.3, 101.1, 104.0, 107.5];

  // Mock / Cached SD Files for quick browsing
  final List<Map<String, String>> _sdPlaylist = [
    {
      'title': 'Cyber Ambient Track 01',
      'artist': 'Croenix Labs',
      'album': 'Hardware OST',
      'path': '/sd/track01.mp3'
    },
    {
      'title': 'Deep Synthwave Groove',
      'artist': 'DSP Engine',
      'album': 'Audio Engine',
      'path': '/sd/synthwave.flac'
    },
    {
      'title': 'Hi-Fi Acoustic Session',
      'artist': 'PCM5102 Studio',
      'album': 'Lossless Master',
      'path': '/sd/acoustic.wav'
    },
    {
      'title': 'Neon Horizon Beats',
      'artist': 'ESP32 Audio',
      'album': 'Croenix Mix',
      'path': '/sd/neon.mp3'
    },
  ];

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

    _deviceState = _wsService.currentState;

    _wsService.stateStream.listen((state) {
      if (mounted) {
        setState(() => _deviceState = state);
        if (state.isPlaying) {
          _rotationController.repeat();
        } else {
          _rotationController.stop();
        }
      }
    });
  }

  @override
  void dispose() {
    _rotationController.dispose();
    _waveController.dispose();
    super.dispose();
  }

  void _showDirectFrequencyDialog() {
    final controller = TextEditingController(
        text: _deviceState.fmFrequency.toStringAsFixed(1));
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        backgroundColor: const Color(0xFF141927),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(18),
          side: const BorderSide(color: CroenixColors.sourceFM, width: 1.5),
        ),
        title: const Row(
          children: [
            Icon(Icons.dialpad_rounded, color: CroenixColors.sourceFM),
            SizedBox(width: 10),
            Text('Direct FM Frequency Entry',
                style: TextStyle(color: Colors.white, fontSize: 16)),
          ],
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text(
              'Enter FM Frequency (87.5 - 108.0 MHz):',
              style: TextStyle(color: Colors.white70, fontSize: 13),
            ),
            const SizedBox(height: 14),
            TextField(
              controller: controller,
              keyboardType:
                  const TextInputType.numberWithOptions(decimal: true),
              style: const TextStyle(
                  color: Colors.white,
                  fontSize: 22,
                  fontWeight: FontWeight.bold),
              textAlign: TextAlign.center,
              decoration: InputDecoration(
                suffixText: 'MHz',
                suffixStyle: const TextStyle(color: CroenixColors.sourceFM),
                filled: true,
                fillColor: const Color(0xFF1A2236),
                border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(12),
                    borderSide: BorderSide.none),
              ),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child:
                const Text('Cancel', style: TextStyle(color: Colors.white60)),
          ),
          ElevatedButton(
            style: ElevatedButton.styleFrom(
                backgroundColor: CroenixColors.sourceFM,
                foregroundColor: Colors.black),
            onPressed: () {
              final val = double.tryParse(controller.text);
              if (val != null && val >= 87.5 && val <= 108.0) {
                _wsService.setFMFrequency(val);
                Navigator.pop(ctx);
              }
            },
            child: const Text('Tune',
                style: TextStyle(fontWeight: FontWeight.bold)),
          ),
        ],
      ),
    );
  }

  void _showSDFolderBrowser() {
    showModalBottomSheet(
      context: context,
      backgroundColor: const Color(0xFF141927),
      shape: const RoundedRectangleBorder(
          borderRadius: BorderRadius.vertical(top: Radius.circular(24))),
      builder: (ctx) => Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Row(
              children: [
                Icon(Icons.folder_special_rounded,
                    color: CroenixColors.sourceSDCard),
                SizedBox(width: 10),
                Text('SD Card File & Folder Browser',
                    style: TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                        color: Colors.white)),
              ],
            ),
            const SizedBox(height: 8),
            const Text('Folder: /sd/MUSIC (Direct I2S DMA Streaming)',
                style: TextStyle(color: Colors.white60, fontSize: 12)),
            const Divider(color: Colors.white24, height: 20),
            Expanded(
              child: ListView.separated(
                itemCount: _sdPlaylist.length,
                separatorBuilder: (_, __) =>
                    const Divider(color: Colors.white10),
                itemBuilder: (context, idx) {
                  final song = _sdPlaylist[idx];
                  return ListTile(
                    contentPadding: EdgeInsets.zero,
                    leading: Container(
                      padding: const EdgeInsets.all(8),
                      decoration: BoxDecoration(
                          color: CroenixColors.sourceSDCard
                              .withValues(alpha: 0.15),
                          borderRadius: BorderRadius.circular(8)),
                      child: const Icon(Icons.audio_file_rounded,
                          color: CroenixColors.sourceSDCard),
                    ),
                    title: Text(song['title']!,
                        style: const TextStyle(
                            color: Colors.white,
                            fontWeight: FontWeight.bold,
                            fontSize: 14)),
                    subtitle: Text('${song["artist"]} • ${song["album"]}',
                        style: const TextStyle(
                            color: Colors.white60, fontSize: 12)),
                    trailing: IconButton(
                      icon: const Icon(Icons.play_circle_fill_rounded,
                          color: CroenixColors.sourceSDCard),
                      onPressed: () {
                        _wsService.loadFile(song['path']!);
                        Navigator.pop(ctx);
                      },
                    ),
                  );
                },
              ),
            ),
          ],
        ),
      ),
    );
  }

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
            CroenixColors.darkBackground
          ],
        ),
      ),
      child: SafeArea(
        child: CustomScrollView(
          physics: const BouncingScrollPhysics(),
          slivers: [
            SliverAppBar(
              floating: true,
              backgroundColor: Colors.transparent,
              elevation: 0,
              centerTitle: true,
              title: _buildSourceBadge(),
              leading: IconButton(
                icon: const Icon(Icons.keyboard_arrow_down_rounded,
                    color: Colors.white),
                onPressed: () => Navigator.pop(context),
              ),
              actions: [
                IconButton(
                  icon: const Icon(Icons.tune_rounded,
                      color: CroenixColors.primaryCyan),
                  onPressed: _showSourceSelector,
                ),
              ],
            ),
            SliverPadding(
              padding: const EdgeInsets.symmetric(horizontal: 24),
              sliver: SliverList(
                delegate: SliverChildListDelegate([
                  const SizedBox(height: 12),
                  _buildDynamicArtwork(),
                  const SizedBox(height: 24),
                  _buildContextualRemoteDashboard(),
                  const SizedBox(height: 24),
                  VolumeControl(
                    volume: _deviceState.volume,
                    onVolumeChanged: (volume) =>
                        _wsService.setVolume(volume.round()),
                  ),
                  const SizedBox(height: 20),
                  _buildBottomControls(),
                  const SizedBox(height: 60),
                ]),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSourceBadge() {
    String sourceText = 'Bluetooth';
    Color sourceColor = CroenixColors.sourceBluetooth;
    IconData sourceIcon = Icons.bluetooth_audio_rounded;

    switch (_deviceState.currentSource) {
      case AudioSource.bluetooth:
        sourceText = 'Bluetooth A2DP';
        sourceColor = CroenixColors.sourceBluetooth;
        sourceIcon = Icons.bluetooth_audio_rounded;
        break;
      case AudioSource.sdCard:
        sourceText = 'SD Card Playback';
        sourceColor = CroenixColors.sourceSDCard;
        sourceIcon = Icons.sd_card_rounded;
        break;
      case AudioSource.fmRadio:
        sourceText = 'HW-322 FM Tuner';
        sourceColor = CroenixColors.sourceFM;
        sourceIcon = Icons.radio_rounded;
        break;
      case AudioSource.webStream:
        sourceText = 'Web FM Digital';
        sourceColor = CroenixColors.sourceWebStream;
        sourceIcon = Icons.language_rounded;
        break;
      default:
        sourceText = 'Standby';
    }

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 6),
      decoration: BoxDecoration(
        color: sourceColor.withValues(alpha: 0.15),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: sourceColor.withValues(alpha: 0.4), width: 1),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(sourceIcon, size: 14, color: sourceColor),
          const SizedBox(width: 6),
          Text(
            sourceText,
            style: TextStyle(
                fontSize: 12, fontWeight: FontWeight.bold, color: sourceColor),
          ),
        ],
      ),
    );
  }

  Widget _buildDynamicArtwork() {
    if (_deviceState.isPlaying &&
        _deviceState.currentSource != AudioSource.fmRadio) {
      return AudioVisualizer(
        isPlaying: _deviceState.isPlaying,
        animationController: _waveController,
      );
    }

    // Dynamic Artwork Centerpiece
    return Center(
      child: Container(
        width: 240,
        height: 240,
        decoration: BoxDecoration(
          color: const Color(0xFF141927),
          borderRadius: BorderRadius.circular(24),
          border: Border.all(color: Colors.white.withValues(alpha: 0.08)),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withValues(alpha: 0.4),
              blurRadius: 20,
              spreadRadius: 2,
            ),
          ],
        ),
        child: Stack(
          alignment: Alignment.center,
          children: [
            if (_deviceState.currentSource == AudioSource.fmRadio) ...[
              Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  const Icon(Icons.radio_rounded,
                      size: 48, color: CroenixColors.sourceFM),
                  const SizedBox(height: 12),
                  Text(
                    '${_deviceState.fmFrequency.toStringAsFixed(1)} MHz',
                    style: const TextStyle(
                        fontSize: 32,
                        fontWeight: FontWeight.bold,
                        color: Colors.white),
                  ),
                  const SizedBox(height: 6),
                  Text(
                    _deviceState.fmStationName.isNotEmpty
                        ? _deviceState.fmStationName
                        : 'HW-322 Stereo Tuner',
                    style: const TextStyle(fontSize: 13, color: Colors.white70),
                  ),
                ],
              ),
            ] else if (_deviceState.currentSource == AudioSource.bluetooth) ...[
              Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Container(
                    padding: const EdgeInsets.all(16),
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      color:
                          CroenixColors.sourceBluetooth.withValues(alpha: 0.15),
                    ),
                    child: const Icon(Icons.bluetooth_rounded,
                        size: 48, color: CroenixColors.sourceBluetooth),
                  ),
                  const SizedBox(height: 14),
                  const Text('CROENIX BT SINK',
                      style: TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.bold,
                          color: Colors.white)),
                  const SizedBox(height: 4),
                  Text(
                    _deviceState.bluetoothConnected
                        ? 'Connected: ${_deviceState.bluetoothDevice}'
                        : 'Discoverable (Ready)',
                    style: TextStyle(
                      fontSize: 12,
                      color: _deviceState.bluetoothConnected
                          ? CroenixColors.primaryCyan
                          : Colors.amber,
                    ),
                  ),
                ],
              ),
            ] else ...[
              const Icon(Icons.music_note_rounded,
                  size: 64, color: CroenixColors.primaryCyan),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildContextualRemoteDashboard() {
    switch (_deviceState.currentSource) {
      case AudioSource.fmRadio:
        return _buildFMRadioDashboard();
      case AudioSource.sdCard:
        return _buildSDCardDashboard();
      case AudioSource.bluetooth:
      case AudioSource.webStream:
      default:
        return _buildMediaRemoteDashboard();
    }
  }

  // -------------------------------------------------------------
  // Mode 1: FM Radio Remote Dashboard
  // -------------------------------------------------------------
  Widget _buildFMRadioDashboard() {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: const Color(0xFF141927),
        borderRadius: BorderRadius.circular(20),
        border:
            Border.all(color: CroenixColors.sourceFM.withValues(alpha: 0.3)),
      ),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              const Text('TUNER CONTROLS',
                  style: TextStyle(
                      fontSize: 13,
                      fontWeight: FontWeight.bold,
                      color: Colors.white70)),
              TextButton.icon(
                icon: const Icon(Icons.keyboard_alt_rounded,
                    size: 16, color: CroenixColors.sourceFM),
                label: const Text('Direct Entry',
                    style:
                        TextStyle(color: CroenixColors.sourceFM, fontSize: 12)),
                onPressed: _showDirectFrequencyDialog,
              ),
            ],
          ),
          const SizedBox(height: 14),
          // Auto Seek & Step Buttons
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              IconButton(
                iconSize: 32,
                tooltip: 'Seek Down',
                icon: const Icon(Icons.fast_rewind_rounded,
                    color: CroenixColors.sourceFM),
                onPressed: () => _wsService.seekFMDown(),
              ),
              OutlinedButton(
                style: OutlinedButton.styleFrom(
                  foregroundColor: Colors.white,
                  side: const BorderSide(color: CroenixColors.sourceFM),
                ),
                onPressed: () {
                  final cur = _deviceState.fmFrequency;
                  final next = (cur - 0.1 < 87.5) ? 108.0 : cur - 0.1;
                  _wsService.setFMFrequency(next);
                },
                child: const Text('- 0.1 MHz'),
              ),
              OutlinedButton(
                style: OutlinedButton.styleFrom(
                  foregroundColor: Colors.white,
                  side: const BorderSide(color: CroenixColors.sourceFM),
                ),
                onPressed: () {
                  final cur = _deviceState.fmFrequency;
                  final next = (cur + 0.1 > 108.0) ? 87.5 : cur + 0.1;
                  _wsService.setFMFrequency(next);
                },
                child: const Text('+ 0.1 MHz'),
              ),
              IconButton(
                iconSize: 32,
                tooltip: 'Seek Up',
                icon: const Icon(Icons.fast_forward_rounded,
                    color: CroenixColors.sourceFM),
                onPressed: () => _wsService.seekFMUp(),
              ),
            ],
          ),
          const SizedBox(height: 16),
          const Align(
            alignment: Alignment.centerLeft,
            child: Text('PRESET STATIONS',
                style: TextStyle(
                    fontSize: 11,
                    color: Colors.white60,
                    fontWeight: FontWeight.bold)),
          ),
          const SizedBox(height: 8),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: _fmPresets.map((f) {
              final isCurrent = (_deviceState.fmFrequency - f).abs() < 0.05;
              return ChoiceChip(
                label: Text('${f.toStringAsFixed(1)} MHz'),
                selected: isCurrent,
                selectedColor: CroenixColors.sourceFM,
                labelStyle: TextStyle(
                    color: isCurrent ? Colors.black : Colors.white,
                    fontWeight: FontWeight.bold,
                    fontSize: 11),
                backgroundColor: const Color(0xFF1B2338),
                onSelected: (_) => _wsService.setFMFrequency(f),
              );
            }).toList(),
          ),
        ],
      ),
    );
  }

  // -------------------------------------------------------------
  // Mode 2: SD Card Remote Dashboard
  // -------------------------------------------------------------
  Widget _buildSDCardDashboard() {
    return Column(
      children: [
        _buildTrackInfo(),
        const SizedBox(height: 16),
        TrackProgressBar(
          position: _deviceState.trackPosition,
          duration: _deviceState.trackDuration,
          onSeek: (pos) {},
        ),
        const SizedBox(height: 16),
        ControlButtons(
          isPlaying: _deviceState.isPlaying,
          onPlay: () => _wsService.play(),
          onPause: () => _wsService.pause(),
          onPrevious: () => _wsService.previousTrack(),
          onNext: () => _wsService.nextTrack(),
        ),
        const SizedBox(height: 16),
        SizedBox(
          width: double.infinity,
          height: 44,
          child: OutlinedButton.icon(
            style: OutlinedButton.styleFrom(
              foregroundColor: CroenixColors.sourceSDCard,
              side: const BorderSide(color: CroenixColors.sourceSDCard),
              shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(12)),
            ),
            icon: const Icon(Icons.folder_open_rounded, size: 18),
            label: const Text('Open SD Card File Browser',
                style: TextStyle(fontWeight: FontWeight.bold)),
            onPressed: _showSDFolderBrowser,
          ),
        ),
      ],
    );
  }

  // -------------------------------------------------------------
  // Mode 3: Bluetooth & Digital Media Remote Dashboard
  // -------------------------------------------------------------
  Widget _buildMediaRemoteDashboard() {
    return Column(
      children: [
        _buildTrackInfo(),
        const SizedBox(height: 20),
        ControlButtons(
          isPlaying: _deviceState.isPlaying,
          onPlay: () => _wsService.play(),
          onPause: () => _wsService.pause(),
          onPrevious: () => _wsService.previousTrack(),
          onNext: () => _wsService.nextTrack(),
        ),
      ],
    );
  }

  Widget _buildTrackInfo() {
    return Column(
      children: [
        Text(
          _deviceState.trackTitle.isNotEmpty
              ? _deviceState.trackTitle
              : 'Croenix Audio Engine',
          textAlign: TextAlign.center,
          maxLines: 2,
          overflow: TextOverflow.ellipsis,
          style: const TextStyle(
              fontSize: 22, fontWeight: FontWeight.bold, color: Colors.white),
        ),
        const SizedBox(height: 8),
        Text(
          _deviceState.trackArtist.isNotEmpty
              ? _deviceState.trackArtist
              : 'ID3 Tag Streaming Live',
          textAlign: TextAlign.center,
          style: const TextStyle(fontSize: 14, color: Colors.white70),
        ),
      ],
    );
  }

  Widget _buildBottomControls() {
    final eqNames = ['Flat', 'Rock', 'Pop', 'Classical', 'Jazz', 'Bass'];
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: const Color(0xFF141927),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.white.withValues(alpha: 0.08)),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: [
          _buildControlButton(
            icon: Icons.equalizer_rounded,
            label: eqNames[_deviceState.eqPreset],
            onTap: _showEQPresets,
          ),
          Container(width: 1, height: 32, color: Colors.white12),
          _buildControlButton(
            icon: Icons.swap_horiz_rounded,
            label: 'Cycle Source',
            onTap: _showSourceSelector,
          ),
        ],
      ),
    );
  }

  Widget _buildControlButton(
      {required IconData icon,
      required String label,
      required VoidCallback onTap}) {
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(10),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
        child: Row(
          children: [
            Icon(icon, size: 20, color: CroenixColors.primaryCyan),
            const SizedBox(width: 8),
            Text(label,
                style: const TextStyle(
                    fontSize: 13,
                    color: Colors.white,
                    fontWeight: FontWeight.bold)),
          ],
        ),
      ),
    );
  }

  void _showEQPresets() {
    showModalBottomSheet(
      context: context,
      backgroundColor: const Color(0xFF141927),
      shape: const RoundedRectangleBorder(
          borderRadius: BorderRadius.vertical(top: Radius.circular(20))),
      builder: (ctx) => Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text('DSP Equalizer Profiles',
                style: TextStyle(
                    color: Colors.white,
                    fontWeight: FontWeight.bold,
                    fontSize: 16)),
            const SizedBox(height: 12),
            ...['Flat', 'Rock', 'Pop', 'Classical', 'Jazz', 'Bass Boost']
                .asMap()
                .entries
                .map((entry) {
              return ListTile(
                title: Text(entry.value,
                    style: const TextStyle(color: Colors.white)),
                trailing: _deviceState.eqPreset == entry.key
                    ? const Icon(Icons.check, color: CroenixColors.primaryCyan)
                    : null,
                onTap: () {
                  _wsService.setEQPreset(entry.key);
                  Navigator.pop(ctx);
                },
              );
            }),
          ],
        ),
      ),
    );
  }

  void _showSourceSelector() {
    showModalBottomSheet(
      context: context,
      backgroundColor: const Color(0xFF141927),
      shape: const RoundedRectangleBorder(
          borderRadius: BorderRadius.vertical(top: Radius.circular(20))),
      builder: (ctx) => Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text('Switch Input Mode',
                style: TextStyle(
                    color: Colors.white,
                    fontWeight: FontWeight.bold,
                    fontSize: 16)),
            const SizedBox(height: 12),
            ListTile(
              leading: const Icon(Icons.bluetooth_rounded,
                  color: CroenixColors.sourceBluetooth),
              title: const Text('Bluetooth Audio (A2DP)',
                  style: TextStyle(color: Colors.white)),
              onTap: () {
                _wsService.setSource(AudioSource.bluetooth);
                Navigator.pop(ctx);
              },
            ),
            ListTile(
              leading: const Icon(Icons.sd_card_rounded,
                  color: CroenixColors.sourceSDCard),
              title: const Text('MicroSD Card Playback',
                  style: TextStyle(color: Colors.white)),
              onTap: () {
                _wsService.setSource(AudioSource.sdCard);
                Navigator.pop(ctx);
              },
            ),
            ListTile(
              leading: const Icon(Icons.radio_rounded,
                  color: CroenixColors.sourceFM),
              title: const Text('HW-322 FM Radio',
                  style: TextStyle(color: Colors.white)),
              onTap: () {
                _wsService.setSource(AudioSource.fmRadio);
                Navigator.pop(ctx);
              },
            ),
            ListTile(
              leading: const Icon(Icons.language_rounded,
                  color: CroenixColors.sourceWebStream),
              title: const Text('Web FM Digital Stream',
                  style: TextStyle(color: Colors.white)),
              onTap: () {
                _wsService.setSource(AudioSource.webStream);
                Navigator.pop(ctx);
              },
            ),
          ],
        ),
      ),
    );
  }
}
