import 'package:flutter/material.dart';
import '../../../core/theme/croenix_colors.dart';

class NetworkListItem extends StatelessWidget {
  final String title;
  final String subtitle;
  final bool isConnected;
  final bool isConnecting;
  final VoidCallback onTap;

  const NetworkListItem({
    super.key,
    required this.title,
    required this.subtitle,
    this.isConnected = false,
    this.isConnecting = false,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      decoration: GlassEffect.standard,
      child: ListTile(
        contentPadding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
        leading: Container(
          width: 48,
          height: 48,
          decoration: BoxDecoration(
            gradient: isConnected
                ? CroenixColors.primaryGradient
                : null,
            color: isConnected ? null : CroenixColors.glassBackground,
            shape: BoxShape.circle,
            border: Border.all(
              color: isConnected
                  ? Colors.transparent
                  : CroenixColors.glassBorder,
              width: 1,
            ),
          ),
          child: Icon(
            Icons.router_rounded,
            color: isConnected
                ? Colors.white
                : CroenixColors.textSecondary,
          ),
        ),
        title: Text(
          title,
          style: TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.w600,
            color: isConnected
                ? CroenixColors.primaryCyan
                : CroenixColors.textPrimary,
          ),
        ),
        subtitle: Text(
          subtitle,
          style: const TextStyle(
            fontSize: 13,
            color: CroenixColors.textSecondary,
          ),
        ),
        trailing: isConnecting
            ? const SizedBox(
                width: 24,
                height: 24,
                child: CircularProgressIndicator(strokeWidth: 2),
              )
            : isConnected
                ? const Icon(
                    Icons.check_circle_rounded,
                    color: CroenixColors.success,
                  )
                : const Icon(
                    Icons.arrow_forward_ios_rounded,
                    size: 16,
                    color: CroenixColors.textSecondary,
                  ),
        onTap: isConnecting ? null : onTap,
      ),
    );
  }
}
