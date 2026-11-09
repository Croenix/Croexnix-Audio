#ifndef CROENIX_ERROR_MANAGER_H
#define CROENIX_ERROR_MANAGER_H

#include <Arduino.h>
#include <vector>

struct ErrorRecord {
  uint32_t code;
  String subsystem;
  String message;
  unsigned long timestamp;
};

class ErrorManager {
public:
  static ErrorManager& getInstance();

  void reportError(uint32_t code, const String& subsystem, const String& message);
  void reportWarning(const String& subsystem, const String& message);
  uint32_t getErrorCount();
  String getLastErrorJson();

private:
  ErrorManager();
  std::vector<ErrorRecord> _history;
  uint32_t _errorCount;
  SemaphoreHandle_t _mutex;
};

#endif // CROENIX_ERROR_MANAGER_H

