#ifndef CROENIX_COMMAND_MANAGER_H
#define CROENIX_COMMAND_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

class PlayerStateManager;
class CroenixWiFiManager;

struct CommandExecutionResult {
  bool success;
  String command;
  String requestId;
  String appliedValueJson;
  String errorCode;
  String errorMessage;
  String customDataJson;

  String toJson() const;
};

class CommandManager {
public:
  CommandManager(PlayerStateManager* stateManager, CroenixWiFiManager* wifiManager);

  CommandExecutionResult execute(const String& command, const JsonDocument& params, const String& requestId = "");

private:
  PlayerStateManager* _stateManager;
  CroenixWiFiManager* _wifiManager;

  CommandExecutionResult _handleSetVolume(const JsonDocument& params, const String& requestId);
  CommandExecutionResult _handleSetMute(const JsonDocument& params, const String& requestId);
  CommandExecutionResult _handleSetSource(const JsonDocument& params, const String& requestId);
  CommandExecutionResult _handlePlayback(const String& action, const JsonDocument& params, const String& requestId);
  CommandExecutionResult _handleFm(const String& action, const JsonDocument& params, const String& requestId);
  CommandExecutionResult _handleBluetooth(const String& action, const JsonDocument& params, const String& requestId);
  CommandExecutionResult _handleLibrary(const String& action, const JsonDocument& params, const String& requestId);
  CommandExecutionResult _handleNetwork(const String& action, const JsonDocument& params, const String& requestId);
};

#endif // CROENIX_COMMAND_MANAGER_H

