class SdFile {
  final String name;
  final String path;
  final int size; // in bytes
  final String format; // mp3, wav, flac, etc.

  SdFile({
    required this.name,
    required this.path,
    required this.size,
    required this.format,
  });

  String get formattedSize {
    if (size <= 0) return '0 B';
    if (size < 1024) return '$size B';
    if (size < 1024 * 1024) return '${(size / 1024).toStringAsFixed(1)} KB';
    return '${(size / (1024 * 1024)).toStringAsFixed(1)} MB';
  }

  factory SdFile.fromJson(Map<String, dynamic> json) {
    final path = json['path']?.toString() ?? '';
    var name = json['name']?.toString() ?? '';
    if (name.isEmpty && path.isNotEmpty) {
      name = path.split('/').last;
    }

    String format = json['format']?.toString() ?? '';
    if (format.isEmpty && name.contains('.')) {
      format = name.split('.').last.toLowerCase();
    }

    return SdFile(
      name: name,
      path: path,
      size: (json['size'] is num) ? (json['size'] as num).toInt() : 0,
      format: format.toUpperCase(),
    );
  }
}

