#include "croenix_bt_init.h"
#include "croenix_bt_config.h"

#include <string.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

static const char *TAG = "CROENIX_BT_INIT";

static void croenix_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Classic BT Auth Success: %s [%02x:%02x:%02x:%02x:%02x:%02x]",
                     param->auth_cmpl.device_name,
                     param->auth_cmpl.bda[0], param->auth_cmpl.bda[1],
                     param->auth_cmpl.bda[2], param->auth_cmpl.bda[3],
                     param->auth_cmpl.bda[4], param->auth_cmpl.bda[5]);
        } else {
            ESP_LOGE(TAG, "Classic BT Auth Failed, status: %d", param->auth_cmpl.stat);
        }
        break;

    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(TAG, "Classic BT Numeric Confirmation Request: passkey = %" PRIu32, param->cfm_req.num_val);
        // Automatically accept confirmation for standard audio pairing
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;

    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG, "Classic BT Passkey Notification: %" PRIu32, param->key_notif.passkey);
        break;

    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(TAG, "Classic BT Passkey Request");
        break;

    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGD(TAG, "Classic BT Mode Change: mode = %d", param->mode_chg.mode);
        break;

    default:
        ESP_LOGD(TAG, "Classic BT GAP Event: %d", event);
        break;
    }
}

esp_err_t croenix_bt_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "=============================================");
    ESP_LOGI(TAG, "Initializing CROENIX Bluetooth Dual-Mode Core");
    ESP_LOGI(TAG, "Device: %s | Firmware: %s", CROENIX_DEVICE_NAME, CROENIX_FIRMWARE_VER);
    ESP_LOGI(TAG, "=============================================");

    // 1. Release unused memory if BLE only or Classic only, but for DUAL MODE we retain all
    // Do NOT release memory when running dual-mode (BTDM)

    // 2. Initialize Bluetooth Controller in Dual Mode (Classic A2DP + BLE)
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_enable (BTDM) failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Bluetooth Controller enabled in Dual Mode (BTDM)");

    // 3. Initialize and enable Bluedroid Host Stack
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_enable failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Bluedroid Host Stack enabled successfully");

    // 4. Set device identity name
    ret = esp_bt_dev_set_device_name(CROENIX_DEVICE_NAME);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_dev_set_device_name failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Local Bluetooth Device Name set to: '%s'", CROENIX_DEVICE_NAME);

    // 5. Register GAP callback for Classic Bluetooth
    ret = esp_bt_gap_register_callback(croenix_gap_callback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_gap_register_callback failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 6. Configure SSP (Simple Secure Pairing)
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

    // 7. Make device Connectable and Discoverable for Classic Bluetooth (A2DP phones)
    ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_gap_set_scan_mode failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Classic BT Discoverable and Connectable mode set");

    return ESP_OK;
}

