#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

// BLE Service & Characteristic UUIDs (Matching Croenix Dual-Mode Architecture)
#define CROENIX_BLE_SERVICE_UUID        "000000ff-0000-1000-8000-00805f9b34fb"
#define CROENIX_BLE_CHAR_CMD_UUID       "0000ff01-0000-1000-8000-00805f9b34fb"
#define CROENIX_BLE_CHAR_STATUS_UUID    "0000ff02-0000-1000-8000-00805f9b34fb"
#define CROENIX_BLE_CHAR_INFO_UUID      "0000ff03-0000-1000-8000-00805f9b34fb"

/**
 * @brief Initialize BLE GATT Server and begin advertising as "CROENIX"
 */
void initBLEManager();

/**
 * @brief Periodic BLE update handler (state notifications & keepalive)
 */
void updateBLEManager();

/**
 * @brief Broadcast current device state & telemetry to connected BLE mobile app
 */
void broadcastBLEStatus();

/**
 * @brief Check if mobile app is currently connected via BLE
 */
bool isBLEConnected();

#endif // BLE_MANAGER_H

