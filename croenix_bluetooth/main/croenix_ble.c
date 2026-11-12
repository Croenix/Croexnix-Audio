#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_system.h"

#include "croenix_ble.h"
#include "croenix_audio.h"
#include "croenix_avrcp.h"
#include "croenix_bt_config.h"

static const char *TAG = "CROENIX_BLE";

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define CROENIX_SVC_INST_ID         0

// Attribute handles
static uint16_t s_handle_table[HRS_IDX_NB];
static esp_gatt_if_t s_gatts_if = ESP_GATT_IF_NONE;
static uint16_t s_conn_id = 0xFFFF;
static bool s_status_notify_enabled = false;
static bool s_conn_notify_enabled = false;

// -------------------------------------------------------------------------
// BLE Advertising Configuration
// -------------------------------------------------------------------------
static uint8_t s_service_uuid[16] = {
    // 000000ff-0000-1000-8000-00805f9b34fb
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00
};

static esp_ble_adv_data_t s_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0020, // 20ms fast advertising
    .max_interval = 0x0040, // 40ms
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(s_service_uuid),
    .p_service_uuid = s_service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_DMT_CONTROLLER_SPT | ESP_BLE_ADV_FLAG_DMT_HOST_SPT),
};

static esp_ble_adv_data_t s_scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0020,
    .max_interval = 0x0040,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(s_service_uuid),
    .p_service_uuid = s_service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_DMT_CONTROLLER_SPT | ESP_BLE_ADV_FLAG_DMT_HOST_SPT),
};

static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min        = 0x40, // 40ms interval for ultra-fast nearby detection
    .adv_int_max        = 0x80, // 80ms
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// -------------------------------------------------------------------------
// GATT Database Attribute Table
// -------------------------------------------------------------------------
static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_only  = ESP_GATT_CHAR_PROP_BIT_READ;

static const uint16_t GATTS_CHAR_UUID_CMD     = CROENIX_BLE_CHAR_UUID_CMD;
static const uint16_t GATTS_CHAR_UUID_STATUS  = CROENIX_BLE_CHAR_UUID_STATUS;
static const uint16_t GATTS_CHAR_UUID_DEVINFO = CROENIX_BLE_CHAR_UUID_DEVINFO;
static const uint16_t GATTS_CHAR_UUID_CONN    = CROENIX_BLE_CHAR_UUID_CONN;

static const uint8_t s_ccc[2] = {0x00, 0x00};

static const esp_gatts_attr_db_t s_gatt_db[HRS_IDX_NB] = {
    // Service Declaration
    [IDX_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
         sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)&s_service_uuid[12]}
    },

    // Characteristic 1: Command (Write)
    [IDX_CHAR_CMD] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}
    },
    [IDX_CHAR_VAL_CMD] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_CMD, ESP_GATT_PERM_WRITE,
         256, 0, NULL}
    },

    // Characteristic 2: Status (Read & Notify)
    [IDX_CHAR_STATUS] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}
    },
    [IDX_CHAR_VAL_STATUS] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_STATUS, ESP_GATT_PERM_READ,
         512, 0, NULL}
    },
    [IDX_CHAR_CFG_STATUS] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         sizeof(uint16_t), sizeof(s_ccc), (uint8_t *)s_ccc}
    },

    // Characteristic 3: Device Info (Read-only)
    [IDX_CHAR_DEVINFO] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_only}
    },
    [IDX_CHAR_VAL_DEVINFO] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_DEVINFO, ESP_GATT_PERM_READ,
         128, 0, NULL}
    },

    // Characteristic 4: Connection Telemetry (Read & Notify)
    [IDX_CHAR_CONN] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}
    },
    [IDX_CHAR_VAL_CONN] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_CONN, ESP_GATT_PERM_READ,
         256, 0, NULL}
    },
    [IDX_CHAR_CFG_CONN] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         sizeof(uint16_t), sizeof(s_ccc), (uint8_t *)s_ccc}
    },
};

