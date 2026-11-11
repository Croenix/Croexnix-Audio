#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// ESP32 Audio Player Configuration - Croenix
// ============================================

#define DEVICE_NAME               "Croenix"
#define FIRMWARE_VERSION          "1.1.0"
#define BT_DEVICE_NAME            "Croenix"

// --- Hardware Pin Definitions ---

// I2S DAC (WCMCU / CJMCU-5102 / PCM5102A)
#define I2S_DOUT      27  // DIN / DOUT (GPIO 27)
#define I2S_BCLK      26  // BCK (GPIO 26)
#define I2S_LRC       25  // LCK / LRCK / WS (GPIO 25)

// SPI Display (ST7735 1.8" TFT)
#define TFT_MOSI      23  // Shared SPI MOSI (SDA on TFT)
#define TFT_SCLK      18  // Shared SPI SCK
#define TFT_CS        5   // Display Chip Select
#define TFT_DC        2   // Data/Command (A0 / DC / RS)
#define TFT_RST       4   // Display Reset

// SD Card (SPI - MicroSD Card Reader Module)
#define SD_MISO       19  // SPI MISO (DO)
#define SD_MOSI       23  // Shared SPI MOSI (DI)
#define SD_SCK        18  // Shared SPI SCK (CLK)
#define SD_CS         15  // SD Chip Select

// HW-322 FM Radio (Si4703 / TEA5767 I2C)
#define FM_SDA        21  // I2C SDA (SDIO)
#define FM_SCL        22  // I2C SCL (SCLK)
#define FM_RST        16  // FM Module Reset

// 5-Pin Rotary Encoder (KY-040 Style)
#define ENCODER_CLK   32  // Rotary Channel A (CLK)
#define ENCODER_DT    33  // Rotary Channel B (DT)
#define ENCODER_SW    17  // Push Button (SW) - Active LOW

// Encoder Timings
#define ENCODER_LONG_PRESS_MS     10000  // 10-second long press for Hardware Diagnostics
#define ENCODER_SHORT_PRESS_MIN   40     // Debounce threshold (ms)
#define ENCODER_SHORT_PRESS_MAX   1200   // Single press window (ms)

// --- System Configuration ---

// FreeRTOS Task Priorities
#define AUDIO_TASK_PRIORITY       3  // Highest priority to prevent I2S buffer underruns
#define NETWORK_TASK_PRIORITY     2
#define DISPLAY_TASK_PRIORITY     1  // Non-blocking UI
#define CONTROL_TASK_PRIORITY     2  // High responsiveness for encoder and controls

// FreeRTOS Core Assignments
#define AUDIO_CORE                0  // Core 0: Audio Pipeline & Network
#define CONTROL_CORE              1  // Core 1: Display & Peripherals Control

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

// Audio Source Modes (in single-press cycling order)
enum AudioSource {
    SOURCE_NONE = 0,
    SOURCE_FM_RADIO = 1,      // 1. FM Radio
    SOURCE_WEB_STREAM = 2,    // 2. Music Player (Web/Network stream)
    SOURCE_BLUETOOTH = 3,     // 3. Bluetooth Audio
    SOURCE_SD_CARD = 4        // 4. SD Card Playback
};

// Bluetooth Connection Status
enum BluetoothState {
    BT_STATE_DISCONNECTED = 0, // Blinking red indicator
    BT_STATE_PAIRING = 1,      // Blinking blue indicator
    BT_STATE_CONNECTED = 2     // Solid blue indicator
};

// UI Screen State Machine
enum UIScreenState {
    UI_SCREEN_BOOT_SPLASH = 0,
    UI_SCREEN_HOME = 1,
    UI_SCREEN_HARDWARE_DIAGNOSTICS = 2
};

// --- Network Configuration ---

// Wi-Fi SoftAP Settings (Secure provisioning hotspot on boot)
#define AP_SSID                   "Croenix-Audio-Setup"
#define AP_PASSWORD               "croenix1234"  // Secure WPA2 password
#define AP_CHANNEL                1
#define AP_MAX_CONNECTIONS        4
#define AP_IP                     "192.168.4.1"

// mDNS Service Discovery
#define MDNS_HOSTNAME             "croenix"
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
#define TFT_ROTATION              1  // Landscape (160x128)
#define DISPLAY_UPDATE_RATE       80 // ms (non-blocking smooth 12+ FPS)

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
