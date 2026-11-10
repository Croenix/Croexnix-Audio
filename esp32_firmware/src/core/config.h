#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// ESP32 Audio Player Configuration
// ============================================

// --- Hardware Pin Definitions ---

// I2S DAC (CJMCU-5102)
#define I2S_DOUT      22
#define I2S_BCLK      26
#define I2S_LRC       25

// SPI Display (ST7735 1.8" TFT)
#define TFT_MOSI      23
#define TFT_SCLK      18
#define TFT_CS        5
#define TFT_DC        2
#define TFT_RST       4

// SD Card (SPI)
#define SD_MISO       19
#define SD_MOSI       23  // Shared with TFT
#define SD_SCK        18  // Shared with TFT
#define SD_CS         15

// FM Radio (Si4703 I2C)
#define FM_SDA        21
#define FM_SCL        22
#define FM_RST        16

// --- System Configuration ---

// FreeRTOS Task Priorities
#define AUDIO_TASK_PRIORITY       3
#define NETWORK_TASK_PRIORITY     2
#define DISPLAY_TASK_PRIORITY     1
#define CONTROL_TASK_PRIORITY     2

// FreeRTOS Core Assignments
#define AUDIO_CORE                0  // Core 0: Audio & Network
#define CONTROL_CORE              1  // Core 1: Display & Control

// Task Stack Sizes (bytes)
#define AUDIO_STACK_SIZE          8192
#define NETWORK_STACK_SIZE        4096
#define DISPLAY_STACK_SIZE        4096
#define CONTROL_STACK_SIZE        4096

// --- Audio Configuration ---

#define AUDIO_SAMPLE_RATE         44100
#define AUDIO_BITS_PER_SAMPLE     16
#define AUDIO_CHANNELS            2
#define I2S_BUFFER_SIZE           512

// Audio Source Types
enum AudioSource {
    SOURCE_NONE = 0,
    SOURCE_BLUETOOTH,
    SOURCE_SD_CARD,
    SOURCE_FM_RADIO,
    SOURCE_WEB_STREAM
};

// --- Network Configuration ---

// Wi-Fi SoftAP Settings
#define AP_SSID                   "ESP32-Audio-Setup"
#define AP_PASSWORD               ""  // Open AP
#define AP_CHANNEL                1
#define AP_MAX_CONNECTIONS        4
#define AP_IP                     "192.168.4.1"

// mDNS Service Discovery
#define MDNS_HOSTNAME             "esp32-audio"
#define MDNS_SERVICE_NAME         "croenix-audio"
#define MDNS_SERVICE_PROTOCOL     "tcp"
#define MDNS_SERVICE_PORT         8080

// WebSocket Configuration
#define WS_PORT                   8080
#define WS_MAX_CLIENTS            4
#define WS_PING_INTERVAL          30000  // ms

// --- Display Configuration ---

#define TFT_WIDTH                 128
#define TFT_HEIGHT                160
#define TFT_ROTATION              1  // Landscape
#define DISPLAY_UPDATE_RATE       100  // ms

// --- Storage Configuration ---

// NVS Keys
#define NVS_NAMESPACE             "croenix"
#define NVS_WIFI_SSID             "wifi_ssid"
#define NVS_WIFI_PASS             "wifi_pass"
#define NVS_VOLUME                "volume"
#define NVS_EQ_PRESET             "eq_preset"
#define NVS_LAST_SOURCE           "last_source"
#define NVS_FM_FREQUENCY          "fm_freq"

// SD Card Configuration
#define SD_MOUNT_POINT            "/sd"
#define MAX_FILENAME_LENGTH       256
#define MAX_PLAYLIST_SIZE         1000

// --- FM Radio Configuration ---

#define FM_MIN_FREQUENCY          87.5   // MHz
#define FM_MAX_FREQUENCY          108.0  // MHz
#define FM_SPACING                0.1    // MHz
#define FM_MAX_PRESETS            20

// --- System Limits ---

#define MAX_METADATA_LENGTH       128
#define MAX_URL_LENGTH            256
#define MAX_SSID_LENGTH           32
#define MAX_PASSWORD_LENGTH       64

// --- Debug Configuration ---

#define DEBUG_SERIAL              true
#define DEBUG_BAUD_RATE           115200

#if DEBUG_SERIAL
    #define DEBUG_PRINT(x)        Serial.print(x)
    #define DEBUG_PRINTLN(x)      Serial.println(x)
    #define DEBUG_PRINTF(...)     Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H
