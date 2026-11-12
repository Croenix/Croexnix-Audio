#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"

#include "croenix_a2dp.h"
#include "croenix_audio.h"
#include "croenix_ble.h"
#include "croenix_bt_config.h"

static const char *TAG = "CROENIX_A2DP";

// -------------------------------------------------------------------------
// Non-Blocking Audio Data Callback
// Called by the Bluedroid SBC decoder thread with raw stereo 16-bit PCM.
// -------------------------------------------------------------------------
void croenix_a2dp_data_cb(const uint8_t *data, uint32_t len) {
    if (data == NULL || len == 0) return;

    // Push into non-blocking ring buffer. NEVER do blocking I/O here!
    croenix_audio_write_ringbuf(data, len);
}

// -------------------------------------------------------------------------
// A2DP Event Callback Handler
// -------------------------------------------------------------------------
void croenix_a2dp_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
    switch (event) {
        case ESP_A2D_CONNECTION_STATE_EVT: {
            esp_a2d_cb_param_t *a2d = (esp_a2d_cb_param_t *)(param);
            uint8_t *bda = a2d->conn_stat.remote_bda;

            if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
                ESP_LOGI(TAG, "A2DP Connected to [%02X:%02X:%02X:%02X:%02X:%02X]",
                         bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

                g_croenix_state.a2dp_connected = true;
                memcpy(g_croenix_state.a2dp_peer_bda, bda, 6);
                snprintf(g_croenix_state.a2dp_peer_name, sizeof(g_croenix_state.a2dp_peer_name),
                         "Device %02X:%02X", bda[4], bda[5]);

                // Notify BLE companion app of connection status
                croenix_ble_notify_connection_status();
            } else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
                ESP_LOGI(TAG, "A2DP Disconnected");
                g_croenix_state.a2dp_connected = false;
                g_croenix_state.playback_state = CROENIX_PLAYBACK_STOPPED;
                memset(g_croenix_state.a2dp_peer_bda, 0, 6);
                g_croenix_state.a2dp_peer_name[0] = '\0';

                croenix_audio_flush_ringbuf();
                croenix_ble_notify_status();
                croenix_ble_notify_connection_status();
            } else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTING) {
                ESP_LOGI(TAG, "A2DP Connecting...");
            }
            break;
        }

        case ESP_A2D_AUDIO_STATE_EVT: {
            esp_a2d_cb_param_t *a2d = (esp_a2d_cb_param_t *)(param);
            if (a2d->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED) {
                ESP_LOGI(TAG, "A2DP Audio Streaming Started");
                g_croenix_state.playback_state = CROENIX_PLAYBACK_PLAYING;
            } else if (a2d->audio_stat.state == ESP_A2D_AUDIO_STATE_STOPPED) {
                ESP_LOGI(TAG, "A2DP Audio Streaming Stopped");
                g_croenix_state.playback_state = CROENIX_PLAYBACK_STOPPED;
                croenix_audio_flush_ringbuf();
            } else if (a2d->audio_stat.state == ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND) {
                ESP_LOGI(TAG, "A2DP Audio Suspended (Paused by phone)");
                g_croenix_state.playback_state = CROENIX_PLAYBACK_PAUSED;
                croenix_audio_flush_ringbuf();
            }

            // Sync with mobile app over BLE
            croenix_ble_notify_status();
            break;
        }

        case ESP_A2D_AUDIO_CFG_EVT: {
            esp_a2d_cb_param_t *a2d = (esp_a2d_cb_param_t *)(param);
            ESP_LOGI(TAG, "A2DP Audio Codec Config: type %d", a2d->audio_cfg.mcc.type);

            if (a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
                uint32_t sample_rate = 16000;
                char oct0 = a2d->audio_cfg.mcc.cie.sbc[0];

                if (oct0 & (0x01 << 6)) {
                    sample_rate = 32000;
                } else if (oct0 & (0x01 << 5)) {
                    sample_rate = 44100;
                } else if (oct0 & (0x01 << 4)) {
                    sample_rate = 48000;
                }

                ESP_LOGI(TAG, "Negotiated A2DP Sample Rate: %lu Hz", sample_rate);
                croenix_audio_set_sample_rate(sample_rate);
            }
            break;
        }

        case ESP_A2D_PROF_STATE_EVT: {
            ESP_LOGI(TAG, "A2DP Profile init/deinit complete");
            break;
        }

        default:
            ESP_LOGD(TAG, "Unhandled A2DP event: %d", event);
            break;
    }
}

// -------------------------------------------------------------------------
// Initialization Routine
// -------------------------------------------------------------------------
esp_err_t croenix_a2dp_init(void) {
    ESP_LOGI(TAG, "Initializing A2DP Sink...");

    esp_err_t err = esp_a2d_register_callback(croenix_a2dp_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_a2d_register_callback failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_a2d_sink_register_data_callback(croenix_a2dp_data_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_a2d_sink_register_data_callback failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_a2d_sink_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_a2d_sink_init failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "A2DP Sink initialized successfully");
    return ESP_OK;
}

