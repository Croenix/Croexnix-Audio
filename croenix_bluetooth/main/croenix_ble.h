#ifndef CROENIX_BLE_H
#define CROENIX_BLE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_gatts_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize BLE GATT Server and start advertising for mobile app discovery.
 * @return ESP_OK on success.
 */
esp_err_t croenix_ble_init(void);

/**
 * @brief Send real-time playback state and ID3 metadata notifications to connected BLE app.
 */
void croenix_ble_notify_status(void);

/**
 * @brief Send connection telemetry notifications (A2DP status, peer name) to BLE app.
 */
void croenix_ble_notify_connection_status(void);

/**
 * @brief Parse and execute incoming remote control command string from BLE app.
 * @param cmd Null-terminated or length-delimited command buffer.
 * @param len Buffer length.
 */
void croenix_ble_handle_command(const char *cmd, size_t len);

#ifdef __cplusplus
}
#endif

#endif // CROENIX_BLE_H

