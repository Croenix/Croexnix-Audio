import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/sd_file.dart';
import '../services/esp32_service.dart';
import '../theme/app_theme.dart';

class SdFilesScreen extends StatefulWidget {
  final VoidCallback onBackToPlayer;

  const SdFilesScreen({
    super.key,
    required this.onBackToPlayer,
  });

  @override
  State<SdFilesScreen> createState() => _SdFilesScreenState();
}

class _SdFilesScreenState extends State<SdFilesScreen> {
  final TextEditingController _searchController = TextEditingController();
  String _searchQuery = '';

  @override
  void dispose() {
    _searchController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final espService = context.watch<Esp32Service>();
    final files = espService.sdFiles;
    final currentFile = espService.playerState.currentFile;
    final isLoading = espService.isLoadingFiles;

    final filteredFiles = files.where((file) {
      if (_searchQuery.isEmpty) return true;
      return file.name.toLowerCase().contains(_searchQuery.toLowerCase()) ||
          file.path.toLowerCase().contains(_searchQuery.toLowerCase());
    }).toList();

    return Scaffold(
      backgroundColor: AppTheme.background,
      body: SafeArea(
        child: Column(
          children: [
            // Header bar
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 16.0, vertical: 12.0),
              child: Row(
                children: [
                  IconButton(
                    icon: const Icon(Icons.arrow_back_ios_new, color: Colors.white, size: 20),
                    onPressed: widget.onBackToPlayer,
                  ),
                  const Expanded(
                    child: Text(
                      'SD Card Music Files',
                      style: TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                        color: Colors.white,
                      ),
                    ),
                  ),
                  IconButton(
                    icon: isLoading
                        ? const SizedBox(
                            width: 18,
                            height: 18,
                            child: CircularProgressIndicator(
                              strokeWidth: 2,
                              color: AppTheme.primaryNeon,
                            ),
                          )
                        : const Icon(Icons.refresh, color: AppTheme.primaryNeon),
                    tooltip: 'Rescan SD Card',
                    onPressed: isLoading ? null : () => espService.fetchSdFiles(),
                  ),
                ],
              ),
            ),

            // Search Bar
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 20.0, vertical: 8.0),
              child: TextField(
                controller: _searchController,
                onChanged: (val) {
                  setState(() {
                    _searchQuery = val.trim();
                  });
                },
                style: const TextStyle(color: Colors.white),
                decoration: InputDecoration(
                  hintText: 'Search songs on SD card...',
                  hintStyle: const TextStyle(color: AppTheme.textMuted),
                  prefixIcon: const Icon(Icons.search, color: AppTheme.textSecondary),
                  suffixIcon: _searchQuery.isNotEmpty
                      ? IconButton(
                          icon: const Icon(Icons.clear, color: AppTheme.textMuted),
                          onPressed: () {
                            _searchController.clear();
                            setState(() {
                              _searchQuery = '';
                            });
                          },
                        )
                      : null,
                  filled: true,
                  fillColor: AppTheme.surfaceHighlight.withOpacity(0.6),
                  contentPadding: const EdgeInsets.symmetric(vertical: 12),
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(16),
                    borderSide: BorderSide.none,
                  ),
                ),
              ),
            ),

            // Total file count subtitle
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 24.0, vertical: 4.0),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Text(
                    '${filteredFiles.length} ${filteredFiles.length == 1 ? 'track' : 'tracks'} found',
                    style: const TextStyle(
                      fontSize: 12,
                      color: AppTheme.textSecondary,
                    ),
                  ),
                  if (files.isNotEmpty)
                    Text(
                      'ESP32 Storage',
                      style: TextStyle(
                        fontSize: 12,
                        color: AppTheme.borderGlow.withOpacity(0.8),
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                ],
              ),
            ),

            const SizedBox(height: 8),

            // File List View
            Expanded(
              child: isLoading && files.isEmpty
                  ? const Center(
                      child: CircularProgressIndicator(color: AppTheme.primaryNeon),
                    )
                  : filteredFiles.isEmpty
                      ? Center(
                          child: Column(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              Icon(
                                Icons.music_off_rounded,
                                size: 56,
                                color: AppTheme.textMuted.withOpacity(0.6),
                              ),
                              const SizedBox(height: 12),
                              Text(
                                files.isEmpty
                                    ? 'No audio files detected on SD card'
                                    : 'No matches found',
                                style: const TextStyle(
                                  color: AppTheme.textSecondary,
                                  fontSize: 15,
                                ),
                              ),
                              const SizedBox(height: 16),
                              ElevatedButton.icon(
                                onPressed: () => espService.fetchSdFiles(),
                                icon: const Icon(Icons.sync, size: 18),
                                label: const Text('Scan SD Card'),
                                style: ElevatedButton.styleFrom(
                                  backgroundColor: AppTheme.surfaceHighlight,
                                  foregroundColor: AppTheme.primaryNeon,
                                  shape: RoundedRectangleBorder(
                                    borderRadius: BorderRadius.circular(12),
                                  ),
                                ),
                              ),
                            ],
                          ),
                        )
                      : ListView.separated(
                          physics: const BouncingScrollPhysics(),
                          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                          itemCount: filteredFiles.length,
                          separatorBuilder: (ctx, i) => const SizedBox(height: 8),
                          itemBuilder: (ctx, i) {
                            final file = filteredFiles[i];
                            final isPlayingThis = currentFile.isNotEmpty &&
                                (currentFile == file.path || currentFile.endsWith(file.name));

                            return _buildFileItem(
                              context,
                              file,
                              isPlayingThis,
                              espService,
                            );
                          },
                        ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildFileItem(
    BuildContext context,
    SdFile file,
    bool isPlayingThis,
    Esp32Service espService,
  ) {
    return Container(
      decoration: BoxDecoration(
        color: isPlayingThis
            ? AppTheme.primaryNeon.withOpacity(0.12)
            : AppTheme.surface.withOpacity(0.7),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: isPlayingThis
              ? AppTheme.primaryNeon
              : AppTheme.borderCard.withOpacity(0.2),
          width: isPlayingThis ? 1.5 : 1.0,
        ),
      ),
      child: ListTile(
        contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
        leading: Container(
          width: 44,
          height: 44,
          decoration: BoxDecoration(
            color: isPlayingThis
                ? AppTheme.primaryNeon
                : AppTheme.surfaceHighlight,
            borderRadius: BorderRadius.circular(12),
          ),
          child: Icon(
            isPlayingThis ? Icons.volume_up_rounded : Icons.music_note_rounded,
            color: isPlayingThis ? Colors.white : AppTheme.secondaryNeon,
            size: 22,
          ),
        ),
        title: Text(
          file.name,
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
          style: TextStyle(
            color: isPlayingThis ? Colors.white : AppTheme.textPrimary,
            fontWeight: isPlayingThis ? FontWeight.bold : FontWeight.w500,
            fontSize: 15,
          ),
        ),
        subtitle: Row(
          children: [
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
              decoration: BoxDecoration(
                color: AppTheme.surfaceHighlight,
                borderRadius: BorderRadius.circular(4),
              ),
              child: Text(
                file.format,
                style: const TextStyle(
                  fontSize: 10,
                  fontWeight: FontWeight.bold,
                  color: AppTheme.accentCyan,
                ),
              ),
            ),
            const SizedBox(width: 8),
            Text(
              file.formattedSize,
              style: const TextStyle(
                fontSize: 12,
                color: AppTheme.textMuted,
              ),
            ),
          ],
        ),
        trailing: IconButton(
          icon: Icon(
            isPlayingThis ? Icons.play_circle_fill : Icons.play_circle_outline,
            color: isPlayingThis ? AppTheme.primaryNeon : AppTheme.textSecondary,
            size: 32,
          ),
          tooltip: 'Play Song',
          onPressed: () {
            espService.playFile(file.path);
            widget.onBackToPlayer();
          },
        ),
        onTap: () {
          espService.playFile(file.path);
          widget.onBackToPlayer();
        },
      ),
    );
  }
}

