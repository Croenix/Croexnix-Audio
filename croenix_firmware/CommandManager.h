#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "StateManager.h"

enum class CommandResult {
  SUCCESS,
  INVALID_COMMAND,
  INVALID_PARAMETERS,
  UNAVAILABLE,
  ERROR
};

struct CommandResponse {
  CommandResult result;
  String message;
  String data;  // JSON data if applicable
};

class CommandManager {
public:
  CommandManager(StateManager* stateManager);

  CommandResponse executeCommand(const String& command, const JsonDocument& params);

private:
  StateManager* _stateManager;

  // Command handlers
  CommandResponse _handleSetVolume(const JsonDocument& params);
  CommandResponse _handleSetMute(const JsonDocument& params);
  CommandResponse _handleSetSource(const JsonDocument& params);
  CommandResponse _handlePlayback(const String& action);
};

#endif // COMMAND_MANAGER_H
