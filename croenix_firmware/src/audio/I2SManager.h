#ifndef CROENIX_I2S_MANAGER_H
#define CROENIX_I2S_MANAGER_H

#include <Arduino.h>

class I2SManager {
public:
  I2SManager();
  bool begin(int sampleRate = 44100, int bitDepth = 16);
  void stop();

  bool isInitialized();
  int getSampleRate();
  int getBitDepth();

  size_t write(const uint8_t* data, size_t size);

private:
  bool _initialized;
  int _sampleRate;
  int _bitDepth;
};

#endif // CROENIX_I2S_MANAGER_H

