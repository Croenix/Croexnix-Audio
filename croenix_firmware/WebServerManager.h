#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "StateManager.h"
#include "AuthManager.h"
#include "WiFiManager.h"
#include "SystemMonitor.h"
#include "CommandManager.h"

class WebServerManager {
public:
  WebServerManager(StateManager* stateManager,
                   AuthManager* authManager,
                   WiFiManager* wifiManager,
                   SystemMonitor* systemMonitor,
                   CommandManager* commandManager);

  void begin();

private:
  AsyncWebServer _server;
  StateManager* _stateManager;
  AuthManager* _authManager;
  WiFiManager* _wifiManager;
  SystemMonitor* _systemMonitor;
  CommandManager* _commandManager;

  // Middleware
  bool _checkAuthentication(AsyncWebServerRequest* request);
  void _sendJsonResponse(AsyncWebServerRequest* request, int code, const String& json);
  void _sendError(AsyncWebServerRequest* request, int code, const String& message);

  // Route handlers
  void _handleGetDevice(AsyncWebServerRequest* request);
  void _handleGetStatus(AsyncWebServerRequest* request);
  void _handleGetNetwork(AsyncWebServerRequest* request);
  void _handleGetSystem(AsyncWebServerRequest* request);
  void _handlePostPair(AsyncWebServerRequest* request);
  void _handlePostCommands(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void _handlePostNetworkConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void _handlePostRestart(AsyncWebServerRequest* request);
  void _handleNotFound(AsyncWebServerRequest* request);
};

#endif // WEB_SERVER_MANAGER_H
