import 'package:flutter/material.dart';
import 'dart:math' as math;
import '../../../core/theme/croenix_colors.dart';

class AudioVisualizer extends StatelessWidget {
  final bool isPlaying;
  final AnimationController animationController;

  const AudioVisualizer({
    super.key,
    required this.isPlaying,
    required this.animationController,
  });

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Container(
        width: 280,
        height: 280,
        decoration: GlassEffect.elevated,
        child: ClipRRect(
          borderRadius: BorderRadius.circular(20),
          child: CustomPaint(
            painter: _VisualizerPainter(
              animation: animationController,
              isPlaying: isPlaying,
            ),
            child: Container(),
          ),
        ),
      ),
    );
  }
}

class _VisualizerPainter extends CustomPainter {
  final Animation<double> animation;
  final bool isPlaying;

  _VisualizerPainter({
    required this.animation,
    required this.isPlaying,
  }) : super(repaint: animation);

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final maxRadius = size.width / 2;

    // Draw background gradient
    final bgRect = Rect.fromLTWH(0, 0, size.width, size.height);
    const bgGradient = RadialGradient(
      colors: [
        CroenixColors.darkSurface,
        CroenixColors.darkBackground,
      ],
    );
    final bgPaint = Paint()..shader = bgGradient.createShader(bgRect);
    canvas.drawRect(bgRect, bgPaint);

    if (!isPlaying) {
      _drawStaticWaves(canvas, center, maxRadius);
      return;
    }

    // Draw animated waveform circles
    for (int i = 0; i < 5; i++) {
      final progress = (animation.value + (i * 0.2)) % 1.0;
      final radius = maxRadius * 0.3 + (maxRadius * 0.7 * progress);
      final opacity = 1.0 - progress;

      final paint = Paint()
        ..color = CroenixColors.primaryCyan.withValues(alpha: opacity * 0.3)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2;

      canvas.drawCircle(center, radius, paint);
    }

    // Draw spectrum bars
    const barCount = 40;
    const angleStep = (2 * math.pi) / barCount;
    final innerRadius = maxRadius * 0.4;
    final maxBarHeight = maxRadius * 0.35;

    for (int i = 0; i < barCount; i++) {
      final angle = i * angleStep;

      // Simulate random bar heights
      final seed = i + (animation.value * 100).toInt();
      final heightFactor = (math.sin(seed * 0.1) + 1) / 2;
      final barHeight =
          maxBarHeight * heightFactor * (0.5 + animation.value * 0.5);

      final startX = center.dx + math.cos(angle) * innerRadius;
      final startY = center.dy + math.sin(angle) * innerRadius;
      final endX = center.dx + math.cos(angle) * (innerRadius + barHeight);
      final endY = center.dy + math.sin(angle) * (innerRadius + barHeight);

      const gradient = LinearGradient(
        colors: [
          CroenixColors.spectrumLow,
          CroenixColors.spectrumMid,
          CroenixColors.spectrumHigh,
        ],
        begin: Alignment.bottomCenter,
        end: Alignment.topCenter,
      );

      final paint = Paint()
        ..shader = gradient.createShader(
          Rect.fromPoints(
            Offset(startX, startY),
            Offset(endX, endY),
          ),
        )
        ..strokeWidth = 3
        ..strokeCap = StrokeCap.round;

      canvas.drawLine(
        Offset(startX, startY),
        Offset(endX, endY),
        paint,
      );
    }

    // Draw center circle
    final centerPaint = Paint()
      ..color = CroenixColors.darkSurface
      ..style = PaintingStyle.fill;
    canvas.drawCircle(center, innerRadius - 5, centerPaint);

    // Draw center icon
    final iconPaint = Paint()
      ..color = CroenixColors.primaryCyan
      ..style = PaintingStyle.fill;

    _drawMusicIcon(canvas, center, iconPaint);
  }

  void _drawStaticWaves(Canvas canvas, Offset center, double maxRadius) {
    for (int i = 1; i <= 3; i++) {
      final radius = maxRadius * (0.3 + i * 0.15);
      final paint = Paint()
        ..color = CroenixColors.primaryCyan.withValues(alpha: 0.1)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1.5;

      canvas.drawCircle(center, radius, paint);
    }

    // Draw static center icon
    final iconPaint = Paint()
      ..color = CroenixColors.textSecondary
      ..style = PaintingStyle.fill;

    _drawMusicIcon(canvas, center, iconPaint);
  }

  void _drawMusicIcon(Canvas canvas, Offset center, Paint paint) {
    final path = Path();
    const iconSize = 40.0;

    // Draw music note shape
    path.addOval(Rect.fromCircle(
      center: Offset(center.dx - iconSize * 0.2, center.dy + iconSize * 0.2),
      radius: iconSize * 0.15,
    ));

    path.addRect(Rect.fromLTWH(
      center.dx - iconSize * 0.1,
      center.dy - iconSize * 0.3,
      iconSize * 0.1,
      iconSize * 0.5,
    ));

    canvas.drawPath(path, paint);
  }

  @override
  bool shouldRepaint(_VisualizerPainter oldDelegate) => true;
}
