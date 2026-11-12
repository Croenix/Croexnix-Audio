import 'package:flutter/material.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/models/device_state.dart';
import '../../data/services/websocket_service.dart';

class SDCardScreen extends StatefulWidget {
  const SDCardScreen({super.key});

  @override
  State<SDCardScreen> createState() => _SDCardScreenState();
}

class _SDCardScreenState extends State<SDCardScreen> {
  final WebSocketService _ws = WebSocketService();
  final TextEditingController _searchController = TextEditingController();
  String _searchFilter = '';
  List<Map<String, dynamic>> _tracks = [];

  @override
  void initState() {
    super.initState();
    _tracks = List.from(_ws.lastTrackList);
    // Request fresh track list from ESP32
    if (_ws.isConnected) {
      _ws.requestTrackList();
    }
  }

  @override
  void dispose() {
    _searchController.dispose();
    super.dispose();
  }

  Color _getFormatColor(String filename) {
    final lower = filename.toLowerCase();
    if (lower.endsWith('.mp3')) return const Color(0xFF00D9FF);
    if (lower.endsWith('.flac')) return const Color(0xFF10B981);
    if (lower.endsWith('.wav')) return const Color(0xFFF59E0B);
    if (lower.endsWith('.aac') || lower.endsWith('.m4a')) return const Color(0xFFEC4899);
    return Colors.white70;
  }

