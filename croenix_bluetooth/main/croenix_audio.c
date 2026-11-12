#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_log.h"
#include "esp_idf_version.h"

#include "croenix_audio.h"
#include "croenix_bt_config.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "driver/i2s_std.h"
static i2s_chan_handle_t s_tx_chan = NULL;
#else
#include "driver/i2s.h"
#endif

static const char *TAG = "CROENIX_AUDIO";

static RingbufHandle_t s_ringbuf = NULL;
static TaskHandle_t s_audio_task_handle = NULL;
static uint8_t s_current_volume = 75;
static uint32_t s_current_rate = DEFAULT_AUDIO_SAMPLE_RATE;
static bool s_audio_initialized = false;

// -------------------------------------------------------------------------
// Digital Volume Scaling for 16-Bit Stereo Samples
// -------------------------------------------------------------------------
static void apply_software_volume(int16_t *samples, size_t sample_count, uint8_t vol) {
    if (vol >= 100) return;
    if (vol == 0) {
        memset(samples, 0, sample_count * sizeof(int16_t));
        return;
    }

    // Fixed-point scaling (0 - 256)
    int32_t scale = (vol * 256) / 100;
    for (size_t i = 0; i < sample_count; i++) {
        int32_t val = ((int32_t)samples[i] * scale) >> 8;
        if (val > 32767) val = 32767;
        else if (val < -32768) val = -32768;
        samples[i] = (int16_t)val;
    }
}

