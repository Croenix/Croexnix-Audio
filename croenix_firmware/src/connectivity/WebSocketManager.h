#ifndef CROENIX_WEBSOCKET_MANAGER_H
#define CROENIX_WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class PlayerStateManager;
class AuthManager;
class CommandManager;

class WebSocketManager {
public:
  WebSocketManager(AsyncWebServer* server, PlayerStateManager* stateManager,
                   AuthManager* authManager, CommandManager* commandManager);
  void begin();
  void update();

  void broadcastEvent(const String& eventName, const String& payloadJson, uint32_t seq, unsigned long timestamp);
  size_t getConnectedClientCount();

private:
  AsyncWebServer* _server;
  AsyncWebSocket _ws;
  PlayerStateManager* _stateManager;
  AuthManager* _authManager;
  CommandManager* _commandManager;

  void _onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len);
  void _handleTextMessage(AsyncWebSocketClient* client, const String& message);
  void _sendStateSnapshot(AsyncWebSocketClient* client);
};

#endif // CROENIX_WEBSOCKET_MANAGER_H

