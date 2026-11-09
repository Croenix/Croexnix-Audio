/*
 * CROENIX Music Player Firmware
 * Phase 1 & 2 - Network Foundation
 * Version: 1.0.0
 *
 * Description:
 * ESP32-based lossless music player with Android remote control.
 * This phase implements AP+STA WiFi, HTTP REST API, authentication,
 * and device discovery for Android app integration.
 *
 * Required Libraries (install via Arduino Library Manager):
 * - ArduinoJson (v7.x)
 * - ESPAsyncWebServer
 * - AsyncTCP
 *
 * Hardware:
 * - ESP32-WROOM-32 (38-pin NodeMCU development board)
 *
 * Author: Croenix Project
 * Date: 2026-11-09
 */

#include "Config.h"
#include "StateManager.h"
#include "WiFiManager.h"
#include "AuthManager.h"
#include "SystemMonitor.h"
#include "DiscoveryManager.h"
#include "CommandManager.h"
#include "WebServerManager.h"

// Manager instances
StateManager stateManager;
WiFiManager wifiManager;
AuthManager authManager;
SystemMonitor systemMonitor;
DiscoveryManager discoveryManager;
CommandManager commandManager(&stateManager);
WebServerManager webServerManager(&stateManager, &authManager, &wifiManager,
                                  &systemMonitor, &commandManager);

void setup() {
  // Initialize serial for debugging
  if (DEBUG_SERIAL) {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);
    Serial.println();
    Serial.println("========================================");
    Serial.println("       CROENIX Music Player v1.0.0");
    Serial.println("     Phase 1/2 - Network Foundation");
    Serial.println("========================================");
    Serial.println();
  }

  // Initialize managers in order
  DEBUG_PRINTLN("[Main] Initializing State Manager...");
  stateManager.begin();

  DEBUG_PRINTLN("[Main] Initializing System Monitor...");
  systemMonitor.begin();

  DEBUG_PRINTLN("[Main] Initializing Authentication Manager...");
  authManager.begin();

  DEBUG_PRINTLN("[Main] Initializing WiFi Manager...");
  wifiManager.begin();

  DEBUG_PRINTLN("[Main] Initializing Discovery Manager...");
  discoveryManager.begin();

  // Wait for WiFi AP to stabilize
  delay(1000);

  // Start mDNS after WiFi is ready
  if (wifiManager.isAPActive() || wifiManager.isConnectedToRouter()) {
    DEBUG_PRINTLN("[Main] Starting mDNS...");
    discoveryManager.startMDNS(DEVICE_HOSTNAME, FIRMWARE_VERSION);
  }

  DEBUG_PRINTLN("[Main] Starting Web Server...");
  webServerManager.begin();

  // Print connection info
  Serial.println();
  Serial.println("========================================");
  Serial.println("        SYSTEM READY");
  Serial.println("========================================");

  if (wifiManager.isAPActive()) {
    Serial.println("Access Point Active:");
    Serial.print("  SSID: ");
    Serial.println(AP_SSID);
    Serial.print("  IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.println();
  }

  if (wifiManager.isConnectedToRouter()) {
    Serial.println("Connected to Router:");
    Serial.print("  SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("  IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("  Hostname: ");
    Serial.print(DEVICE_HOSTNAME);
    Serial.println(".local");
    Serial.println();
  }

  if (authManager.isPairingModeActive()) {
    Serial.println("⚠️  PAIRING MODE ACTIVE");
    Serial.println("   Waiting for pairing request...");
    Serial.println("   (Times out after 30 seconds)");
    Serial.println();
  }

  Serial.println("API Endpoints:");
  Serial.println("  GET  /api/device");
  Serial.println("  GET  /api/status");
  Serial.println("  GET  /api/network");
  Serial.println("  GET  /api/system");
  Serial.println("  POST /api/pair");
  Serial.println("  POST /api/commands");
  Serial.println("  POST /api/network/config");
  Serial.println("  POST /api/device/restart");
  Serial.println("========================================");
  Serial.println();
}

void loop() {
  // Update managers
  wifiManager.update();
  systemMonitor.update();
  discoveryManager.update();
  stateManager.update();

  // Check for encoder press (placeholder for Phase 3+)
  // In Phase 3, this will enable pairing mode when encoder is pressed
  // For now, pairing mode is auto-enabled if no token exists

  // Small delay to prevent watchdog triggers
  delay(10);
}
