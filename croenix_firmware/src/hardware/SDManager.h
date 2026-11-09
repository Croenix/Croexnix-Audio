#ifndef CROENIX_SD_MANAGER_H
#define CROENIX_SD_MANAGER_H

#include <Arduino.h>

class PlayerStateManager;

class SDManager {
public:
  SDManager(PlayerStateManager* stateManager);
  void begin();
  void update();

  bool isInserted();
  uint64_t getTotalBytes();
  uint64_t getUsedBytes();
  uint64_t getFreeBytes();

private:
  PlayerStateManager* _stateManager;
  bool _cardDetected;
  uint64_t _totalBytes;
  uint64_t _usedBytes;
  uint64_t _freeBytes;
  unsigned long _lastCheckMs;

  void _checkCardState();
};

#endif // CROENIX_SD_MANAGER_H

