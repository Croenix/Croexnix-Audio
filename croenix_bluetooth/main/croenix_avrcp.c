#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_avrc_api.h"

#include "croenix_avrcp.h"
#include "croenix_audio.h"
#include "croenix_ble.h"
#include "croenix_bt_config.h"

static const char *TAG = "CROENIX_AVRCP";

// -------------------------------------------------------------------------
// Target Callback (Handles remote control events from phone)
// -------------------------------------------------------------------------
static void croenix_avrc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param) {
    switch (event) {
        case ESP_AVRC_TG_CONNECTION_STATE_EVT: {
            uint8_t *bda = param->conn_stat.remote_bda;
            ESP_LOGI(TAG, "AVRCP Target connection state: %d [%02X:%02X:%02X:%02X:%02X:%02X]",
                     param->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
            break;
        }

        case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: {
            // Volume percentage 0-100 derived from 0-127 AVRCP scale
            uint8_t vol_pct = (param->set_abs_vol.volume * 100) / 127;
            ESP_LOGI(TAG, "Phone adjusted volume: %d%%", vol_pct);
            croenix_audio_set_volume(vol_pct);
            croenix_ble_notify_status();
            break;
        }

        case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT: {
            ESP_LOGI(TAG, "AVRCP Passthrough command received: key_code=0x%x, state=%d",
                     param->psth_cmd.key_code, param->psth_cmd.key_state);
            break;
        }

        default:
            ESP_LOGD(TAG, "Unhandled AVRCP TG event: %d", event);
            break;
    }
}

// -------------------------------------------------------------------------
// Controller Callback (Receives track metadata from phone: Title, Artist, Album)
// -------------------------------------------------------------------------
static void croenix_avrc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param) {
    switch (event) {
        case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
            if (param->conn_stat.connected) {
                ESP_LOGI(TAG, "AVRCP Controller connected - requesting track metadata...");
                esp_avrc_ct_send_metadata_cmd(
                    0,
                    ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM
                );
            }
            break;
        }

        case ESP_AVRC_CT_METADATA_RSP_EVT: {
            uint8_t attr_id = param->meta_rsp.attr_id;
            uint8_t *text = param->meta_rsp.attr_text;
            uint16_t len = param->meta_rsp.attr_length;

            if (text != NULL && len > 0) {
                char clean_buf[128];
                int copy_len = (len < sizeof(clean_buf) - 1) ? len : sizeof(clean_buf) - 1;
                memcpy(clean_buf, text, copy_len);
                clean_buf[copy_len] = '\0';

                if (attr_id == ESP_AVRC_MD_ATTR_TITLE) {
                    strncpy(g_croenix_state.current_title, clean_buf, sizeof(g_croenix_state.current_title) - 1);
                    ESP_LOGI(TAG, "Now Playing Title: %s", clean_buf);
                } else if (attr_id == ESP_AVRC_MD_ATTR_ARTIST) {
                    strncpy(g_croenix_state.current_artist, clean_buf, sizeof(g_croenix_state.current_artist) - 1);
                    ESP_LOGI(TAG, "Now Playing Artist: %s", clean_buf);
                } else if (attr_id == ESP_AVRC_MD_ATTR_ALBUM) {
                    strncpy(g_croenix_state.current_album, clean_buf, sizeof(g_croenix_state.current_album) - 1);
                    ESP_LOGI(TAG, "Now Playing Album: %s", clean_buf);
                }

                // Notify BLE companion app with real-time ID3 metadata!
                croenix_ble_notify_status();
            }
            break;
        }

        case ESP_AVRC_CT_CHANGE_NOTIFY_EVT: {
            ESP_LOGI(TAG, "AVRCP Change notification: event_id=0x%x", param->change_ntf.event_id);
            if (param->change_ntf.event_id == ESP_AVRC_RN_TRACK_CHANGE) {
                // Track changed on phone - re-request metadata
                esp_avrc_ct_send_metadata_cmd(
                    0,
                    ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM
                );
            }
            break;
        }

        default:
            ESP_LOGD(TAG, "Unhandled AVRCP CT event: %d", event);
            break;
    }
}

// -------------------------------------------------------------------------
// Transport Control Functions (Sent from Croenix to Phone)
// -------------------------------------------------------------------------
static void send_passthrough_cmd(uint8_t key_code) {
    esp_avrc_ct_send_passthrough_cmd(0, key_code, ESP_AVRC_PT_CMD_STATE_PRESSED);
    esp_avrc_ct_send_passthrough_cmd(0, key_code, ESP_AVRC_PT_CMD_STATE_RELEASED);
}

void croenix_avrcp_play(void) {
    ESP_LOGI(TAG, "Sending AVRCP PLAY command to phone");
    send_passthrough_cmd(ESP_AVRC_PT_CMD_PLAY);
}

void croenix_avrcp_pause(void) {
    ESP_LOGI(TAG, "Sending AVRCP PAUSE command to phone");
    send_passthrough_cmd(ESP_AVRC_PT_CMD_PAUSE);
}

void croenix_avrcp_next(void) {
    ESP_LOGI(TAG, "Sending AVRCP FORWARD/NEXT command to phone");
    send_passthrough_cmd(ESP_AVRC_PT_CMD_FORWARD);
}

void croenix_avrcp_previous(void) {
    ESP_LOGI(TAG, "Sending AVRCP BACKWARD/PREV command to phone");
    send_passthrough_cmd(ESP_AVRC_PT_CMD_BACKWARD);
}

void croenix_avrcp_set_volume(uint8_t volume) {
    if (volume > 100) volume = 100;
    uint8_t raw_vol = (volume * 127) / 100;
    esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_INTERIM, &raw_vol);
}

// -------------------------------------------------------------------------
// Initialization Routine
// -------------------------------------------------------------------------
esp_err_t croenix_avrcp_init(void) {
    ESP_LOGI(TAG, "Initializing AVRCP Target and Controller...");

    // 1. Target (receives commands/volume from phone)
    esp_err_t err = esp_avrc_tg_register_callback(croenix_avrc_tg_cb);
    if (err != ESP_OK) return err;

    err = esp_avrc_tg_init();
    if (err != ESP_OK) return err;

    // 2. Controller (sends transport controls & gets track metadata from phone)
    err = esp_avrc_ct_register_callback(croenix_avrc_ct_cb);
    if (err != ESP_OK) return err;

    err = esp_avrc_ct_init();
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "AVRCP Dual Target/Controller initialized successfully");
    return ESP_OK;
}

