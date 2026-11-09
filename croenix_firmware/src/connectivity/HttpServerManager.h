#ifndef CROENIX_HTTP_SERVER_MANAGER_H
#define CROENIX_HTTP_SERVER_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class PlayerStateManager;
class AuthManager;
class CroenixWiFiManager;
class SystemMonitor;
class CommandManager;
class WebSocketManager;

class HttpServerManager {
public:
  HttpServerManager(PlayerStateManager* stateManager,
                    AuthManager* authManager,
                    CroenixWiFiManager* wifiManager,
                    SystemMonitor* systemMonitor,
                    CommandManager* commandManager);

  void begin();
  void update();
  AsyncWebServer* getServer();

private:
  AsyncWebServer _server;
  PlayerStateManager* _stateManager;
  AuthManager* _authManager;
  CroenixWiFiManager* _wifiManager;
  SystemMonitor* _systemMonitor;
  CommandManager* _commandManager;
  WebSocketManager* _wsManager;

  bool _isRequestAuthorized(AsyncWebServerRequest* request);
  void _sendJson(AsyncWebServerRequest* request, int code, const String& json);
  void _sendError(AsyncWebServerRequest* request, int code, const String& message, const String& errCode = "");

  // Endpoints
  void _handleGetDevice(AsyncWebServerRequest* request);
  void _handleGetStatus(AsyncWebServerRequest* request);
  void _handleGetNetwork(AsyncWebServerRequest* request);
  void _handleGetSystem(AsyncWebServerRequest* request);
  void _handlePostPair(AsyncWebServerRequest* request);
  void _handlePostCommands(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void _handlePostNetworkConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void _handlePostRestart(AsyncWebServerRequest* request);
};

#endif // CROENIX_HTTP_SERVER_MANAGER_H

