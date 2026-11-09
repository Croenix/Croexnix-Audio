#include "WebServerManager.h"
#include "Config.h"

WebServerManager::WebServerManager(StateManager* stateManager,
                                   AuthManager* authManager,
                                   WiFiManager* wifiManager,
                                   SystemMonitor* systemMonitor,
                                   CommandManager* commandManager)
  : _server(HTTP_PORT),
    _stateManager(stateManager),
    _authManager(authManager),
    _wifiManager(wifiManager),
    _systemMonitor(systemMonitor),
    _commandManager(commandManager) {
}

void WebServerManager::begin() {
  DEBUG_PRINT("[WebServerManager] Starting HTTP server on port ");
  DEBUG_PRINTLN(HTTP_PORT);

  // CORS headers for all responses
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

  // GET /api/device - Device info
  _server.on("/api/device", HTTP_GET, [this](AsyncWebServerRequest* request) {
    _handleGetDevice(request);
  });

  // GET /api/status - Complete state
  _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    _handleGetStatus(request);
  });

  // GET /api/network - Network status
  _server.on("/api/network", HTTP_GET, [this](AsyncWebServerRequest* request) {
    _handleGetNetwork(request);
  });

  // GET /api/system - System diagnostics
  _server.on("/api/system", HTTP_GET, [this](AsyncWebServerRequest* request) {
    _handleGetSystem(request);
  });

  // POST /api/pair - Pairing (no auth required)
  _server.on("/api/pair", HTTP_POST, [this](AsyncWebServerRequest* request) {
    _handlePostPair(request);
  });

  // POST /api/commands - Execute commands
  _server.on("/api/commands", HTTP_POST,
    [](AsyncWebServerRequest* request) { /* Handled in body callback */ },
    NULL,
    [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) {
        _handlePostCommands(request, data, len);
      }
    }
  );

  // POST /api/network/config - Configure WiFi
  _server.on("/api/network/config", HTTP_POST,
    [](AsyncWebServerRequest* request) { /* Handled in body callback */ },
    NULL,
    [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) {
        _handlePostNetworkConfig(request, data, len);
      }
    }
  );

  // POST /api/device/restart - Restart device
  _server.on("/api/device/restart", HTTP_POST, [this](AsyncWebServerRequest* request) {
    _handlePostRestart(request);
  });

  // Handle OPTIONS for CORS preflight
  _server.onNotFound([this](AsyncWebServerRequest* request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      _handleNotFound(request);
    }
  });

  _server.begin();
  DEBUG_PRINTLN("[WebServerManager] Server started");
}

// Middleware

bool WebServerManager::_checkAuthentication(AsyncWebServerRequest* request) {
  if (!request->hasHeader("Authorization")) {
    _sendError(request, 401, "Missing Authorization header");
    return false;
  }

  String authHeader = request->header("Authorization");

  if (!authHeader.startsWith("Bearer ")) {
    _sendError(request, 401, "Invalid Authorization format");
    return false;
  }

  String token = authHeader.substring(7);

  if (!_authManager->validateToken(token)) {
    _sendError(request, 401, "Invalid token");
    return false;
  }

  return true;
}

void WebServerManager::_sendJsonResponse(AsyncWebServerRequest* request, int code, const String& json) {
  request->send(code, "application/json", json);
}

void WebServerManager::_sendError(AsyncWebServerRequest* request, int code, const String& message) {
  JsonDocument doc;
  doc["error"]["code"] = code;
  doc["error"]["message"] = message;

  String json;
  serializeJson(doc, json);

  _sendJsonResponse(request, code, json);
}

// Route Handlers

void WebServerManager::_handleGetDevice(AsyncWebServerRequest* request) {
  if (!_checkAuthentication(request)) return;

  JsonDocument doc;
  doc["name"] = DEVICE_NAME;
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["hostname"] = DEVICE_HOSTNAME;
  doc["capabilities"]["audio_sources"] = JsonArray();
  doc["capabilities"]["audio_sources"].add("none");  // Phase 1/2 only

  String json;
  serializeJson(doc, json);

  _sendJsonResponse(request, 200, json);
  DEBUG_PRINTLN("[WebServerManager] GET /api/device");
}

void WebServerManager::_handleGetStatus(AsyncWebServerRequest* request) {
  if (!_checkAuthentication(request)) return;

  // Update state before sending
  _stateManager->update();

  // Sync with WiFi and system monitors
  WiFiStatus wifiStatus = _wifiManager->getStatus();
  SystemInfo sysInfo = _systemMonitor->getInfo();

  _stateManager->updateConnection(
    wifiStatus.sta_connected,
    wifiStatus.sta_ip.toString(),
    wifiStatus.sta_ssid,
    wifiStatus.sta_rssi,
    wifiStatus.ap_active,
    wifiStatus.ap_clients
  );

  _stateManager->updateSystem(
    sysInfo.free_heap,
    sysInfo.total_heap,
    sysInfo.min_free_heap,
    sysInfo.restart_reason
  );

  String json = _stateManager->toJson();
  _sendJsonResponse(request, 200, json);

  DEBUG_PRINTLN("[WebServerManager] GET /api/status");
}

