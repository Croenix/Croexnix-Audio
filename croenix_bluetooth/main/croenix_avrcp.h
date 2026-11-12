#ifndef CROENIX_AVRCP_H
#define CROENIX_AVRCP_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_avrc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize AVRCP Target & Controller subsystem.
 * @return ESP_OK on success.
 */
esp_err_t croenix_avrcp_init(void);

/**
 * @brief Send AVRCP passthrough command to connected phone (Play, Pause, Next, Prev).
 */
void croenix_avrcp_play(void);
void croenix_avrcp_pause(void);
void croenix_avrcp_next(void);
void croenix_avrcp_previous(void);
void croenix_avrcp_set_volume(uint8_t volume);

#ifdef __cplusplus
}
#endif

#endif // CROENIX_AVRCP_H

