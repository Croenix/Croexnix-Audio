#ifndef CROENIX_AUDIO_H
#define CROENIX_AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the PCM5102A I2S driver and the FreeRTOS audio ringbuffer.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t croenix_audio_init(void);

/**
 * @brief Reconfigure the I2S sample rate dynamically based on A2DP negotiation.
 * @param rate Target sample rate in Hz (e.g. 44100, 48000).
 * @return ESP_OK on success.
 */
esp_err_t croenix_audio_set_sample_rate(uint32_t rate);

/**
 * @brief Push raw decoded PCM audio data into the non-blocking ringbuffer.
 * Called directly from the A2DP sink data callback. Must be strictly non-blocking!
 * @param data Pointer to stereo 16-bit PCM bytes.
 * @param size Number of bytes to push.
 * @return Number of bytes actually written.
 */
size_t croenix_audio_write_ringbuf(const uint8_t *data, size_t size);

/**
 * @brief Adjust master output volume with digital headroom protection.
 * @param volume Integer level from 0 (mute) to 100 (maximum).
 */
void croenix_audio_set_volume(uint8_t volume);

/**
 * @brief Get current volume level.
 * @return Level 0 - 100.
 */
uint8_t croenix_audio_get_volume(void);

/**
 * @brief Clear any remaining audio in the ringbuffer (e.g., on track skip or pause).
 */
void croenix_audio_flush_ringbuf(void);

#ifdef __cplusplus
}
#endif

#endif // CROENIX_AUDIO_H