void WebServerManager::_handleGetNetwork(AsyncWebServerRequest* request) {
  if (!_checkAuthentication(request)) return;

  WiFiStatus status = _wifiManager->getStatus();

  JsonDocument doc;

  // AP status
  doc["ap"]["active"] = status.ap_active;
  doc["ap"]["ssid"] = AP_SSID;
  doc["ap"]["ip"] = status.ap_ip.toString();
  doc["ap"]["clients"] = status.ap_clients;

  // STA status
  doc["router"]["connected"] = status.sta_connected;
  doc["router"]["ssid"] = status.sta_ssid;
  doc["router"]["ip"] = status.sta_ip.toString();
  doc["router"]["rssi"] = status.sta_rssi;

  String json;
  serializeJson(doc, json);

  _sendJsonResponse(request, 200, json);
  DEBUG_PRINTLN("[WebServerManager] GET /api/network");
}

void WebServerManager::_handleGetSystem(AsyncWebServerRequest* request) {
  if (!_checkAuthentication(request)) return;

  SystemInfo info = _systemMonitor->getInfo();

  JsonDocument doc;
  doc["free_heap"] = info.free_heap;
  doc["total_heap"] = info.total_heap;
  doc["min_free_heap"] = info.min_free_heap;
  doc["uptime_seconds"] = info.uptime_seconds;
  doc["restart_reason"] = info.restart_reason;

  String json;
  serializeJson(doc, json);

  _sendJsonResponse(request, 200, json);
  DEBUG_PRINTLN("[WebServerManager] GET /api/system");
}

void WebServerManager::_handlePostPair(AsyncWebServerRequest* request) {
  if (!_authManager->isPairingModeActive()) {
    _sendError(request, 403, "Pairing mode not active. Press encoder button to enable.");
    return;
  }

  // Generate token (confirmation happens when encoder is pressed)
  String token = _authManager->generateNewToken();

  if (token.isEmpty()) {
    _sendError(request, 500, "Failed to generate token");
    return;
  }

  JsonDocument doc;
  doc["success"] = true;
  doc["token"] = token;
  doc["message"] = "Pairing successful";

  String json;
  serializeJson(doc, json);

  _sendJsonResponse(request, 200, json);
  DEBUG_PRINTLN("[WebServerManager] POST /api/pair - Token issued");
}

void WebServerManager::_handlePostCommands(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  if (!_checkAuthentication(request)) return;

  // Parse JSON body
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, data, len);

  if (error) {
    _sendError(request, 400, "Invalid JSON");
    return;
  }

  if (!doc.containsKey("command")) {
    _sendError(request, 400, "Missing 'command' field");
    return;
  }

  String command = doc["command"].as<String>();
  JsonDocument params = doc["params"];

  // Execute command
  CommandResponse response = _commandManager->executeCommand(command, params);

  // Build response
  JsonDocument responseDoc;
  responseDoc["success"] = (response.result == CommandResult::SUCCESS);
  responseDoc["message"] = response.message;

  if (!response.data.isEmpty()) {
    JsonDocument dataDoc;
    deserializeJson(dataDoc, response.data);
    responseDoc["data"] = dataDoc;
  }

  String json;
  serializeJson(responseDoc, json);

  int httpCode = (response.result == CommandResult::SUCCESS) ? 200 : 400;
  _sendJsonResponse(request, httpCode, json);

  DEBUG_PRINT("[WebServerManager] POST /api/commands - ");
  DEBUG_PRINTLN(command);
}

void WebServerManager::_handlePostNetworkConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  if (!_checkAuthentication(request)) return;

  // Parse JSON body
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, data, len);

  if (error) {
    _sendError(request, 400, "Invalid JSON");
    return;
  }

  if (!doc.containsKey("ssid") || !doc.containsKey("password")) {
    _sendError(request, 400, "Missing 'ssid' or 'password'");
    return;
  }

  String ssid = doc["ssid"].as<String>();
  String password = doc["password"].as<String>();

  bool success = _wifiManager->connectToRouter(ssid, password, true);

  JsonDocument responseDoc;
  responseDoc["success"] = success;
  responseDoc["message"] = success ? "WiFi configuration saved" : "Failed to configure WiFi";

  String json;
  serializeJson(responseDoc, json);

  _sendJsonResponse(request, success ? 200 : 500, json);

  DEBUG_PRINTLN("[WebServerManager] POST /api/network/config");
}

void WebServerManager::_handlePostRestart(AsyncWebServerRequest* request) {
  if (!_checkAuthentication(request)) return;

  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = "Restarting device...";

  String json;
  serializeJson(doc, json);

  _sendJsonResponse(request, 200, json);

  DEBUG_PRINTLN("[WebServerManager] POST /api/device/restart - Restarting in 2 seconds");

  // Restart after a delay to allow response to be sent
  delay(2000);
  ESP.restart();
}

void WebServerManager::_handleNotFound(AsyncWebServerRequest* request) {
  _sendError(request, 404, "Endpoint not found");
  DEBUG_PRINT("[WebServerManager] 404 - ");
  DEBUG_PRINTLN(request->url());
}