// -------------------------------------------------------------------------
// Helper: Construct JSON Status Payload
// -------------------------------------------------------------------------
static int build_status_json(char *buf, size_t max_len) {
    const char *state_str = "STOPPED";
    if (g_croenix_state.playback_state == CROENIX_PLAYBACK_PLAYING) state_str = "PLAYING";
    else if (g_croenix_state.playback_state == CROENIX_PLAYBACK_PAUSED) state_str = "PAUSED";

    g_croenix_state.free_heap = esp_get_free_heap_size();

    return snprintf(buf, max_len,
        "{\"state\":\"%s\",\"vol\":%u,\"rate\":%lu,\"title\":\"%s\",\"artist\":\"%s\",\"heap\":%lu}",
        state_str,
        g_croenix_state.volume,
        g_croenix_state.sample_rate,
        g_croenix_state.current_title[0] ? g_croenix_state.current_title : "Bluetooth Stream",
        g_croenix_state.current_artist[0] ? g_croenix_state.current_artist : "CROENIX Hi-Fi",
        g_croenix_state.free_heap / 1024
    );
}

static int build_conn_json(char *buf, size_t max_len) {
    return snprintf(buf, max_len,
        "{\"a2dp\":%s,\"peer\":\"%s\",\"ble\":true,\"bda\":\"%02X:%02X:%02X:%02X:%02X:%02X\"}",
        g_croenix_state.a2dp_connected ? "true" : "false",
        g_croenix_state.a2dp_peer_name,
        g_croenix_state.a2dp_peer_bda[0], g_croenix_state.a2dp_peer_bda[1],
        g_croenix_state.a2dp_peer_bda[2], g_croenix_state.a2dp_peer_bda[3],
        g_croenix_state.a2dp_peer_bda[4], g_croenix_state.a2dp_peer_bda[5]
    );
}

// -------------------------------------------------------------------------
// BLE Notifications
// -------------------------------------------------------------------------
void croenix_ble_notify_status(void) {
    if (!g_croenix_state.ble_connected || s_gatts_if == ESP_GATT_IF_NONE) return;

    char payload[384];
    int len = build_status_json(payload, sizeof(payload));
    if (len > 0) {
        esp_ble_gatts_send_indicate(
            s_gatts_if,
            s_conn_id,
            s_handle_table[IDX_CHAR_VAL_STATUS],
            len,
            (uint8_t *)payload,
            false // Notification (unacknowledged for high speed)
        );
    }
}

void croenix_ble_notify_connection_status(void) {
    if (!g_croenix_state.ble_connected || s_gatts_if == ESP_GATT_IF_NONE) return;

    char payload[256];
    int len = build_conn_json(payload, sizeof(payload));
    if (len > 0) {
        esp_ble_gatts_send_indicate(
            s_gatts_if,
            s_conn_id,
            s_handle_table[IDX_CHAR_VAL_CONN],
            len,
            (uint8_t *)payload,
            false
        );
    }
}

