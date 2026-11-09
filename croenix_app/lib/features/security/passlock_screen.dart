import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../core/theme/croenix_colors.dart';
import '../../data/services/passlock_service.dart';

final passlockServiceProvider = Provider<PasslockService>((ref) {
  return PasslockService();
});

enum PasslockMode {
  unlock,
  setup,
}

class PasslockScreen extends ConsumerStatefulWidget {
  final PasslockMode mode;
  final VoidCallback? onSuccess;
  final VoidCallback? onCancel;

  const PasslockScreen({
    super.key,
    this.mode = PasslockMode.unlock,
    this.onSuccess,
    this.onCancel,
  });

  @override
  ConsumerState<PasslockScreen> createState() => _PasslockScreenState();
}

class _PasslockScreenState extends ConsumerState<PasslockScreen> {
  String _enteredPin = '';
  String? _firstPin;
  String _errorMessage = '';
  bool _isConfirming = false;

  void _onDigitPressed(String digit) {
    if (_enteredPin.length >= 4) return;

    setState(() {
      _enteredPin += digit;
      _errorMessage = '';
    });

    if (_enteredPin.length == 4) {
      _handlePinComplete();
    }
  }

  void _onBackspace() {
    if (_enteredPin.isEmpty) return;
    setState(() {
      _enteredPin = _enteredPin.substring(0, _enteredPin.length - 1);
      _errorMessage = '';
    });
  }

  Future<void> _handlePinComplete() async {
    final passlockService = ref.read(passlockServiceProvider);

    if (widget.mode == PasslockMode.unlock) {
      final isValid = await passlockService.verifyPasslock(_enteredPin);
      if (isValid) {
        if (widget.onSuccess != null) {
          widget.onSuccess!();
        } else if (mounted && Navigator.canPop(context)) {
          Navigator.pop(context, true);
        }
      } else {
        setState(() {
          _enteredPin = '';
          _errorMessage = 'Incorrect PIN. Unauthorized access prevented.';
        });
      }
    } else {
      // Setup mode
      if (!_isConfirming) {
        setState(() {
          _firstPin = _enteredPin;
          _enteredPin = '';
          _isConfirming = true;
        });
      } else {
        if (_enteredPin == _firstPin) {
          await passlockService.setPasslock(_enteredPin);
          if (mounted) {
            ScaffoldMessenger.of(context).showSnackBar(
              const SnackBar(
                content: Text('Security Passlock enabled successfully'),
                backgroundColor: CroenixColors.success,
              ),
            );
            if (widget.onSuccess != null) {
              widget.onSuccess!();
            } else if (Navigator.canPop(context)) {
              Navigator.pop(context, true);
            }
          }
        } else {
          setState(() {
            _enteredPin = '';
            _firstPin = null;
            _isConfirming = false;
            _errorMessage = 'PINs did not match. Please start again.';
          });
        }
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    String titleText = 'Enter Security PIN';
    String subtitleText = 'Unauthorized access is protected';

    if (widget.mode == PasslockMode.setup) {
      if (_isConfirming) {
        titleText = 'Confirm Security PIN';
        subtitleText = 'Re-enter your 4-digit PIN to confirm';
      } else {
        titleText = 'Create Passlock PIN';
        subtitleText = 'Set a 4-digit code to secure your Croenix player';
      }
    }

    return Scaffold(
      appBar: AppBar(
        title: Text(widget.mode == PasslockMode.setup ? 'Security Setup' : 'Security Lock'),
        leading: widget.onCancel != null || Navigator.canPop(context)
            ? IconButton(
                icon: const Icon(Icons.arrow_back),
                onPressed: () {
                  if (widget.onCancel != null) {
                    widget.onCancel!();
                  } else if (Navigator.canPop(context)) {
                    Navigator.pop(context, false);
                  }
                },
              )
            : null,
      ),
      body: SafeArea(
        child: Column(
          children: [
            const Spacer(),

            // Lock Icon
            Container(
              width: 80,
              height: 80,
              decoration: BoxDecoration(
                gradient: LinearGradient(
                  colors: CroenixColors.primaryGradient,
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                ),
                shape: BoxShape.circle,
              ),
              child: const Icon(
                Icons.lock_outline,
                color: Colors.white,
                size: 40,
              ),
            ),
            const SizedBox(height: 24),

            // Title & Subtitle
            Text(
              titleText,
              style: Theme.of(context).textTheme.headlineMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
            const SizedBox(height: 8),
            Text(
              subtitleText,
              style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                    color: CroenixColors.textSecondary,
                  ),
              textAlign: TextAlign.center,
            ),
            const SizedBox(height: 32),

            // PIN Dots Indicator
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: List.generate(4, (index) {
                final isFilled = index < _enteredPin.length;
                return AnimatedContainer(
                  duration: const Duration(milliseconds: 200),
                  margin: const EdgeInsets.symmetric(horizontal: 12),
                  width: 18,
                  height: 18,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: isFilled ? CroenixColors.primary : Colors.transparent,
                    border: Border.all(
                      color: isFilled ? CroenixColors.primary : CroenixColors.border,
                      width: 2,
                    ),
                  ),
                );
              }),
            ),

            if (_errorMessage.isNotEmpty) ...[
              const SizedBox(height: 16),
              Text(
                _errorMessage,
                style: const TextStyle(
                  color: CroenixColors.error,
                  fontSize: 14,
                  fontWeight: FontWeight.w500,
                ),
                textAlign: TextAlign.center,
              ),
            ],

            const Spacer(),

            // Numeric Keypad
            _buildKeypad(),
            const SizedBox(height: 24),
          ],
        ),
      ),
    );
  }

  Widget _buildKeypad() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 40),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildKey('1'),
              _buildKey('2'),
              _buildKey('3'),
            ],
          ),
          const SizedBox(height: 16),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildKey('4'),
              _buildKey('5'),
              _buildKey('6'),
            ],
          ),
          const SizedBox(height: 16),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildKey('7'),
              _buildKey('8'),
              _buildKey('9'),
            ],
          ),
          const SizedBox(height: 16),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              const SizedBox(width: 72, height: 72),
              _buildKey('0'),
              _buildActionKey(
                icon: Icons.backspace_outlined,
                onPressed: _onBackspace,
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildKey(String label) {
    return SizedBox(
      width: 72,
      height: 72,
      child: OutlinedButton(
        style: OutlinedButton.styleFrom(
          shape: const CircleBorder(),
          side: const BorderSide(color: CroenixColors.border, width: 1.5),
          padding: EdgeInsets.zero,
        ),
        onPressed: () => _onDigitPressed(label),
        child: Text(
          label,
          style: const TextStyle(
            fontSize: 26,
            fontWeight: FontWeight.w600,
            color: CroenixColors.textPrimary,
          ),
        ),
      ),
    );
  }

  Widget _buildActionKey({required IconData icon, required VoidCallback onPressed}) {
    return SizedBox(
      width: 72,
      height: 72,
      child: IconButton(
        icon: Icon(icon, color: CroenixColors.textSecondary, size: 28),
        onPressed: onPressed,
      ),
    );
  }
}

