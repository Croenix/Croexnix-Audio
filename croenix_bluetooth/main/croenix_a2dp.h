#ifndef CROENIX_A2DP_H
#define CROENIX_A2DP_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_a2dp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Bluetooth Classic A2DP Sink subsystem.
 * @return ESP_OK on success.
 */
esp_err_t croenix_a2dp_init(void);

/**
 * @brief Main A2DP Sink controller callback.
 */
void croenix_a2dp_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

/**
 * @brief Decoded PCM audio data callback from the Bluedroid SBC decoder.
 * Pushes raw stereo PCM audio directly to the FreeRTOS audio ringbuffer.
 */
void croenix_a2dp_data_cb(const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // CROENIX_A2DP_H