// -------------------------------------------------------------------------
// Command Protocol Parser & Execution
// -------------------------------------------------------------------------
void croenix_ble_handle_command(const char *cmd, size_t len) {
    if (cmd == NULL || len == 0) return;

    char clean_cmd[128];
    size_t copy_len = (len < sizeof(clean_cmd) - 1) ? len : sizeof(clean_cmd) - 1;
    memcpy(clean_cmd, cmd, copy_len);
    clean_cmd[copy_len] = '\0';

    // Strip carriage return/newline
    while (copy_len > 0 && (clean_cmd[copy_len - 1] == '\r' || clean_cmd[copy_len - 1] == '\n' || clean_cmd[copy_len - 1] == ' ')) {
        clean_cmd[--copy_len] = '\0';
    }

    ESP_LOGI(TAG, "BLE Command Received: \"%s\"", clean_cmd);

    if (strcasecmp(clean_cmd, "PLAY") == 0 || strstr(clean_cmd, "\"command\":\"play\"") != NULL) {
        croenix_avrcp_play();
    } else if (strcasecmp(clean_cmd, "PAUSE") == 0 || strstr(clean_cmd, "\"command\":\"pause\"") != NULL) {
        croenix_avrcp_pause();
    } else if (strcasecmp(clean_cmd, "NEXT") == 0 || strstr(clean_cmd, "\"command\":\"next_track\"") != NULL) {
        croenix_avrcp_next();
    } else if (strcasecmp(clean_cmd, "PREVIOUS") == 0 || strcasecmp(clean_cmd, "PREV") == 0 || strstr(clean_cmd, "\"command\":\"prev_track\"") != NULL) {
        croenix_avrcp_previous();
    } else if (strcasecmp(clean_cmd, "VOLUME_UP") == 0) {
        uint8_t cur = croenix_audio_get_volume();
        croenix_audio_set_volume((cur <= 95) ? cur + 5 : 100);
        croenix_avrcp_set_volume(croenix_audio_get_volume());
    } else if (strcasecmp(clean_cmd, "VOLUME_DOWN") == 0) {
        uint8_t cur = croenix_audio_get_volume();
        croenix_audio_set_volume((cur >= 5) ? cur - 5 : 0);
        croenix_avrcp_set_volume(croenix_audio_get_volume());
    } else if (strncasecmp(clean_cmd, "VOLUME:", 7) == 0) {
        int vol = atoi(clean_cmd + 7);
        if (vol < 0) vol = 0;
        if (vol > 100) vol = 100;
        croenix_audio_set_volume((uint8_t)vol);
        croenix_avrcp_set_volume((uint8_t)vol);
    } else if (strcasecmp(clean_cmd, "MUTE") == 0) {
        g_croenix_state.muted = !g_croenix_state.muted;
        croenix_audio_set_volume(g_croenix_state.muted ? 0 : 50);
    } else if (strcasecmp(clean_cmd, "GET_STATUS") == 0) {
        croenix_ble_notify_status();
        croenix_ble_notify_connection_status();
        return;
    } else {
        ESP_LOGW(TAG, "Unknown BLE command format: %s", clean_cmd);
    }

    // Always push updated status back to mobile app immediately
    croenix_ble_notify_status();
}

// -------------------------------------------------------------------------
// GAP Callback Handler for BLE Advertising & Security
// -------------------------------------------------------------------------
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "BLE advertising data configured - starting advertisement...");
            esp_ble_gap_start_advertising(&s_adv_params);
            break;

        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            ESP_LOGD(TAG, "BLE scan response data set complete");
            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising start failed: %d", param->adv_start_cmpl.status);
            } else {
                ESP_LOGI(TAG, "BLE Advertising ACTIVE. Name: \"%s\" (Discoverable by Nearby Devices)", CROENIX_DEVICE_NAME);
            }
            break;

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            ESP_LOGD(TAG, "Advertising stopped");
            break;

        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGD(TAG, "Connection params updated: interval=%d, latency=%d, timeout=%d",
                     param->update_conn_params.interval,
                     param->update_conn_params.latency,
                     param->update_conn_params.timeout);
            break;

        default:
            break;
    }
}

