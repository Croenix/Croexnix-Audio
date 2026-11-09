#include "WebSocketManager.h"
#include "../core/PlayerStateManager.h"
#include "../core/EventManager.h"
#include "../core/CommandManager.h"
#include "AuthManager.h"
#include "../Config.h"
#include <ArduinoJson.h>

WebSocketManager::WebSocketManager(AsyncWebServer* server, PlayerStateManager* stateManager,
                                   AuthManager* authManager, CommandManager* commandManager)
  : _server(server),
    _ws(WS_ENDPOINT),
    _stateManager(stateManager),
    _authManager(authManager),
    _commandManager(commandManager) {
}

void WebSocketManager::begin() {
  _ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                     AwsEventType type, void* arg, uint8_t* data, size_t len) {
    _onEvent(server, client, type, arg, data, len);
  });

  _server->addHandler(&_ws);
  CROENIX_LOG("WebSocketManager", "WebSocket endpoint registered at %s", WS_ENDPOINT);

  // Subscribe to central EventManager
  EventManager::getInstance().addListener([this](const CroenixEvent& ev) {
    if (_ws.count() > 0) {
      broadcastEvent(ev.name, ev.payloadJson, ev.sequenceNumber, ev.timestamp);
    }
  });
}

void WebSocketManager::update() {
  _ws.cleanupClients();
}

size_t WebSocketManager::getConnectedClientCount() {
  return _ws.count();
}

void WebSocketManager::_onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT: {
      IPAddress ip = client->remoteIP();
      CROENIX_LOG("WebSocketManager", "Client connected: #%u from %s", client->id(), ip.toString().c_str());
      _sendStateSnapshot(client);
      break;
    }
    case WS_EVT_DISCONNECT:
      CROENIX_LOG("WebSocketManager", "Client disconnected: #%u", client->id());
      break;

    case WS_EVT_DATA: {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        String msg = "";
        msg.reserve(len);
        for (size_t i = 0; i < len; i++) {
          msg += (char)data[i];
        }
        _handleTextMessage(client, msg);
      }
      break;
    }
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void WebSocketManager::_handleTextMessage(AsyncWebSocketClient* client, const String& message) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, message);
  if (err) {
    client->text("{\"error\":\"INVALID_JSON\"}");
    return;
  }

  // Handle command execution over WebSocket
  if (doc["command"].is<String>()) {
    String command = doc["command"].as<String>();
    String reqId = doc["request_id"].is<String>() ? doc["request_id"].as<String>() : "";
    JsonDocument params = doc["params"];

    // Check optional token if provided
    if (doc["token"].is<String>()) {
      if (!_authManager->validateToken(doc["token"].as<String>())) {
        client->text("{\"error\":\"UNAUTHORIZED\"}");
        return;
      }
    }

    if (_commandManager) {
      CommandExecutionResult res = _commandManager->execute(command, params, reqId);
      client->text(res.toJson());
    }
  } else if (doc["action"].is<String>()) {
    String action = doc["action"].as<String>();
    if (action == "get_state") {
      _sendStateSnapshot(client);
    }
  }
}

void WebSocketManager::_sendStateSnapshot(AsyncWebSocketClient* client) {
  if (!_stateManager || !client) return;

  JsonDocument doc;
  doc["event"] = "state_snapshot";
  doc["schema_version"] = "1.0.0";
  doc["timestamp"] = millis();

  JsonDocument stateDoc;
  deserializeJson(stateDoc, _stateManager->toJson());
  doc["state"] = stateDoc;

  String out;
  serializeJson(doc, out);
  client->text(out);
}

void WebSocketManager::broadcastEvent(const String& eventName, const String& payloadJson, uint32_t seq, unsigned long timestamp) {
  if (_ws.count() == 0) return;

  JsonDocument doc;
  doc["event"] = eventName;
  doc["seq"] = seq;
  doc["timestamp"] = timestamp;

  if (!payloadJson.isEmpty()) {
    JsonDocument pDoc;
    deserializeJson(pDoc, payloadJson);
    doc["payload"] = pDoc;
  }

  String out;
  serializeJson(doc, out);
  _ws.textAll(out);
}

