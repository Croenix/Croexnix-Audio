import 'package:flutter/material.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/services/websocket_service.dart';
import '../../data/models/device_state.dart';

class SoundCloudScreen extends StatefulWidget {
  const SoundCloudScreen({super.key});

  @override
  State<SoundCloudScreen> createState() => _SoundCloudScreenState();
}

class _SoundCloudScreenState extends State<SoundCloudScreen>
    with SingleTickerProviderStateMixin {
  final WebSocketService _wsService = WebSocketService();
  DeviceState _deviceState = const DeviceState();
  final TextEditingController _urlController = TextEditingController();
  final TextEditingController _customTitleController = TextEditingController();
  final TextEditingController _customArtistController = TextEditingController();

  late AnimationController _pulseController;

  // Curated SoundCloud & Hi-Fi Online Audio Streams
  final List<Map<String, dynamic>> _curatedSoundCloudPlaylist = const [
    {
      'title': 'Lofi Hip Hop Radio - Beats to Relax/Study',
      'artist': 'Lofi Girl / SoundCloud Stream',
      'album': 'SoundCloud Chillhop Sessions',
      'genre': 'Lo-Fi / Chill',
      'url': 'http://stream.zeno.fm/f3wvbbqmdg8uv',
      'duration': 3600,
      'color': Color(0xFFFF5500),
    },
    {
      'title': 'Synthwave & Retrowave Cyber Mix',
      'artist': 'Nightride FM / SoundCloud Mix',
      'album': 'Neon Nights Playlist',
      'genre': 'Synthwave',
      'url': 'https://stream.nightride.fm/nightride.mp3',
      'duration': 3600,
      'color': Color(0xFF9900FF),
    },
    {
      'title': 'Deep House & Electronic Sunset',
      'artist': 'SoundCloud Electronic Lounge',
      'album': 'Ibiza Deep Masters',
      'genre': 'Deep House',
      'url': 'http://stream.zeno.fm/c234a7szh18uv',
      'duration': 3600,
      'color': Color(0xFF00D9FF),
    },
    {
      'title': 'ChilledCow Ambient Dreams',
      'artist': 'SoundCloud Ambient Collective',
      'album': 'Floating Horizons',
      'genre': 'Ambient / Focus',
      'url': 'http://stream.zeno.fm/0r0xa7szh18uv',
      'duration': 3600,
      'color': Color(0xFF10B981),
    },
    {
      'title': 'Acoustic Coffee Shop Sessions',
      'artist': 'Indie Folk / SoundCloud Artists',
      'album': 'Unplugged Live',
      'genre': 'Acoustic Folk',
      'url': 'http://stream.zeno.fm/7x48b6k1v38uv',
      'duration': 3600,
      'color': Color(0xFFF59E0B),
    },
    {
      'title': 'Cyberpunk 2077 Dark Electro Beats',
      'artist': 'Darkwave Labs',
      'album': 'Night City Audio Drive',
      'genre': 'Cyberpunk',
      'url': 'https://stream.nightride.fm/darksynth.mp3',
      'duration': 3600,
      'color': Color(0xFFEC4899),
    },
  ];

  @override
  void initState() {
    super.initState();
    _deviceState = _wsService.currentState;
    _pulseController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 2),
    )..repeat(reverse: true);

    _wsService.stateStream.listen((state) {
      if (mounted) setState(() => _deviceState = state);
    });
  }

  @override
  void dispose() {
    _pulseController.dispose();
    _urlController.dispose();
    _customTitleController.dispose();
    _customArtistController.dispose();
    super.dispose();
  }

  void _playAllSongs() {
    _wsService.playSoundCloudPlaylist(_curatedSoundCloudPlaylist,
        startIndex: 0);
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        backgroundColor: CroenixColors.soundCloudOrange,
        content: Row(
          children: [
            Icon(Icons.playlist_play_rounded, color: Colors.white),
            SizedBox(width: 10),
            Text('Streaming Full SoundCloud Playlist to ESP32 TFT!'),
          ],
        ),
        duration: Duration(seconds: 3),
      ),
    );
  }

  void _showCustomStreamDialog() {
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        backgroundColor: const Color(0xFF141927),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(20),
          side: const BorderSide(
              color: CroenixColors.soundCloudOrange, width: 1.5),
        ),
        title: const Row(
          children: [
            Icon(Icons.cloud_upload_rounded,
                color: CroenixColors.soundCloudOrange),
            SizedBox(width: 10),
            Text('Custom SoundCloud / Stream',
                style: TextStyle(color: Colors.white, fontSize: 16)),
          ],
        ),
        content: SingleChildScrollView(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: _urlController,
                style: const TextStyle(color: Colors.white),
                decoration: InputDecoration(
                  labelText: 'Stream / MP3 URL',
                  labelStyle: const TextStyle(color: Colors.white70),
                  hintText: 'https://.../stream.mp3',
                  hintStyle: const TextStyle(color: Colors.white30),
                  filled: true,
                  fillColor: const Color(0xFF1D2436),
                  border: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(12)),
                ),
              ),
              const SizedBox(height: 12),
              TextField(
                controller: _customTitleController,
                style: const TextStyle(color: Colors.white),
                decoration: InputDecoration(
                  labelText: 'Track Title (ID3)',
                  labelStyle: const TextStyle(color: Colors.white70),
                  hintText: 'My Favorite SoundCloud Song',
                  hintStyle: const TextStyle(color: Colors.white30),
                  filled: true,
                  fillColor: const Color(0xFF1D2436),
                  border: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(12)),
                ),
              ),
              const SizedBox(height: 12),
              TextField(
                controller: _customArtistController,
                style: const TextStyle(color: Colors.white),
                decoration: InputDecoration(
                  labelText: 'Artist / Channel (ID3)',
                  labelStyle: const TextStyle(color: Colors.white70),
                  hintText: 'SoundCloud Creator',
                  hintStyle: const TextStyle(color: Colors.white30),
                  filled: true,
                  fillColor: const Color(0xFF1D2436),
                  border: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(12)),
                ),
              ),
            ],
          ),
        ),
        actions: [
          TextButton(
            child:
                const Text('Cancel', style: TextStyle(color: Colors.white60)),
            onPressed: () => Navigator.pop(ctx),
          ),
          ElevatedButton.icon(
            style: ElevatedButton.styleFrom(
              backgroundColor: CroenixColors.soundCloudOrange,
              foregroundColor: Colors.white,
              shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(12)),
            ),
            icon: const Icon(Icons.play_arrow_rounded),
            label: const Text('Stream Now'),
            onPressed: () {
              final url = _urlController.text.trim();
              if (url.isNotEmpty) {
                final title = _customTitleController.text.trim().isNotEmpty
                    ? _customTitleController.text.trim()
                    : 'SoundCloud Custom Stream';
                final artist = _customArtistController.text.trim().isNotEmpty
                    ? _customArtistController.text.trim()
                    : 'SoundCloud Artist';

                _wsService.playSoundCloudTrack(
                  url: url,
                  title: title,
                  artist: artist,
                  album: 'SoundCloud Custom Playlist',
                );
                Navigator.pop(ctx);
              }
            },
          ),
        ],
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
            Color(0xFF160D1A),
            CroenixColors.darkBackground,
          ],
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
              centerTitle: false,
              title: Row(
                children: [
                  Container(
                    padding: const EdgeInsets.all(8),
                    decoration: BoxDecoration(
                      gradient: const LinearGradient(
                        colors: [
                          CroenixColors.soundCloudOrange,
                          Color(0xFFFF8800)
                        ],
                      ),
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: const Icon(Icons.cloud_rounded,
                        color: Colors.white, size: 20),
                  ),
                  const SizedBox(width: 12),
                  const Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'SOUNDCLOUD',
                        style: TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.bold,
                          letterSpacing: 1.5,
                          color: Colors.white,
                        ),
                      ),
                      Text(
                        'Online Playlist & ID3 Stream Engine',
                        style: TextStyle(fontSize: 11, color: Colors.white70),
                      ),
                    ],
                  ),
                ],
              ),
              actions: [
                IconButton(
                  icon: const Icon(Icons.add_link_rounded,
                      color: CroenixColors.soundCloudOrange),
                  tooltip: 'Add Custom Stream URL',
                  onPressed: _showCustomStreamDialog,
                ),
              ],
            ),

            SliverPadding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              sliver: SliverList(
                delegate: SliverChildListDelegate([
                  const SizedBox(height: 12),

                  // Hero Action Card: "Play All Songs"
                  _buildPlayAllHeroCard(),

                  const SizedBox(height: 20),

                  // Real-Time Now-Playing ID3 Metadata Card
                  _buildLiveID3Card(),

                  const SizedBox(height: 24),

                  // Section Header
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      const Text(
                        'SOUNDCLOUD PLAYLISTS & CHANNELS',
                        style: TextStyle(
                          fontSize: 12,
                          fontWeight: FontWeight.bold,
                          letterSpacing: 1.2,
                          color: Colors.white70,
                        ),
                      ),
                      Text(
                        '${_curatedSoundCloudPlaylist.length} Channels',
                        style: const TextStyle(
                            fontSize: 11,
                            color: CroenixColors.soundCloudOrange),
                      ),
                    ],
                  ),
                  const SizedBox(height: 12),

                  // Playlist Tracks
                  ..._curatedSoundCloudPlaylist.asMap().entries.map((entry) {
                    final idx = entry.key;
                    final track = entry.value;
                    final isCurrent = _deviceState.trackTitle == track['title'];
                    return _buildTrackTile(track, idx, isCurrent);
                  }),

                  const SizedBox(height: 80),
                ]),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildPlayAllHeroCard() {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          colors: [
            CroenixColors.soundCloudOrange.withValues(alpha: 0.25),
            const Color(0xFF1B1B2F),
          ],
        ),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(
            color: CroenixColors.soundCloudOrange.withValues(alpha: 0.4),
            width: 1.5),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                padding:
                    const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                decoration: BoxDecoration(
                  color: CroenixColors.soundCloudOrange,
                  borderRadius: BorderRadius.circular(12),
                ),
                child: const Text(
                  'FEATURED PLAYLIST',
                  style: TextStyle(
                      fontSize: 10,
                      fontWeight: FontWeight.bold,
                      color: Colors.white),
                ),
              ),
              const Spacer(),
              const Icon(Icons.graphic_eq_rounded,
                  color: CroenixColors.soundCloudOrange, size: 20),
            ],
          ),
          const SizedBox(height: 12),
          const Text(
            'SoundCloud Ultimate Hi-Fi Mix',
            style: TextStyle(
                fontSize: 20, fontWeight: FontWeight.bold, color: Colors.white),
          ),
          const SizedBox(height: 4),
          const Text(
            'High-definition continuous internet audio stream decoded by ESP32 I2S DAC with synchronized TFT ID3 display.',
            style: TextStyle(fontSize: 12, color: Colors.white70),
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: ElevatedButton.icon(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: CroenixColors.soundCloudOrange,
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.symmetric(vertical: 14),
                    shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(14)),
                  ),
                  icon: const Icon(Icons.playlist_play_rounded, size: 26),
                  label: const Text(
                    'PLAY ALL SONGS',
                    style: TextStyle(
                        fontSize: 14,
                        fontWeight: FontWeight.bold,
                        letterSpacing: 1),
                  ),
                  onPressed: _playAllSongs,
                ),
              ),
              const SizedBox(width: 12),
              OutlinedButton.icon(
                style: OutlinedButton.styleFrom(
                  foregroundColor: Colors.white,
                  side: const BorderSide(color: Colors.white24),
                  padding:
                      const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
                  shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(14)),
                ),
                icon: const Icon(Icons.shuffle_rounded, size: 20),
                label: const Text('Shuffle'),
                onPressed: () {
                  final shuffled = List<Map<String, dynamic>>.from(
                      _curatedSoundCloudPlaylist)
                    ..shuffle();
                  _wsService.playSoundCloudPlaylist(shuffled);
                },
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildLiveID3Card() {
    final isStreaming = _deviceState.currentSource == AudioSource.webStream &&
        _deviceState.isPlaying;

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: const Color(0xFF141927),
        borderRadius: BorderRadius.circular(18),
        border: Border.all(
          color: isStreaming ? CroenixColors.soundCloudOrange : Colors.white12,
          width: 1.5,
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                width: 8,
                height: 8,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  color: isStreaming
                      ? CroenixColors.soundCloudOrange
                      : Colors.grey,
                ),
              ),
              const SizedBox(width: 8),
              Text(
                isStreaming
                    ? 'STREAMING LIVE TO TFT & I2S DAC'
                    : 'STANDBY (NO STREAM LOADED)',
                style: TextStyle(
                  fontSize: 10,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 1,
                  color: isStreaming
                      ? CroenixColors.soundCloudOrange
                      : Colors.white38,
                ),
              ),
              const Spacer(),
              if (isStreaming)
                Container(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                  decoration: BoxDecoration(
                    color:
                        CroenixColors.soundCloudOrange.withValues(alpha: 0.2),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: const Text('44.1kHz • 16-bit',
                      style: TextStyle(
                          fontSize: 10, color: CroenixColors.soundCloudOrange)),
                ),
            ],
          ),
          const SizedBox(height: 12),
          Text(
            _deviceState.trackTitle.isNotEmpty
                ? _deviceState.trackTitle
                : 'Select a SoundCloud song or click "PLAY ALL"',
            maxLines: 2,
            overflow: TextOverflow.ellipsis,
            style: const TextStyle(
                fontSize: 16, fontWeight: FontWeight.bold, color: Colors.white),
          ),
          const SizedBox(height: 4),
          Text(
            _deviceState.trackArtist.isNotEmpty
                ? '${_deviceState.trackArtist} • ${_deviceState.trackAlbum}'
                : 'SoundCloud Audio Pipeline',
            style: const TextStyle(fontSize: 13, color: Colors.white70),
          ),
          const SizedBox(height: 14),
          // Playback Controls Row
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              IconButton(
                icon: const Icon(Icons.skip_previous_rounded,
                    color: Colors.white),
                onPressed: () => _wsService.prevSoundCloudTrack(),
              ),
              FloatingActionButton.small(
                backgroundColor: CroenixColors.soundCloudOrange,
                foregroundColor: Colors.white,
                child: Icon(_deviceState.isPlaying
                    ? Icons.pause_rounded
                    : Icons.play_arrow_rounded),
                onPressed: () {
                  if (_deviceState.isPlaying) {
                    _wsService.pause();
                  } else {
                    _wsService.play();
                  }
                },
              ),
              IconButton(
                icon: const Icon(Icons.skip_next_rounded, color: Colors.white),
                onPressed: () => _wsService.nextSoundCloudTrack(),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildTrackTile(
      Map<String, dynamic> track, int index, bool isCurrent) {
    final trackColor =
        track['color'] as Color? ?? CroenixColors.soundCloudOrange;

    return Container(
      margin: const EdgeInsets.only(bottom: 10),
      decoration: BoxDecoration(
        color: isCurrent
            ? trackColor.withValues(alpha: 0.15)
            : const Color(0xFF131726),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: isCurrent ? trackColor : Colors.white.withValues(alpha: 0.06),
          width: 1.2,
        ),
      ),
      child: ListTile(
        contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
        leading: Container(
          width: 44,
          height: 44,
          decoration: BoxDecoration(
            color: trackColor.withValues(alpha: 0.2),
            borderRadius: BorderRadius.circular(12),
          ),
          child: Center(
            child: isCurrent
                ? const Icon(Icons.graphic_eq_rounded,
                    color: Colors.white, size: 24)
                : Text(
                    '#${index + 1}',
                    style: TextStyle(
                      color: trackColor,
                      fontWeight: FontWeight.bold,
                      fontSize: 13,
                    ),
                  ),
          ),
        ),
        title: Text(
          track['title'] as String,
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
          style: TextStyle(
            color: isCurrent ? trackColor : Colors.white,
            fontWeight: FontWeight.bold,
            fontSize: 14,
          ),
        ),
        subtitle: Text(
          '${track["artist"]} • ${track["genre"]}',
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
          style: const TextStyle(color: Colors.white60, fontSize: 12),
        ),
        trailing: IconButton(
          icon: Icon(
            isCurrent && _deviceState.isPlaying
                ? Icons.pause_circle_filled_rounded
                : Icons.play_circle_fill_rounded,
            color: trackColor,
            size: 32,
          ),
          onPressed: () {
            _wsService.playSoundCloudTrack(
              url: track['url'] as String,
              title: track['title'] as String,
              artist: track['artist'] as String,
              album: track['album'] as String,
            );
          },
        ),
      ),
    );
  }
}
