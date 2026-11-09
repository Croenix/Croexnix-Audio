#include "HttpServerManager.h"
#include "WebSocketManager.h"
#include "../core/PlayerStateManager.h"
#include "../core/CommandManager.h"
#include "AuthManager.h"
#include "CroenixWiFiManager.h"
#include "../diagnostics/SystemMonitor.h"
#include "../Config.h"
#include <ArduinoJson.h>

static void restartTask(void* param) {
  vTaskDelay(pdMS_TO_TICKS(1500));
  ESP.restart();
  vTaskDelete(NULL);
}

HttpServerManager::HttpServerManager(PlayerStateManager* stateManager,
                                     AuthManager* authManager,
                                     CroenixWiFiManager* wifiManager,
                                     SystemMonitor* systemMonitor,
                                     CommandManager* commandManager)
  : _server(HTTP_PORT),
    _stateManager(stateManager),
    _authManager(authManager),
    _wifiManager(wifiManager),
    _systemMonitor(systemMonitor),
    _commandManager(commandManager) {
  _wsManager = new WebSocketManager(&_server, _stateManager, _authManager, _commandManager);
}

void HttpServerManager::begin() {
  CROENIX_LOG("HttpServer", "Starting asynchronous HTTP API on port %d...", HTTP_PORT);

  // Global CORS headers
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

  // OPTIONS handler for browser / web client preflight
  _server.onNotFound([this](AsyncWebServerRequest* request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      _sendError(request, 404, "Endpoint not found", "NOT_FOUND");
    }
  });

  // 1. GET /api/device
  _server.on("/api/device", HTTP_GET, [this](AsyncWebServerRequest* request) {
    _handleGetDevice(request);
  });

  // 2. GET /api/status
  _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    _handleGetStatus(request);
  });

  // 3. GET /api/network
  _server.on("/api/network", HTTP_GET, [this](AsyncWebServerRequest* request) {
    _handleGetNetwork(request);
  });

  // 4. GET /api/system
  _server.on("/api/system", HTTP_GET, [this](AsyncWebServerRequest* request) {
    _handleGetSystem(request);
  });

  // 5. POST /api/pair (Unauthenticated, pairing mode check)
  _server.on("/api/pair", HTTP_POST, [this](AsyncWebServerRequest* request) {
    _handlePostPair(request);
  });

  // 6. POST /api/commands
  _server.on("/api/commands", HTTP_POST,
    [](AsyncWebServerRequest* request) { /* Request filter */ },
    NULL,
    [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) {
        _handlePostCommands(request, data, len);
      }
    }
  );

  // 7. POST /api/network/config
  _server.on("/api/network/config", HTTP_POST,
    [](AsyncWebServerRequest* request) { /* Request filter */ },
    NULL,
    [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) {
        _handlePostNetworkConfig(request, data, len);
      }
    }
  );

  // 8. POST /api/device/restart
  _server.on("/api/device/restart", HTTP_POST, [this](AsyncWebServerRequest* request) {
    _handlePostRestart(request);
  });

  // Initialize WebSocket subsystem
  _wsManager->begin();

  _server.begin();
  CROENIX_LOG("HttpServer", "REST API running with 8 endpoints and /ws WebSocket channel.");
}

void HttpServerManager::update() {
  if (_wsManager) {
    _wsManager->update();
  }
}

AsyncWebServer* HttpServerManager::getServer() {
  return &_server;
}

bool HttpServerManager::_isRequestAuthorized(AsyncWebServerRequest* request) {
  if (!request->hasHeader("Authorization")) {
    _sendError(request, 401, "Missing Authorization header", "AUTH_REQUIRED");
    return false;
  }

  String headerVal = request->header("Authorization");
  if (!headerVal.startsWith("Bearer ")) {
    _sendError(request, 401, "Authorization header must be Bearer token", "INVALID_AUTH_FORMAT");
    return false;
  }

  String token = headerVal.substring(7);
  if (!_authManager->validateToken(token)) {
    if (_authManager->isLockedOut()) {
      _sendError(request, 429, "Too many failed attempts. Temporary lockout.", "RATE_LIMITED");
    } else {
      _sendError(request, 401, "Invalid or expired authorization token", "INVALID_TOKEN");
    }
    return false;
  }

  return true;
}

void HttpServerManager::_sendJson(AsyncWebServerRequest* request, int code, const String& json) {
  request->send(code, "application/json", json);
}

