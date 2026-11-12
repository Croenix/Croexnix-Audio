#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_heap_caps.h"

#include "croenix_bt_config.h"
#include "croenix_audio.h"
#include "croenix_bt_init.h"
#include "croenix_a2dp.h"
#include "croenix_avrcp.h"
#include "croenix_ble.h"

static const char *TAG = "CROENIX_MAIN";

/**
 * @brief System telemetry and health monitor task.
 * Periodically logs memory headroom, audio buffer health, and connection states.
 */
static void croenix_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Croenix Telemetry Monitor running on Core %d", xPortGetCoreID());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // Every 10 seconds

        uint32_t free_heap = esp_get_free_heap_size();
        uint32_t min_free_heap = esp_get_minimum_free_heap_size();
        uint32_t dma_heap = heap_caps_get_free_size(MALLOC_CAP_DMA);

        ESP_LOGI(TAG, "--- [CROENIX DUAL-MODE HEALTH] ---");
        ESP_LOGI(TAG, "Free Heap: %" PRIu32 " B | Min Heap: %" PRIu32 " B | DMA Heap: %" PRIu32 " B",
                 free_heap, min_free_heap, dma_heap);
        ESP_LOGI(TAG, "Audio: %s | Sample Rate: %d Hz | Vol: %d%%",
                 croenix_audio_is_active() ? "PLAYING" : "IDLE",
                 croenix_audio_get_sample_rate(),
                 croenix_audio_get_volume());
        ESP_LOGI(TAG, "---------------------------------");
    }
}

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "   CROENIX DUAL-MODE BLUETOOTH MUSIC RECEIVER   ");
    ESP_LOGI(TAG, "       A2DP Sink + BLE Companion Control         ");
    ESP_LOGI(TAG, "          ESP32-WROOM-32 / ESP-IDF 5.x           ");
    ESP_LOGI(TAG, "=================================================");

    // 1. Initialize NVS (required for Bluetooth bonding and calibration storage)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated or updated; erasing and reinitializing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "[1/5] NVS Flash initialized successfully");

    // 2. Initialize PCM5102 I2S Audio Pipeline (Core 1, DMA Ringbuffer)
    ret = croenix_audio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fatal: Audio subsystem initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "[2/5] PCM5102 I2S Audio Subsystem initialized (BCK: %d, DIN: %d, LCK: %d)",
             CONFIG_I2S_BCK_PIN, CONFIG_I2S_DOUT_PIN, CONFIG_I2S_LRC_PIN);

    // 3. Initialize Bluetooth Dual-Mode Controller & Bluedroid Stack
    ret = croenix_bt_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fatal: Bluetooth Core initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "[3/5] Dual-Mode Controller & Bluedroid Stack initialized");

    // 4. Initialize Bluetooth Classic A2DP Sink & AVRCP Target/Controller
    ret = croenix_a2dp_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fatal: A2DP Sink initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = croenix_avrcp_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fatal: AVRCP initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "[4/5] Bluetooth Classic A2DP Sink & AVRCP profiles registered");

    // 5. Initialize BLE GATT Server for Croenix Mobile Application
    ret = croenix_ble_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fatal: BLE GATT Server initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "[5/5] BLE GATT Server and Fast Advertising initialized");

    // 6. Launch supervisory telemetry task
    xTaskCreatePinnedToCore(
        croenix_monitor_task,
        "croenix_mon",
        3072,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL,
        0
    );

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, " CROENIX READY: Discoverable as '%s'", CROENIX_DEVICE_NAME);
    ESP_LOGI(TAG, " - Pair phone via Bluetooth settings for Audio");
    ESP_LOGI(TAG, " - Open Croenix App for instant BLE telemetry");
    ESP_LOGI(TAG, "=================================================");
}

