#include "CommandManager.h"
#include "PlayerStateManager.h"
#include "../connectivity/CroenixWiFiManager.h"
#include "../Config.h"

String CommandExecutionResult::toJson() const {
  JsonDocument doc;
  doc["success"] = success;
  doc["command"] = command;
  if (!requestId.isEmpty()) {
    doc["request_id"] = requestId;
  }

  if (success) {
    if (!appliedValueJson.isEmpty()) {
      JsonDocument valDoc;
      deserializeJson(valDoc, appliedValueJson);
      doc["applied_value"] = valDoc;
    }
    if (!customDataJson.isEmpty()) {
      JsonDocument custDoc;
      deserializeJson(custDoc, customDataJson);
      doc["data"] = custDoc;
    }
  } else {
    doc["error"]["code"] = errorCode.isEmpty() ? "COMMAND_FAILED" : errorCode;
    doc["error"]["message"] = errorMessage.isEmpty() ? "Unknown command failure" : errorMessage;
  }

  String out;
  serializeJson(doc, out);
  return out;
}

CommandManager::CommandManager(PlayerStateManager* stateManager, CroenixWiFiManager* wifiManager)
  : _stateManager(stateManager), _wifiManager(wifiManager) {
}

CommandExecutionResult CommandManager::execute(const String& command, const JsonDocument& params, const String& requestId) {
  CROENIX_LOG("CommandManager", "Processing command: '%s'", command.c_str());

  if (command == "set_volume") {
    return _handleSetVolume(params, requestId);
  } else if (command == "set_mute") {
    return _handleSetMute(params, requestId);
  } else if (command == "set_source") {
    return _handleSetSource(params, requestId);
  } else if (command == "play" || command == "pause" || command == "stop" ||
             command == "next" || command == "previous" || command == "seek") {
    return _handlePlayback(command, params, requestId);
  } else if (command == "fm_tune" || command == "fm_seek_up" || command == "fm_seek_down") {
    return _handleFm(command, params, requestId);
  } else if (command == "bluetooth_disconnect" || command == "bluetooth_pairing_mode") {
    return _handleBluetooth(command, params, requestId);
  } else if (command == "refresh_library" || command == "select_track") {
    return _handleLibrary(command, params, requestId);
  } else if (command == "configure_wifi" || command == "reconnect_wifi" || command == "get_network") {
    return _handleNetwork(command, params, requestId);
  }

  CommandExecutionResult res;
  res.success = false;
  res.command = command;
  res.requestId = requestId;
  res.errorCode = "UNKNOWN_COMMAND";
  res.errorMessage = "Command '" + command + "' is not recognized";
  return res;
}

CommandExecutionResult CommandManager::_handleSetVolume(const JsonDocument& params, const String& requestId) {
  CommandExecutionResult res;
  res.command = "set_volume";
  res.requestId = requestId;

  if (!params["value"].is<int>()) {
    res.success = false;
    res.errorCode = "INVALID_PARAMETERS";
    res.errorMessage = "Missing or invalid integer 'value' (0-100)";
    return res;
  }

  int val = params["value"].as<int>();
  if (val < 0) val = 0;
  if (val > 100) val = 100;

  if (_stateManager) {
    _stateManager->setVolume(val, true);
  }

  res.success = true;
  res.appliedValueJson = String(val);
  return res;
}

CommandExecutionResult CommandManager::_handleSetMute(const JsonDocument& params, const String& requestId) {
  CommandExecutionResult res;
  res.command = "set_mute";
  res.requestId = requestId;

  if (!params["value"].is<bool>()) {
    res.success = false;
    res.errorCode = "INVALID_PARAMETERS";
    res.errorMessage = "Missing or invalid boolean 'value'";
    return res;
  }

  bool muted = params["value"].as<bool>();
  if (_stateManager) {
    _stateManager->setMute(muted, true);
  }

  res.success = true;
  res.appliedValueJson = muted ? "true" : "false";
  return res;
}

CommandExecutionResult CommandManager::_handleSetSource(const JsonDocument& params, const String& requestId) {
  CommandExecutionResult res;
  res.command = "set_source";
  res.requestId = requestId;

  if (!params["source"].is<String>()) {
    res.success = false;
    res.errorCode = "INVALID_PARAMETERS";
    res.errorMessage = "Missing 'source' string parameter";
    return res;
  }

  String source = params["source"].as<String>();
  if (source == "none") {
    if (_stateManager) _stateManager->setSource("none", true);
    res.success = true;
    res.appliedValueJson = "\"none\"";
    return res;
  }

  // Capability awareness: Hardware modules (SD, BT, FM) check
  if (source == "sd") {
    PlayerState st = _stateManager->getState();
    if (!st.storage.sd_inserted) {
      res.success = false;
      res.errorCode = "NOT_AVAILABLE";
      res.errorMessage = "SD card is not inserted";
      return res;
    }
  }

  // Currently audio decoding hardware is planned / in integration
  res.success = false;
  res.errorCode = "NOT_IMPLEMENTED";
  res.errorMessage = "Audio source '" + source + "' hardware pipeline is not yet attached";
  return res;
}

CommandExecutionResult CommandManager::_handlePlayback(const String& action, const JsonDocument& params, const String& requestId) {
  CommandExecutionResult res;
  res.command = action;
  res.requestId = requestId;
  res.success = false;
  res.errorCode = "NOT_IMPLEMENTED";
  res.errorMessage = "Playback engine for '" + action + "' is in progress";
  return res;
}

CommandExecutionResult CommandManager::_handleFm(const String& action, const JsonDocument& params, const String& requestId) {
  CommandExecutionResult res;
  res.command = action;
  res.requestId = requestId;
  res.success = false;
  res.errorCode = "NOT_IMPLEMENTED";
  res.errorMessage = "FM Tuner hardware module is not connected";
  return res;
}

CommandExecutionResult CommandManager::_handleBluetooth(const String& action, const JsonDocument& params, const String& requestId) {
  CommandExecutionResult res;
  res.command = action;
  res.requestId = requestId;
  res.success = false;
  res.errorCode = "NOT_IMPLEMENTED";
  res.errorMessage = "Bluetooth Classic A2DP audio sink subsystem is in progress";
  return res;
}

CommandExecutionResult CommandManager::_handleLibrary(const String& action, const JsonDocument& params, const String& requestId) {
  CommandExecutionResult res;
  res.command = action;
  res.requestId = requestId;
  res.success = false;
  res.errorCode = "NOT_IMPLEMENTED";
  res.errorMessage = "Library browser requires SD card filesystem activation";
  return res;
}

CommandExecutionResult CommandManager::_handleNetwork(const String& action, const JsonDocument& params, const String& requestId) {
  CommandExecutionResult res;
  res.command = action;
  res.requestId = requestId;

  if (action == "configure_wifi") {
    if (!params["ssid"].is<String>() || !params["password"].is<String>()) {
      res.success = false;
      res.errorCode = "INVALID_PARAMETERS";
      res.errorMessage = "Missing ssid or password";
      return res;
    }
    String ssid = params["ssid"].as<String>();
    String password = params["password"].as<String>();
    bool started = _wifiManager->connectToRouterAsync(ssid, password, true);
    res.success = started;
    if (!started) {
      res.errorCode = "WIFI_FAILED";
      res.errorMessage = "Could not start WiFi connection";
    }
    return res;
  } else if (action == "get_network") {
    res.success = true;
    res.customDataJson = _stateManager->getNetworkApiJson();
    return res;
  }

  res.success = false;
  res.errorCode = "NOT_IMPLEMENTED";
  res.errorMessage = "Action not supported";
  return res;
}

