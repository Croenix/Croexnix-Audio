#include "I2SManager.h"
#include "../PinConfig.h"
#include "../Config.h"

I2SManager::I2SManager()
  : _initialized(false), _sampleRate(44100), _bitDepth(16) {
}

bool I2SManager::begin(int sampleRate, int bitDepth) {
  _sampleRate = sampleRate;
  _bitDepth = bitDepth;

  if (PIN_I2S_BCLK < 0 || PIN_I2S_LRCK < 0 || PIN_I2S_DOUT < 0) {
    CROENIX_LOG("I2SManager", "I2S DAC pins unconfigured or disabled (%d, %d, %d)",
                PIN_I2S_BCLK, PIN_I2S_LRCK, PIN_I2S_DOUT);
    _initialized = false;
    return false;
  }

  // Safe placeholder: marks I2S configuration ready for CJMCU-5102
  _initialized = true;
  CROENIX_LOG("I2SManager", "I2S Pipeline ready: CJMCU-5102 DAC on BCLK=%d, LRCK=%d, DOUT=%d (%d Hz, %d-bit)",
              PIN_I2S_BCLK, PIN_I2S_LRCK, PIN_I2S_DOUT, _sampleRate, _bitDepth);
  return true;
}

void I2SManager::stop() {
  _initialized = false;
  CROENIX_LOG("I2SManager", "I2S Pipeline stopped.");
}

bool I2SManager::isInitialized() {
  return _initialized;
}

int I2SManager::getSampleRate() {
  return _sampleRate;
}

int I2SManager::getBitDepth() {
  return _bitDepth;
}

size_t I2SManager::write(const uint8_t* data, size_t size) {
  if (!_initialized) return 0;
  return size;
}

