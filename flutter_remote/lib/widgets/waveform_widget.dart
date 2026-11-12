import 'dart:math';
import 'package:flutter/material.dart';

class WaveformWidget extends StatefulWidget {
  final bool isPlaying;
  final int barCount;
  final Color barColor;
  final double maxHeight;

  const WaveformWidget({
    super.key,
    required this.isPlaying,
    this.barCount = 28,
    this.barColor = const Color(0xFFD6C8F0),
    this.maxHeight = 36.0,
  });

  @override
  State<WaveformWidget> createState() => _WaveformWidgetState();
}

class _WaveformWidgetState extends State<WaveformWidget> with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late List<double> _baseHeights;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 1000),
    )..addListener(() {
        if (mounted) setState(() {});
      });

    // Gaussian bell shape curve for standard waveform looks
    _baseHeights = List.generate(widget.barCount, (i) {
      final x = (i - widget.barCount / 2) / (widget.barCount / 4);
      return exp(-0.5 * x * x);
    });

    if (widget.isPlaying) {
      _controller.repeat();
    }
  }

  @override
  void didUpdateWidget(covariant WaveformWidget oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.isPlaying != oldWidget.isPlaying) {
      if (widget.isPlaying) {
        _controller.repeat();
      } else {
        _controller.stop();
      }
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      size: Size(widget.barCount * 3.8, widget.maxHeight),
      painter: _WaveformPainter(
        progress: _controller.value,
        isPlaying: widget.isPlaying,
        barCount: widget.barCount,
        baseHeights: _baseHeights,
        color: widget.barColor,
        maxHeight: widget.maxHeight,
      ),
    );
  }
}

class _WaveformPainter extends CustomPainter {
  final double progress;
  final bool isPlaying;
  final int barCount;
  final List<double> baseHeights;
  final Color color;
  final double maxHeight;

  _WaveformPainter({
    required this.progress,
    required this.isPlaying,
    required this.barCount,
    required this.baseHeights,
    required this.color,
    required this.maxHeight,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = color
      ..strokeCap = StrokeCap.round
      ..strokeWidth = 1.6;

    final barSpacing = size.width / barCount;
    final centerY = size.height / 2;

    for (int i = 0; i < barCount; i++) {
      double amplitude = baseHeights[i];
      if (isPlaying) {
        final phase = progress * 2 * pi + (i * 0.45);
        final dynamicFactor = 0.65 + 0.35 * sin(phase) * cos(phase * 0.7);
        amplitude *= dynamicFactor;
      } else {
        amplitude *= 0.4;
      }

      final barH = max(3.0, amplitude * maxHeight);
      final x = i * barSpacing + (barSpacing / 2);
      final top = centerY - (barH / 2);
      final bottom = centerY + (barH / 2);

      canvas.drawLine(Offset(x, top), Offset(x, bottom), paint);
    }
  }

  @override
  bool shouldRepaint(covariant _WaveformPainter oldDelegate) {
    return isPlaying || oldDelegate.isPlaying != isPlaying;
  }
}
