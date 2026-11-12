#ifndef CROENIX_BT_INIT_H
#define CROENIX_BT_INIT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Bluetooth controller and Bluedroid stack in Dual Mode (Classic + BLE).
 * 
 * Sets up controller memory, initialises controller in ESP_BT_MODE_BTDM, initializes
 * Bluedroid stack, sets the device name to "CROENIX", and configures Classic GAP scan mode.
 * 
 * @return ESP_OK on success, or appropriate error code.
 */
esp_err_t croenix_bt_init(void);

#ifdef __cplusplus
}
#endif

#endif // CROENIX_BT_INIT_H

