#include "RotaryEncoder.h"
#include "../core/PlayerStateManager.h"
#include "../connectivity/AuthManager.h"
#include "../PinConfig.h"
#include "../Config.h"

static volatile int s_rotaryCounter = 0;
static portMUX_TYPE s_encoderMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR RotaryEncoder::isrClockChange() {
  if (PIN_ENCODER_CLK < 0 || PIN_ENCODER_DT < 0) return;

  int clk = digitalRead(PIN_ENCODER_CLK);
  int dt = digitalRead(PIN_ENCODER_DT);

  portENTER_CRITICAL_ISR(&s_encoderMux);
  if (clk != dt) {
    s_rotaryCounter++;
  } else {
    s_rotaryCounter--;
  }
  portEXIT_CRITICAL_ISR(&s_encoderMux);
}

RotaryEncoder::RotaryEncoder(PlayerStateManager* stateManager, AuthManager* authManager)
  : _stateManager(stateManager),
    _authManager(authManager),
    _initialized(false),
    _lastClkState(HIGH),
    _lastDtState(HIGH),
    _lastSwState(HIGH),
    _swDownTime(0),
    _swPressed(false),
    _longPressTriggered(false) {
}

void RotaryEncoder::begin() {
  if (PIN_ENCODER_CLK < 0 || PIN_ENCODER_DT < 0) {
    CROENIX_LOG("Encoder", "Pins disabled or unconfigured in PinConfig.h (%d, %d)", PIN_ENCODER_CLK, PIN_ENCODER_DT);
    return;
  }

  pinMode(PIN_ENCODER_CLK, INPUT_PULLUP);
  pinMode(PIN_ENCODER_DT, INPUT_PULLUP);

  if (PIN_ENCODER_SW >= 0) {
    pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
    _lastSwState = digitalRead(PIN_ENCODER_SW);
  }

  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_CLK), RotaryEncoder::isrClockChange, FALLING);

  _initialized = true;
  CROENIX_LOG("Encoder", "KY-040 rotary encoder initialized on CLK=%d, DT=%d, SW=%d",
              PIN_ENCODER_CLK, PIN_ENCODER_DT, PIN_ENCODER_SW);
}

void RotaryEncoder::update() {
  if (!_initialized) return;

  // Process rotary counter from ISR safely
  int delta = 0;
  portENTER_CRITICAL(&s_encoderMux);
  delta = s_rotaryCounter;
  s_rotaryCounter = 0;
  portEXIT_CRITICAL(&s_encoderMux);

  if (delta != 0) {
    _onRotaryTurn(delta);
  }

  // Switch button handling
  if (PIN_ENCODER_SW >= 0) {
    bool rawSw = digitalRead(PIN_ENCODER_SW);
    bool pressed = (rawSw == (ENCODER_ACTIVE_LOW ? LOW : HIGH));

    if (pressed && !_swPressed) {
      _swPressed = true;
      _swDownTime = millis();
      _longPressTriggered = false;
    } else if (pressed && _swPressed) {
      if (!_longPressTriggered && (millis() - _swDownTime > 2000)) {
        _longPressTriggered = true;
        _onLongPress();
      }
    } else if (!pressed && _swPressed) {
      _swPressed = false;
      if (!_longPressTriggered && (millis() - _swDownTime > 50)) {
        _onShortPress();
      }
    }
  }
}

void RotaryEncoder::_onRotaryTurn(int delta) {
  if (!_stateManager) return;
  PlayerState st = _stateManager->getState();
  int newVol = st.audio.volume + delta;
  if (newVol < 0) newVol = 0;
  if (newVol > 100) newVol = 100;
  _stateManager->setVolume(newVol, true);
  CROENIX_LOG("Encoder", "Physical rotary turned: volume %d (delta %d)", newVol, delta);
}

void RotaryEncoder::_onShortPress() {
  if (!_stateManager) return;
  PlayerState st = _stateManager->getState();
  _stateManager->setMute(!st.audio.muted, true);
  CROENIX_LOG("Encoder", "Switch short press: toggle mute to %s", !st.audio.muted ? "MUTED" : "UNMUTED");
}

void RotaryEncoder::_onLongPress() {
  if (_authManager) {
    _authManager->enablePairingMode(PAIRING_TIMEOUT_MS);
    CROENIX_LOG("Encoder", "Switch long press: Activated pairing mode for %d seconds!", PAIRING_TIMEOUT_MS / 1000);
  }
}