  String _getFormatBadge(String filename) {
    final dot = filename.lastIndexOf('.');
    if (dot != -1 && dot < filename.length - 1) {
      return filename.substring(dot + 1).toUpperCase();
    }
    return 'AUDIO';
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
            Color(0xFF081C1B),
            Color(0xFF0F1523),
          ],
        ),
      ),
      child: SafeArea(
        child: StreamBuilder<DeviceState>(
          stream: _ws.stateStream,
          initialData: _ws.currentState,
          builder: (context, stateSnap) {
            final state = stateSnap.data ?? _ws.currentState;

            return StreamBuilder<List<Map<String, dynamic>>>(
              stream: _ws.trackListStream,
              initialData: _tracks,
              builder: (context, trackSnap) {
                if (trackSnap.hasData && trackSnap.data != null) {
                  _tracks = trackSnap.data!;
                }

                final filteredTracks = _tracks.where((t) {
                  final title = (t['title'] ?? t['name'] ?? '').toString().toLowerCase();
                  final artist = (t['artist'] ?? '').toString().toLowerCase();
                  return title.contains(_searchFilter) || artist.contains(_searchFilter);
                }).toList();

                return Column(
                  children: [
                    _buildHeader(state),
                    _buildSearchBar(),
                    _buildStorageInfoCard(state),
                    Expanded(
                      child: filteredTracks.isEmpty
                          ? _buildEmptyState(state)
                          : ListView.builder(
                              physics: const BouncingScrollPhysics(),
                              padding: const EdgeInsets.only(
                                left: 16,
                                right: 16,
                                top: 8,
                                bottom: 100, // Clearance for bottom navigation
                              ),
                              itemCount: filteredTracks.length,
                              itemBuilder: (context, index) {
                                final track = filteredTracks[index];
                                final trackIndex = track['index'] as int? ?? index;
                                final title = track['title'] ?? track['name'] ?? 'Track ${trackIndex + 1}';
                                final artist = track['artist'] ?? 'SD Card Audio';
                                final filename = track['filename'] ?? track['name'] ?? '$title.mp3';
                                final isCurrent = state.currentSource == AudioSource.sdCard &&
                                    state.trackTitle.toLowerCase() == title.toString().toLowerCase();

                                return _buildTrackTile(
                                  index: trackIndex,
                                  title: title.toString(),
                                  artist: artist.toString(),
                                  filename: filename.toString(),
                                  isCurrent: isCurrent,
                                  isPlaying: isCurrent && state.isPlaying,
                                );
                              },
                            ),
                    ),
                  ],
                );
              },
            );
          },
        ),
      ),
    );
  }

  Widget _buildHeader(DeviceState state) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(20, 16, 20, 8),
      child: Row(
        children: [
          Container(
            padding: const EdgeInsets.all(10),
            decoration: BoxDecoration(
              color: CroenixColors.sourceSDCard.withOpacity(0.15),
              borderRadius: BorderRadius.circular(12),
              border: Border.all(color: CroenixColors.sourceSDCard.withOpacity(0.3)),
            ),
            child: const Icon(
              Icons.sd_storage_rounded,
              color: CroenixColors.sourceSDCard,
              size: 26,
            ),
          ),
          const SizedBox(width: 14),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text(
                  'SD CARD EXPLORER',
                  style: TextStyle(
                    fontSize: 20,
                    fontWeight: FontWeight.bold,
                    letterSpacing: 1.1,
                    color: Colors.white,
                  ),
                ),
                Text(
                  state.sdCardMounted
                      ? '${_tracks.length} tracks detected • FAT32 Ready'
                      : 'SD Module: GPIO 15 (CS), 19, 23, 18',
                  style: TextStyle(
                    fontSize: 12,
                    color: state.sdCardMounted ? CroenixColors.sourceSDCard : Colors.white54,
                  ),
                ),
              ],
            ),
          ),
          IconButton(
            icon: const Icon(Icons.sync_rounded, color: CroenixColors.primaryCyan),
            tooltip: 'Rescan SD Card',
            onPressed: () {
              _ws.requestTrackList();
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(
                  content: Text('Refreshing track list from ESP32 SD card...'),
                  duration: Duration(milliseconds: 1200),
                ),
              );
            },
          ),
        ],
      ),
    );
  }

  Widget _buildSearchBar() {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Container(
        decoration: GlassEffect.standard,
        child: TextField(
          controller: _searchController,
          onChanged: (val) => setState(() => _searchFilter = val.trim().toLowerCase()),
          style: const TextStyle(color: Colors.white, fontSize: 14),
          decoration: InputDecoration(
            hintText: 'Search songs, artists, or filenames...',
            hintStyle: const TextStyle(color: Colors.white38, fontSize: 14),
            prefixIcon: const Icon(Icons.search_rounded, color: CroenixColors.primaryCyan, size: 20),
            suffixIcon: _searchFilter.isNotEmpty
                ? IconButton(
                    icon: const Icon(Icons.clear_rounded, color: Colors.white38, size: 18),
                    onPressed: () {
                      _searchController.clear();
                      setState(() => _searchFilter = '');
                    },
                  )
                : null,
            border: InputBorder.none,
            contentPadding: const EdgeInsets.symmetric(vertical: 14),
          ),
        ),
      ),
    );
  }

  Widget _buildStorageInfoCard(DeviceState state) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
        decoration: BoxDecoration(
          color: Colors.white.withOpacity(0.04),
          borderRadius: BorderRadius.circular(14),
          border: Border.all(color: Colors.white.withOpacity(0.08)),
        ),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Row(
              children: [
                Icon(
                  state.sdCardMounted ? Icons.check_circle_rounded : Icons.info_outline_rounded,
                  color: state.sdCardMounted ? CroenixColors.success : Colors.amber,
                  size: 16,
                ),
                const SizedBox(width: 8),
                Text(
                  state.sdCardMounted ? 'Status: Mounted' : 'Waiting for SD Insert',
                  style: const TextStyle(color: Colors.white70, fontSize: 13, fontWeight: FontWeight.w500),
                ),
              ],
            ),
            Text(
              'Tracks: ${_tracks.length}',
              style: const TextStyle(color: CroenixColors.primaryCyan, fontSize: 13, fontWeight: FontWeight.bold),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildTrackTile({
    required int index,
    required String title,
    required String artist,
    required String filename,
    required bool isCurrent,
    required bool isPlaying,
  }) {
    final formatBadge = _getFormatBadge(filename);
    final badgeColor = _getFormatColor(filename);

    return Container(
      margin: const EdgeInsets.only(bottom: 8),
      decoration: BoxDecoration(
        color: isCurrent ? CroenixColors.primaryCyan.withOpacity(0.12) : Colors.white.withOpacity(0.04),
        borderRadius: BorderRadius.circular(14),
        border: Border.all(
          color: isCurrent ? CroenixColors.primaryCyan.withOpacity(0.5) : Colors.white.withOpacity(0.07),
          width: isCurrent ? 1.5 : 1,
        ),
      ),
      child: ListTile(
        contentPadding: const EdgeInsets.symmetric(horizontal: 14, vertical: 4),
        leading: Container(
          width: 44,
          height: 44,
          decoration: BoxDecoration(
            color: isCurrent ? CroenixColors.primaryCyan.withOpacity(0.2) : Colors.white.withOpacity(0.05),
            borderRadius: BorderRadius.circular(10),
          ),
          child: Center(
            child: isPlaying
                ? const Icon(Icons.equalizer_rounded, color: CroenixColors.primaryCyan, size: 22)
                : Text(
                    '${index + 1}',
                    style: TextStyle(
                      color: isCurrent ? CroenixColors.primaryCyan : Colors.white70,
                      fontWeight: FontWeight.bold,
                      fontSize: 14,
                    ),
                  ),
          ),
        ),
        title: Text(
          title,
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
          style: TextStyle(
            color: isCurrent ? CroenixColors.primaryCyan : Colors.white,
            fontWeight: isCurrent ? FontWeight.bold : FontWeight.w600,
            fontSize: 14,
          ),
        ),
        subtitle: Row(
          children: [
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 5, vertical: 1.5),
              decoration: BoxDecoration(
                color: badgeColor.withOpacity(0.15),
                borderRadius: BorderRadius.circular(4),
                border: Border.all(color: badgeColor.withOpacity(0.4), width: 0.8),
              ),
              child: Text(
                formatBadge,
                style: TextStyle(
                  color: badgeColor,
                  fontSize: 9,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ),
            const SizedBox(width: 8),
            Expanded(
              child: Text(
                artist,
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
                style: const TextStyle(color: Colors.white54, fontSize: 12),
              ),
            ),
          ],
        ),
        trailing: IconButton(
          icon: Icon(
            isPlaying ? Icons.pause_circle_filled_rounded : Icons.play_circle_fill_rounded,
            color: isCurrent ? CroenixColors.primaryCyan : Colors.white70,
            size: 32,
          ),
          onPressed: () {
            if (isCurrent && isPlaying) {
              _ws.pause();
            } else {
              _ws.setSource(AudioSource.sdCard);
              _ws.playSDIndex(index);
            }
          },
        ),
        onTap: () {
          _ws.setSource(AudioSource.sdCard);
          _ws.playSDIndex(index);
        },
      ),
    );
  }

  Widget _buildEmptyState(DeviceState state) {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              state.sdCardMounted ? Icons.queue_music_rounded : Icons.sd_card_alert_rounded,
              size: 64,
              color: Colors.white24,
            ),
            const SizedBox(height: 16),
            Text(
              state.sdCardMounted ? 'No audio files found' : 'SD Card Not Detected',
              style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Colors.white70),
            ),
            const SizedBox(height: 8),
            Text(
              state.sdCardMounted
                  ? 'Put .mp3, .flac, or .wav songs on the FAT32 SD card'
                  : 'Check SPI wiring: CS->15, DO->19, DI->23, CLK->18',
              textAlign: TextAlign.center,
              style: const TextStyle(fontSize: 13, color: Colors.white38),
            ),
            const SizedBox(height: 20),
            ElevatedButton.icon(
              onPressed: () => _ws.requestTrackList(),
              icon: const Icon(Icons.refresh_rounded, size: 18),
              label: const Text('Scan Tracks Now'),
              style: ElevatedButton.styleFrom(
                backgroundColor: CroenixColors.sourceSDCard,
                foregroundColor: Colors.black,
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
