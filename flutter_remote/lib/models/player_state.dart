class PlayerStateModel {
  final bool isPlaying;
  final bool isPaused;
  final String title;
  final String artist;
  final String album;
  final int currentTime; // seconds
  final int duration; // seconds
  final int volume; // 0 to 100
  final String currentFile;
  final int trackIndex;
  final int totalTracks;

  PlayerStateModel({
    this.isPlaying = false,
    this.isPaused = false,
    this.title = 'No Track Loaded',
    this.artist = 'ESP32 Audio Player',
    this.album = 'SD Card',
    this.currentTime = 0,
    this.duration = 0,
    this.volume = 75,
    this.currentFile = '',
    this.trackIndex = 0,
    this.totalTracks = 0,
  });

  int get remainingTime {
    if (duration <= 0) return 0;
    final rem = duration - currentTime;
    return rem >= 0 ? rem : 0;
  }

  String get formattedCurrentTime {
    return formatSeconds(currentTime);
  }

  String get formattedRemainingTime {
    if (duration <= 0) return '-0:00';
    return '-${formatSeconds(remainingTime)}';
  }

  String get formattedDuration {
    return formatSeconds(duration);
  }

  double get progressRatio {
    if (duration <= 0) return 0.0;
    final r = currentTime / duration.toDouble();
    return r.clamp(0.0, 1.0);
  }

  static String formatSeconds(int seconds) {
    if (seconds < 0) seconds = 0;
    final m = seconds ~/ 60;
    final s = seconds % 60;
    return '$m:${s.toString().padLeft(2, '0')}';
  }

  factory PlayerStateModel.fromJson(Map<String, dynamic> json) {
    return PlayerStateModel(
      isPlaying: json['isPlaying'] == true,
      isPaused: json['isPaused'] == true,
      title: (json['title'] != null && json['title'].toString().isNotEmpty)
          ? json['title'].toString()
          : (json['currentFile'] != null
                ? _extractFilename(json['currentFile'].toString())
                : 'Unknown Title'),
      artist: (json['artist'] != null && json['artist'].toString().isNotEmpty)
          ? json['artist'].toString()
          : 'Unknown Artist',
      album: (json['album'] != null && json['album'].toString().isNotEmpty)
          ? json['album'].toString()
          : 'SD Card Audio',
      currentTime: (json['currentTime'] is num)
          ? (json['currentTime'] as num).toInt()
          : 0,
      duration: (json['duration'] is num)
          ? (json['duration'] as num).toInt()
          : 0,
      volume: (json['volume'] is num) ? (json['volume'] as num).toInt() : 75,
      currentFile: json['currentFile']?.toString() ?? '',
      trackIndex: (json['trackIndex'] is num)
          ? (json['trackIndex'] as num).toInt()
          : 0,
      totalTracks: (json['totalTracks'] is num)
          ? (json['totalTracks'] as num).toInt()
          : 0,
    );
  }

  static String _extractFilename(String path) {
    if (path.isEmpty) return 'No Track';
    final parts = path.split('/');
    var name = parts.last;
    if (name.contains('.')) {
      name = name.substring(0, name.lastIndexOf('.'));
    }
    return name.replaceAll('_', ' ');
  }

  PlayerStateModel copyWith({
    bool? isPlaying,
    bool? isPaused,
    String? title,
    String? artist,
    String? album,
    int? currentTime,
    int? duration,
    int? volume,
    String? currentFile,
    int? trackIndex,
    int? totalTracks,
  }) {
    return PlayerStateModel(
      isPlaying: isPlaying ?? this.isPlaying,
      isPaused: isPaused ?? this.isPaused,
      title: title ?? this.title,
      artist: artist ?? this.artist,
      album: album ?? this.album,
      currentTime: currentTime ?? this.currentTime,
      duration: duration ?? this.duration,
      volume: volume ?? this.volume,
      currentFile: currentFile ?? this.currentFile,
      trackIndex: trackIndex ?? this.trackIndex,
      totalTracks: totalTracks ?? this.totalTracks,
    );
  }
}