// -------------------------------------------------------------------------
// GATTS Callback Handler
// -------------------------------------------------------------------------
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            s_gatts_if = gatts_if;
            ESP_LOGI(TAG, "GATTS registered, creating attribute table...");
            esp_ble_gap_set_device_name(CROENIX_DEVICE_NAME);

            esp_ble_gap_config_adv_data(&s_adv_data);
            esp_ble_gap_config_adv_data(&s_scan_rsp_data);

            esp_ble_gatts_create_attr_tab(s_gatt_db, gatts_if, HRS_IDX_NB, CROENIX_SVC_INST_ID);
            break;
        }

        case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
            if (param->add_attr_tab.status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "Create attribute table failed, error code: 0x%x", param->add_attr_tab.status);
            } else if (param->add_attr_tab.num_handle != HRS_IDX_NB) {
                ESP_LOGE(TAG, "Attribute table handle count mismatch: %d != %d", param->add_attr_tab.num_handle, HRS_IDX_NB);
            } else {
                ESP_LOGI(TAG, "GATT table created successfully (%d handles). Starting service...", param->add_attr_tab.num_handle);
                memcpy(s_handle_table, param->add_attr_tab.handles, sizeof(s_handle_table));
                esp_ble_gatts_start_service(s_handle_table[IDX_SVC]);
            }
            break;
        }

        case ESP_GATTS_CONNECT_EVT: {
            ESP_LOGI(TAG, "Croenix Mobile Application connected via BLE (conn_id: %d)", param->connect.conn_id);
            s_conn_id = param->connect.conn_id;
            g_croenix_state.ble_connected = true;
            g_croenix_state.ble_conn_id = s_conn_id;

            // Notify initial states immediately
            croenix_ble_notify_status();
            croenix_ble_notify_connection_status();
            break;
        }

        case ESP_GATTS_DISCONNECT_EVT: {
            ESP_LOGI(TAG, "Mobile Application disconnected from BLE. Restarting advertising...");
            g_croenix_state.ble_connected = false;
            s_conn_id = 0xFFFF;
            s_status_notify_enabled = false;
            s_conn_notify_enabled = false;

            // Always resume advertising immediately for zero-friction reconnect!
            esp_ble_gap_start_advertising(&s_adv_params);
            break;
        }

        case ESP_GATTS_READ_EVT: {
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.handle = param->read.handle;

            if (param->read.handle == s_handle_table[IDX_CHAR_VAL_STATUS]) {
                char buf[384];
                rsp.attr_value.len = build_status_json(buf, sizeof(buf));
                memcpy(rsp.attr_value.value, buf, rsp.attr_value.len);
            } else if (param->read.handle == s_handle_table[IDX_CHAR_VAL_DEVINFO]) {
                const char *info = "{\"device\":\"CROENIX\",\"fw\":\"" CROENIX_FIRMWARE_VER "\",\"dac\":\"PCM5102A\"}";
                rsp.attr_value.len = strlen(info);
                memcpy(rsp.attr_value.value, info, rsp.attr_value.len);
            } else if (param->read.handle == s_handle_table[IDX_CHAR_VAL_CONN]) {
                char buf[256];
                rsp.attr_value.len = build_conn_json(buf, sizeof(buf));
                memcpy(rsp.attr_value.value, buf, rsp.attr_value.len);
            }

            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            break;
        }

        case ESP_GATTS_WRITE_EVT: {
            if (!param->write.is_prep) {
                // Command written to IDX_CHAR_VAL_CMD
                if (param->write.handle == s_handle_table[IDX_CHAR_VAL_CMD]) {
                    croenix_ble_handle_command((const char *)param->write.value, param->write.len);
                }
                // Client Configuration Descriptor (CCCD) enabled for notifications
                else if (param->write.handle == s_handle_table[IDX_CHAR_CFG_STATUS]) {
                    uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
                    s_status_notify_enabled = (descr_value == 0x0001);
                    ESP_LOGI(TAG, "Status notification %s", s_status_notify_enabled ? "ENABLED" : "DISABLED");
                } else if (param->write.handle == s_handle_table[IDX_CHAR_CFG_CONN]) {
                    uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
                    s_conn_notify_enabled = (descr_value == 0x0001);
                    ESP_LOGI(TAG, "Connection notification %s", s_conn_notify_enabled ? "ENABLED" : "DISABLED");
                }
            }

            if (param->write.need_rsp) {
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            }
            break;
        }

        default:
            break;
    }
}

// -------------------------------------------------------------------------
// Initialization Routine
// -------------------------------------------------------------------------
esp_err_t croenix_ble_init(void) {
    ESP_LOGI(TAG, "Initializing BLE GATT Server...");

    esp_err_t err = esp_ble_gatts_register_callback(gatts_profile_event_handler);
    if (err != ESP_OK) return err;

    err = esp_ble_gap_register_callback(gap_event_handler);
    if (err != ESP_OK) return err;

    err = esp_ble_gatts_app_register(ESP_APP_ID);
    if (err != ESP_OK) return err;

    return ESP_OK;
}

