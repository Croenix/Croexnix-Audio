/*
 * CROENIX ESP32 FIRMWARE — FULL ANDROID APP INTEGRATION
 * Target Controller: ESP32-WROOM-32 (38-pin NodeMCU)
 *
 * Capabilities:
 * - Simultaneous AP + STA Wi-Fi (Croenix-Audio Hotspot + Router association)
 * - BLE Provisioning, Device Identification, and Setup/Recovery
 * - Real-Time WebSocket (/ws) Event Streaming & Command Dispatch
 * - 8 REST Endpoints 100% Compatible with Flutter Croenix App
 * - Bearer Token Authentication, Rate Limiting, Physical Pairing Confirmation
 * - Authoritative Thread-Safe PlayerStateManager
 * - KY-040 Rotary Encoder & 1.8" SPI TFT Display HAL
 * - I2S CJMCU-5102 DAC Audio Engine & Extensible Source Architecture
 */

#include "src/Config.h"
#include "src/PinConfig.h"
#include "src/Version.h"

// Core & Storage
#include "src/storage/SettingsManager.h"
#include "src/core/PlayerStateManager.h"
#include "src/core/EventManager.h"
#include "src/core/CommandManager.h"
#include "src/diagnostics/SystemMonitor.h"
#include "src/diagnostics/ErrorManager.h"

// Connectivity
#include "src/connectivity/AuthManager.h"
#include "src/connectivity/CroenixWiFiManager.h"
#include "src/connectivity/DiscoveryManager.h"
#include "src/connectivity/HttpServerManager.h"
#include "src/connectivity/CroenixBLEManager.h"

// Hardware & Audio
#include "src/hardware/RotaryEncoder.h"
#include "src/hardware/DisplayManager.h"
#include "src/hardware/SDManager.h"
#include "src/hardware/StatusLed.h"
#include "src/audio/AudioManager.h"

// Static Module Instances
SettingsManager     settingsManager;
PlayerStateManager  stateManager;
SystemMonitor       systemMonitor(&stateManager);
AuthManager         authManager(&settingsManager);
CroenixWiFiManager  wifiManager(&settingsManager, &stateManager);
CommandManager      commandManager(&stateManager, &wifiManager);
DiscoveryManager    discoveryManager;
HttpServerManager   httpServerManager(&stateManager, &authManager, &wifiManager, &systemMonitor, &commandManager);
CroenixBLEManager   bleManager(&stateManager, &authManager, &wifiManager, &settingsManager);
RotaryEncoder       rotaryEncoder(&stateManager, &authManager);
DisplayManager      displayManager(&stateManager);
SDManager           sdManager(&stateManager);
StatusLed           statusLed(&wifiManager, &systemMonitor);
AudioManager        audioManager(&stateManager);

void setup() {
  if (DEBUG_SERIAL_ENABLED) {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(500);
    Serial.println();
    Serial.println("==================================================");
    Serial.printf("          CROENIX FIRMWARE v%s\n", CROENIX_FIRMWARE_VERSION);
    Serial.printf("          Target: %s\n", CROENIX_HARDWARE_TARGET);
    Serial.println("   Dual AP+STA Wi-Fi / BLE / WebSocket / Audio   ");
    Serial.println("==================================================");
    Serial.println();
  }

  // 1. Settings & Persistent Storage
  CROENIX_LOG("Setup", "Initializing Storage...");
  settingsManager.begin();

  // 2. Central State Manager
  CROENIX_LOG("Setup", "Initializing PlayerStateManager...");
  stateManager.begin();

  // 3. Diagnostics & Monitoring
  CROENIX_LOG("Setup", "Initializing System Diagnostics...");
  systemMonitor.begin();

  // 4. Security & Authentication
  CROENIX_LOG("Setup", "Initializing Authentication Engine...");
  authManager.begin();

  // 5. Simultaneous Wi-Fi AP + STA
  CROENIX_LOG("Setup", "Initializing Wi-Fi Subsystem (AP + STA)...");
  wifiManager.begin();

  // 6. Discovery Services (mDNS & UDP broadcast)
  CROENIX_LOG("Setup", "Initializing Discovery Manager...");
  discoveryManager.begin();
  discoveryManager.startMDNS(DEVICE_HOSTNAME, FIRMWARE_VERSION);

  // 7. Asynchronous REST API & WebSocket Server
  CROENIX_LOG("Setup", "Starting HTTP REST API & WebSocket Engine...");
  httpServerManager.begin();

  // 8. Bluetooth Low Energy (BLE) Provisioning & Recovery
  CROENIX_LOG("Setup", "Starting BLE Setup & Recovery Engine...");
  bleManager.begin();

  // 9. Physical Controls (KY-040 Rotary Encoder)
  CROENIX_LOG("Setup", "Initializing KY-040 Rotary Encoder...");
  rotaryEncoder.begin();

  // 10. Display Subsystem (1.8\" SPI TFT)
  CROENIX_LOG("Setup", "Initializing TFT Screen Manager...");
  displayManager.begin();

  // 11. Storage & SD Interface
  CROENIX_LOG("Setup", "Initializing MicroSD Card Interface...");
  sdManager.begin();

  // 12. Audio Subsystem (I2S CJMCU-5102 DAC)
  CROENIX_LOG("Setup", "Initializing I2S Audio Pipeline...");
  audioManager.begin();

  // 13. Power / Status LED Engine
  CROENIX_LOG("Setup", "Initializing Power & Status LED Engine...");
  statusLed.begin();

  // Print system summary
  Serial.println();
  Serial.println("--------------------------------------------------");
  Serial.println(" CROENIX IS READY");
  Serial.printf(" Device ID:      %s\n", stateManager.getDeviceId().c_str());
  Serial.printf(" Access Point:   %s (IP: 192.168.4.1)\n", AP_SSID_DEFAULT);
  if (wifiManager.isSTAConnected()) {
    Serial.printf(" Router Connected: %s (IP: %s)\n", wifiManager.getSTASSID().c_str(), wifiManager.getSTAIP().c_str());
  } else {
    Serial.println(" Router Status:  Standing by (reconnecting / unconfigured)");
  }
  Serial.printf(" BLE Advertising: Croenix (%s)\n", BLE_SERVICE_UUID);
  Serial.printf(" REST Endpoints: 8 active on port %d\n", HTTP_PORT);
  Serial.printf(" WebSocket:      ws://<ip>:%d%s\n", HTTP_PORT, WS_ENDPOINT);
  Serial.printf(" Free Heap:      %lu bytes\n", esp_get_free_heap_size());
  Serial.println("--------------------------------------------------");
  Serial.println();
}

void loop() {
  // Update network & connectivity
  wifiManager.update();
  systemMonitor.update();
  discoveryManager.update();
  authManager.update();
  stateManager.update();
  httpServerManager.update();
  bleManager.update();

  // Update hardware & controls
  rotaryEncoder.update();
  displayManager.update();
  sdManager.update();
  audioManager.update();
  statusLed.update();

  // Cooperative yield
  delay(5);
}

