#ifndef CROENIX_ROTARY_ENCODER_H
#define CROENIX_ROTARY_ENCODER_H

#include <Arduino.h>

class PlayerStateManager;
class AuthManager;

class RotaryEncoder {
public:
  RotaryEncoder(PlayerStateManager* stateManager, AuthManager* authManager);
  void begin();
  void update();

  // Called from ISR or polled
  static void IRAM_ATTR isrClockChange();

private:
  PlayerStateManager* _stateManager;
  AuthManager* _authManager;

  bool _initialized;
  int _lastClkState;
  int _lastDtState;

  // Switch state & debouncing
  bool _lastSwState;
  unsigned long _swDownTime;
  bool _swPressed;
  bool _longPressTriggered;

  void _onRotaryTurn(int delta);
  void _onShortPress();
  void _onLongPress();
};

#endif // CROENIX_ROTARY_ENCODER_H

