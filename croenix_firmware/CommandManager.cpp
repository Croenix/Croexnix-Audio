#include "CommandManager.h"
#include "Config.h"

CommandManager::CommandManager(StateManager* stateManager)
  : _stateManager(stateManager) {
}

CommandResponse CommandManager::executeCommand(const String& command, const JsonDocument& params) {
  DEBUG_PRINT("[CommandManager] Executing command: ");
  DEBUG_PRINTLN(command);

  CommandResponse response;
  response.result = CommandResult::SUCCESS;
  response.message = "OK";

  if (command == "set_volume") {
    response = _handleSetVolume(params);
  }
  else if (command == "set_mute") {
    response = _handleSetMute(params);
  }
  else if (command == "set_source") {
    response = _handleSetSource(params);
  }
  else if (command == "play" || command == "pause" || command == "stop" ||
           command == "next" || command == "previous") {
    response = _handlePlayback(command);
  }
  else {
    response.result = CommandResult::INVALID_COMMAND;
    response.message = "Unknown command: " + command;
    DEBUG_PRINTLN(response.message);
  }

  return response;
}

// Private command handlers

CommandResponse CommandManager::_handleSetVolume(const JsonDocument& params) {
  CommandResponse response;

  if (!params.containsKey("value")) {
    response.result = CommandResult::INVALID_PARAMETERS;
    response.message = "Missing 'value' parameter";
    return response;
  }

  int volume = params["value"];

  // Validate range
  if (volume < 0 || volume > 100) {
    response.result = CommandResult::INVALID_PARAMETERS;
    response.message = "Volume must be between 0 and 100";
    return response;
  }

  // Update state
  DeviceState state = _stateManager->getState();
  state.audio.volume = volume;
  _stateManager->setState(state);

  // In Phase 4+, this would actually control hardware volume
  DEBUG_PRINT("[CommandManager] Volume set to: ");
  DEBUG_PRINTLN(volume);

  response.result = CommandResult::SUCCESS;
  response.message = "Volume set to " + String(volume);

  // Return applied value
  JsonDocument doc;
  doc["applied_value"] = volume;
  String json;
  serializeJson(doc, json);
  response.data = json;

  return response;
}

CommandResponse CommandManager::_handleSetMute(const JsonDocument& params) {
  CommandResponse response;

  if (!params.containsKey("value")) {
    response.result = CommandResult::INVALID_PARAMETERS;
    response.message = "Missing 'value' parameter";
    return response;
  }

  bool muted = params["value"];

  // Update state
  DeviceState state = _stateManager->getState();
  state.audio.muted = muted;
  _stateManager->setState(state);

  DEBUG_PRINT("[CommandManager] Mute set to: ");
  DEBUG_PRINTLN(muted ? "true" : "false");

  response.result = CommandResult::SUCCESS;
  response.message = muted ? "Muted" : "Unmuted";

  JsonDocument doc;
  doc["applied_value"] = muted;
  String json;
  serializeJson(doc, json);
  response.data = json;

  return response;
}

CommandResponse CommandManager::_handleSetSource(const JsonDocument& params) {
  CommandResponse response;

  if (!params.containsKey("source")) {
    response.result = CommandResult::INVALID_PARAMETERS;
    response.message = "Missing 'source' parameter";
    return response;
  }

  String source = params["source"].as<String>();

  // Validate source (placeholder - actual sources in Phase 4+)
  if (source != "none" && source != "bluetooth" && source != "sd" &&
      source != "fm" && source != "webradio" && source != "online") {
    response.result = CommandResult::INVALID_PARAMETERS;
    response.message = "Invalid source: " + source;
    return response;
  }

  // For Phase 1+2, only "none" is available
  if (source != "none") {
    response.result = CommandResult::UNAVAILABLE;
    response.message = "Source '" + source + "' not available in Phase 1/2";
    return response;
  }

  // Update state
  DeviceState state = _stateManager->getState();
  state.audio.source = source;
  _stateManager->setState(state);

  DEBUG_PRINT("[CommandManager] Source set to: ");
  DEBUG_PRINTLN(source);

  response.result = CommandResult::SUCCESS;
  response.message = "Source set to " + source;

  return response;
}

CommandResponse CommandManager::_handlePlayback(const String& action) {
  CommandResponse response;

  // Placeholder for Phase 4+ - playback not yet implemented
  response.result = CommandResult::UNAVAILABLE;
  response.message = "Playback control not available in Phase 1/2";

  DEBUG_PRINT("[CommandManager] Playback command '");
  DEBUG_PRINT(action);
  DEBUG_PRINTLN("' not yet implemented");

  return response;
}
