#ifndef CROENIX_BT_CONFIG_H
#define CROENIX_BT_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// =========================================================================
// CROENIX BLUETOOTH DUAL-MODE ARCHITECTURE CONFIGURATION
// Target: ESP32-WROOM-32 (Dual-Core Xtensa @ 240MHz)
// =========================================================================

// --- Device Branding & Identity ---
#define CROENIX_DEVICE_NAME             "CROENIX"
#define CROENIX_FIRMWARE_VER            "3.1.0-BT-DUAL"
#define CROENIX_HARDWARE_VER            "WROOM-32-REV1"

// --- PCM5102A I²S DAC Pin Definitions ---
#define CONFIG_I2S_BCK_PIN              26   // Bit Clock (BCK)
#define CONFIG_I2S_DOUT_PIN             25   // Serial Data Out (DIN)
#define CONFIG_I2S_LRC_PIN              22   // Left/Right Word Select (LCK/WS)
#define CONFIG_I2S_NUM                  0    // I2S Port 0

// --- Audio Subsystem & DMA Buffering ---
#define CONFIG_AUDIO_RINGBUF_SIZE       (24 * 1024)  // 24KB Fast FreeRTOS Ringbuffer
#define CONFIG_I2S_DMA_BUF_COUNT        8            // Number of DMA descriptors
#define CONFIG_I2S_DMA_BUF_LEN          512          // Samples per DMA buffer
#define CONFIG_AUDIO_TASK_STACK_SIZE    4096         // Stack for Core 1 Audio Task
#define CONFIG_AUDIO_TASK_PRIORITY      (configMAX_PRIORITIES - 2) // High priority
#define CONFIG_AUDIO_TASK_CORE          1            // Dedicated Audio Core

// --- Default & Negotiated Audio Parameters ---
#define DEFAULT_AUDIO_SAMPLE_RATE       44100
#define DEFAULT_AUDIO_BITS_PER_SAMPLE   16
#define DEFAULT_AUDIO_CHANNELS          2

// --- FreeRTOS Task Configuration ---
#define CONFIG_BT_APP_TASK_STACK_SIZE   4096
#define CONFIG_BT_APP_TASK_PRIORITY     (configMAX_PRIORITIES - 3)
#define CONFIG_BT_APP_TASK_CORE         0

// --- BLE GATT Server UUID Specifications ---
// 128-bit Base UUID: 0000xxxx-0000-1000-8000-00805f9b34fb
#define CROENIX_BLE_SERVICE_UUID        0x00FF
#define CROENIX_BLE_CHAR_UUID_CMD       0xFF01  // Write: Remote Control Commands
#define CROENIX_BLE_CHAR_UUID_STATUS    0xFF02  // Read / Notify: Real-time Status
#define CROENIX_BLE_CHAR_UUID_DEVINFO   0xFF03  // Read: Hardware/Firmware Info
#define CROENIX_BLE_CHAR_UUID_CONN      0xFF04  // Read / Notify: Connection Telemetry

// --- BLE Attribute Table Indices ---
enum {
    IDX_SVC,
    IDX_CHAR_CMD,
    IDX_CHAR_VAL_CMD,
    IDX_CHAR_STATUS,
    IDX_CHAR_VAL_STATUS,
    IDX_CHAR_CFG_STATUS,
    IDX_CHAR_DEVINFO,
    IDX_CHAR_VAL_DEVINFO,
    IDX_CHAR_CONN,
    IDX_CHAR_VAL_CONN,
    IDX_CHAR_CFG_CONN,
    HRS_IDX_NB,
};

// --- Playback State Tracking ---
typedef enum {
    CROENIX_PLAYBACK_STOPPED = 0,
    CROENIX_PLAYBACK_PLAYING,
    CROENIX_PLAYBACK_PAUSED
} croenix_playback_state_t;

// --- Combined Device State Structure ---
typedef struct {
    croenix_playback_state_t playback_state;
    uint8_t volume;                  // 0 - 100
    bool muted;
    uint32_t sample_rate;            // Dynamically negotiated A2DP rate
    char current_title[128];
    char current_artist[128];
    char current_album[128];
    bool a2dp_connected;
    char a2dp_peer_name[64];
    uint8_t a2dp_peer_bda[6];
    bool ble_connected;
    uint16_t ble_conn_id;
    uint32_t audio_underruns;
    uint32_t free_heap;
} croenix_state_t;

extern croenix_state_t g_croenix_state;

#ifdef __cplusplus
}
#endif

#endif // CROENIX_BT_CONFIG_H