// -------------------------------------------------------------------------
// Dedicated Audio Pipeline Task (Pinned to Core 1)
// -------------------------------------------------------------------------
static void audio_pipeline_task(void *pvParameters) {
    ESP_LOGI(TAG, "Audio DMA worker task started on Core %d", xPortGetCoreID());

    uint8_t local_buffer[1024];
    size_t bytes_received = 0;

    while (1) {
        // Wait for decoded PCM audio chunks from the ringbuffer
        uint8_t *data = (uint8_t *)xRingbufferReceiveUpTo(
            s_ringbuf,
            &bytes_received,
            pdMS_TO_TICKS(20),
            sizeof(local_buffer)
        );

        if (data != NULL && bytes_received > 0) {
            memcpy(local_buffer, data, bytes_received);
            vRingbufferReturnItem(s_ringbuf, (void *)data);

            // Apply software volume scaling
            apply_software_volume(
                (int16_t *)local_buffer,
                bytes_received / sizeof(int16_t),
                s_current_volume
            );

            // Write stereo PCM stream to I2S DMA
            size_t bytes_written = 0;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
            if (s_tx_chan != NULL) {
                i2s_channel_write(s_tx_chan, local_buffer, bytes_received, &bytes_written, portMAX_DELAY);
            }
#else
            i2s_write((i2s_port_t)CONFIG_I2S_NUM, local_buffer, bytes_received, &bytes_written, portMAX_DELAY);
#endif
        } else {
            // Buffer empty / idle - sleep briefly to prevent busy waiting
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

// -------------------------------------------------------------------------
// Public Hardware Initialization
// -------------------------------------------------------------------------
esp_err_t croenix_audio_init(void) {
    if (s_audio_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing PCM5102A DAC (BCK:%d, DIN:%d, LCK:%d)",
             CONFIG_I2S_BCK_PIN, CONFIG_I2S_DOUT_PIN, CONFIG_I2S_LRC_PIN);

    // 1. Create FreeRTOS Non-Blocking Ringbuffer
    s_ringbuf = xRingbufferCreate(CONFIG_AUDIO_RINGBUF_SIZE, RINGBUF_TYPE_BYTEBUF);
    if (s_ringbuf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate audio ringbuffer (%d bytes)", CONFIG_AUDIO_RINGBUF_SIZE);
        return ESP_ERR_NO_MEM;
    }

    // 2. Initialize Hardware I2S Port
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(CONFIG_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = CONFIG_I2S_DMA_BUF_COUNT;
    chan_cfg.dma_frame_num = CONFIG_I2S_DMA_BUF_LEN;
    chan_cfg.auto_clear = true;

    esp_err_t err = i2s_new_channel(&chan_cfg, &s_tx_chan, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(err));
        return err;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(DEFAULT_AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)CONFIG_I2S_BCK_PIN,
            .ws   = (gpio_num_t)CONFIG_I2S_LRC_PIN,
            .dout = (gpio_num_t)CONFIG_I2S_DOUT_PIN,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(s_tx_chan, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2s_channel_enable(s_tx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(err));
        return err;
    }
#else
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = DEFAULT_AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = CONFIG_I2S_DMA_BUF_COUNT,
        .dma_buf_len = CONFIG_I2S_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = CONFIG_I2S_BCK_PIN,
        .ws_io_num = CONFIG_I2S_LRC_PIN,
        .data_out_num = CONFIG_I2S_DOUT_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install((i2s_port_t)CONFIG_I2S_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2s_set_pin((i2s_port_t)CONFIG_I2S_NUM, &pin_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_set_pin failed: %s", esp_err_to_name(err));
        return err;
    }
#endif

    // 3. Launch Audio Worker Task on Core 1
    BaseType_t task_created = xTaskCreatePinnedToCore(
        audio_pipeline_task,
        "crx_audio_task",
        CONFIG_AUDIO_TASK_STACK_SIZE,
        NULL,
        CONFIG_AUDIO_TASK_PRIORITY,
        &s_audio_task_handle,
        CONFIG_AUDIO_TASK_CORE
    );

    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio worker task");
        return ESP_FAIL;
    }

    s_audio_initialized = true;
    ESP_LOGI(TAG, "Audio subsystem initialized successfully");
    return ESP_OK;
}

// -------------------------------------------------------------------------
// Dynamic Sample Rate Negotiation
// -------------------------------------------------------------------------
esp_err_t croenix_audio_set_sample_rate(uint32_t rate) {
    if (rate == 0 || rate == s_current_rate) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Reconfiguring DAC sample rate: %lu Hz -> %lu Hz", s_current_rate, rate);
    s_current_rate = rate;
    g_croenix_state.sample_rate = rate;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    if (s_tx_chan != NULL) {
        i2s_channel_disable(s_tx_chan);
        i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate);
        i2s_channel_reconfig_std_clock(s_tx_chan, &clk_cfg);
        i2s_channel_enable(s_tx_chan);
    }
#else
    i2s_set_sample_rates((i2s_port_t)CONFIG_I2S_NUM, rate);
#endif

    return ESP_OK;
}

// -------------------------------------------------------------------------
// Non-Blocking Ringbuffer Write (Called from A2DP data callback)
// -------------------------------------------------------------------------
size_t croenix_audio_write_ringbuf(const uint8_t *data, size_t size) {
    if (s_ringbuf == NULL || data == NULL || size == 0) {
        return 0;
    }

    // Must be completely non-blocking (timeout = 0)
    BaseType_t res = xRingbufferSend(s_ringbuf, data, size, 0);
    if (res != pdTRUE) {
        // Buffer full - drop frame cleanly without stalling Bluetooth task
        g_croenix_state.audio_underruns++;
        return 0;
    }

    return size;
}

void croenix_audio_set_volume(uint8_t volume) {
    if (volume > 100) volume = 100;
    s_current_volume = volume;
    g_croenix_state.volume = volume;
    ESP_LOGD(TAG, "Volume set to %d%%", volume);
}

uint8_t croenix_audio_get_volume(void) {
    return s_current_volume;
}

void croenix_audio_flush_ringbuf(void) {
    if (s_ringbuf == NULL) return;

    size_t size = 0;
    while (1) {
        uint8_t *item = (uint8_t *)xRingbufferReceiveUpTo(s_ringbuf, &size, 0, 1024);
        if (item == NULL) break;
        vRingbufferReturnItem(s_ringbuf, (void *)item);
    }
}

