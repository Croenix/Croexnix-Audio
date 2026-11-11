#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <Arduino.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "config.h"

// Initialize WebSocket server
void initWebSocketServer();

// Update WebSocket server (called from network task)
void updateWebSocketServer();

// Send state updates to all connected clients
void broadcastDeviceState();
void broadcastMetadata();
void broadcastNetworkInfo();

// WebSocket event handler
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

// Command handlers
void handlePlayCommand(JsonDocument& doc);
void handlePauseCommand(JsonDocument& doc);
void handleStopCommand(JsonDocument& doc);
void handleVolumeCommand(JsonDocument& doc);
void handleSourceCommand(JsonDocument& doc);
void handleEQCommand(JsonDocument& doc);
void handleWiFiConfigCommand(JsonDocument& doc);
void handleScanNetworksCommand(JsonDocument& doc);
void handleFileListCommand(JsonDocument& doc);
void handleLoadFileCommand(JsonDocument& doc);
void handleFMFrequencyCommand(JsonDocument& doc);
void handleStreamURLCommand(JsonDocument& doc);

#endif // WEBSOCKET_SERVER_H