void HttpServerManager::_sendError(AsyncWebServerRequest* request, int code, const String& message, const String& errCode) {
  JsonDocument doc;
  doc["error"]["code"] = errCode.isEmpty() ? String(code) : errCode;
  doc["error"]["message"] = message;
  String out;
  serializeJson(doc, out);
  _sendJson(request, code, out);
}

void HttpServerManager::_handleGetDevice(AsyncWebServerRequest* request) {
  if (!_isRequestAuthorized(request)) return;
  if (_stateManager) {
    _sendJson(request, 200, _stateManager->getDeviceApiJson());
  } else {
    _sendError(request, 500, "State manager unavailable");
  }
}

void HttpServerManager::_handleGetStatus(AsyncWebServerRequest* request) {
  if (!_isRequestAuthorized(request)) return;
  if (_stateManager) {
    _stateManager->update();
    _sendJson(request, 200, _stateManager->toJson());
  } else {
    _sendError(request, 500, "State manager unavailable");
  }
}

void HttpServerManager::_handleGetNetwork(AsyncWebServerRequest* request) {
  if (!_isRequestAuthorized(request)) return;
  if (_stateManager) {
    _sendJson(request, 200, _stateManager->getNetworkApiJson());
  } else {
    _sendError(request, 500, "State manager unavailable");
  }
}

void HttpServerManager::_handleGetSystem(AsyncWebServerRequest* request) {
  if (!_isRequestAuthorized(request)) return;
  if (_stateManager) {
    _sendJson(request, 200, _stateManager->getSystemApiJson());
  } else {
    _sendError(request, 500, "State manager unavailable");
  }
}

void HttpServerManager::_handlePostPair(AsyncWebServerRequest* request) {
  if (!_authManager->isPairingModeActive()) {
    _sendError(request, 403, "Pairing mode not active. Press encoder button on device.", "PAIRING_INACTIVE");
    return;
  }

  String token = _authManager->generateAndStoreToken();
  if (token.isEmpty()) {
    _sendError(request, 500, "Failed to generate pairing token", "TOKEN_GEN_ERROR");
    return;
  }

  JsonDocument doc;
  doc["success"] = true;
  doc["token"] = token;
  doc["message"] = "Pairing successful";
  String out;
  serializeJson(doc, out);
  _sendJson(request, 200, out);
  CROENIX_LOG("HttpServer", "Pairing completed. Issued token to client.");
}

void HttpServerManager::_handlePostCommands(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  if (!_isRequestAuthorized(request)) return;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, data, len);
  if (err) {
    _sendError(request, 400, "Invalid JSON body", "BAD_REQUEST");
    return;
  }

  if (!doc["command"].is<String>()) {
    _sendError(request, 400, "Missing required 'command' field", "BAD_REQUEST");
    return;
  }

  String command = doc["command"].as<String>();
  String reqId = doc["request_id"].is<String>() ? doc["request_id"].as<String>() : "";
  JsonDocument params = doc["params"];

  CommandExecutionResult result = _commandManager->execute(command, params, reqId);
  int httpStatus = result.success ? 200 : (result.errorCode == "NOT_IMPLEMENTED" ? 200 : 400);
  _sendJson(request, httpStatus, result.toJson());
}

void HttpServerManager::_handlePostNetworkConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  if (!_isRequestAuthorized(request)) return;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, data, len);
  if (err) {
    _sendError(request, 400, "Invalid JSON body", "BAD_REQUEST");
    return;
  }

  if (!doc["ssid"].is<String>() || !doc["password"].is<String>()) {
    _sendError(request, 400, "Missing 'ssid' or 'password'", "BAD_REQUEST");
    return;
  }

  String ssid = doc["ssid"].as<String>();
  String pass = doc["password"].as<String>();

  bool started = _wifiManager->connectToRouterAsync(ssid, pass, true);
  JsonDocument resDoc;
  resDoc["success"] = started;
  resDoc["message"] = started ? "WiFi configuration accepted. Connecting to router..." : "Failed to initiate WiFi connection";
  String out;
  serializeJson(resDoc, out);
  _sendJson(request, started ? 200 : 500, out);
}

void HttpServerManager::_handlePostRestart(AsyncWebServerRequest* request) {
  if (!_isRequestAuthorized(request)) return;

  JsonDocument resDoc;
  resDoc["success"] = true;
  resDoc["message"] = "Croenix player is restarting...";
  String out;
  serializeJson(resDoc, out);
  _sendJson(request, 200, out);

  CROENIX_LOG("HttpServer", "Safe restart scheduled via FreeRTOS background task.");
  xTaskCreate(restartTask, "reboot_task", 2048, NULL, 5, NULL);
}

