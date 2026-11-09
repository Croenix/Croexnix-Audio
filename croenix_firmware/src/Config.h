#ifndef CROENIX_CONFIG_H
#define CROENIX_CONFIG_H

#include <Arduino.h>
#include <IPAddress.h>
#include "Version.h"
#include "PinConfig.h"

// ==============================================================================
// CROENIX FIRMWARE MASTER CONFIGURATION
// ==============================================================================

// ------------------------------------------------------------------------------
// Device Identity
// ------------------------------------------------------------------------------
#define DEVICE_NAME             CROENIX_DEVICE_NAME
#define FIRMWARE_VERSION        CROENIX_FIRMWARE_VERSION
#define DEVICE_HOSTNAME         CROENIX_DEFAULT_HOSTNAME
#define HARDWARE_IDENTIFIER     CROENIX_HARDWARE_TARGET

// ------------------------------------------------------------------------------
// Network Configuration (AP + STA)
// ------------------------------------------------------------------------------
#define AP_SSID_DEFAULT         "Croenix-Audio"
#define AP_PASSWORD_DEFAULT     "12345678"
#define AP_CHANNEL_DEFAULT      1
#define AP_MAX_CONNECTIONS      4
#define AP_IP_ADDRESS           IPAddress(192, 168, 4, 1)
#define AP_GATEWAY              IPAddress(192, 168, 4, 1)
#define AP_SUBNET               IPAddress(255, 255, 255, 0)

// STA Timing & Backoff
#define WIFI_CONNECT_TIMEOUT_MS         10000
#define WIFI_RECONNECT_INTERVAL_BASE_MS 5000
#define WIFI_RECONNECT_INTERVAL_MAX_MS  60000
#define WIFI_STATUS_POLL_MS             1000

// ------------------------------------------------------------------------------
// Web Server & WebSocket
// ------------------------------------------------------------------------------
#define HTTP_PORT                       80
#define WS_ENDPOINT                     "/ws"
#define MAX_FAILED_AUTH_ATTEMPTS        10
#define AUTH_LOCKOUT_DURATION_MS        60000
#define PAIRING_TIMEOUT_MS              30000
#define MAX_JSON_PAYLOAD_SIZE           1024

// ------------------------------------------------------------------------------
// Discovery Protocols
// ------------------------------------------------------------------------------
#define MDNS_SERVICE_NAME       "_croenix"
#define MDNS_PROTOCOL           "_tcp"
#define UDP_DISCOVERY_PORT      5353
#define DISCOVERY_MAGIC         "CROENIX_DISCOVER"
#define DISCOVERY_RESPONSE      "CROENIX_HERE"

// ------------------------------------------------------------------------------
// Bluetooth Low Energy (BLE) Identity & GATT UUIDs
// ------------------------------------------------------------------------------
#define BLE_DEVICE_NAME         "Croenix"
#define BLE_SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define BLE_CHAR_DEVICE_INFO    "19b10001-e8f2-537e-4f6c-d104768a1214"
#define BLE_CHAR_PROVISIONING   "19b10002-e8f2-537e-4f6c-d104768a1214"
#define BLE_CHAR_NETWORK_STATUS "19b10003-e8f2-537e-4f6c-d104768a1214"
#define BLE_CHAR_PAIRING        "19b10004-e8f2-537e-4f6c-d104768a1214"
#define BLE_CHAR_EVENTS         "19b10005-e8f2-537e-4f6c-d104768a1214"

// ------------------------------------------------------------------------------
// Non-Volatile Storage (Preferences / NVS)
// ------------------------------------------------------------------------------
#define NVS_NAMESPACE           "croenix"
#define NVS_KEY_AUTH_TOKEN      "auth_token"
#define NVS_KEY_WIFI_SSID       "wifi_ssid"
#define NVS_KEY_WIFI_PASSWORD   "wifi_pass"
#define NVS_KEY_DEVICE_NAME     "dev_name"
#define NVS_KEY_VOLUME          "vol"
#define NVS_KEY_MUTED           "muted"

// ------------------------------------------------------------------------------
// System Diagnostics & Monitoring
// ------------------------------------------------------------------------------
#define SYSTEM_MONITOR_INTERVAL_MS      5000
#define HEAP_WARNING_THRESHOLD_BYTES    3500

// ------------------------------------------------------------------------------
// Audio Engine Defaults
// ------------------------------------------------------------------------------
#define DEFAULT_VOLUME                  50
#define MAX_VOLUME                      100
#define I2S_SAMPLE_RATE                 44100
#define I2S_BIT_DEPTH                   16

// ------------------------------------------------------------------------------
// Debugging
// ------------------------------------------------------------------------------
#define DEBUG_SERIAL_ENABLED    true
#define SERIAL_BAUD_RATE        115200

#if DEBUG_SERIAL_ENABLED
  #define CROENIX_LOG(tag, fmt, ...) Serial.printf("[%s] " fmt "\n", tag, ##__VA_ARGS__)
  #define CROENIX_LOG_RAW(msg) Serial.print(msg)
#else
  #define CROENIX_LOG(tag, fmt, ...)
  #define CROENIX_LOG_RAW(msg)
#endif

#endif // CROENIX_CONFIG_H

