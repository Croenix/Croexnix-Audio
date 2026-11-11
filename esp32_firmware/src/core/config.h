#ifndef CONFIG_H
#define CONFIG_H

// =========================================================================
// ESP32 Audio Player Configuration - CROENIX AUDIOS
// Production-Grade Embedded Architecture & Hardware Mapping
// =========================================================================

#define DEVICE_NAME               "Croenix Audios"
#define FIRMWARE_VERSION          "2.0.0-PROD"
#define BT_DEVICE_NAME            "Croenix BT"

// -------------------------------------------------------------------------
// Hardware Pin Definitions
// -------------------------------------------------------------------------

// =========================================================================
// WCMCU / GY-PCM5102 I2S DAC (Texas Instruments PCM5102A) Wiring Guide:
// =========================================================================
// PIN CONNECTIONS TO ESP32:
//   VCC   -> 5V (or 3.3V depending on breakout regulator)
//   GND   -> GND
//   BCK   -> GPIO 26  (I2S Bit Clock)
//   DIN   -> GPIO 27  (I2S Data Out)
//   LCK   -> GPIO 25  (I2S Word Select / LRCK)
//
// CRITICAL HARDWARE JUMPERS / WIRING (MANDATORY FOR AUDIO OUTPUT):
//   SCK   -> MUST CONNECT TO GND!
//            (PCM5102A internal PLL uses SCK=GND to generate system clock from BCK.
//             If SCK is floating or high, internal PLL is DISABLED -> ZERO SOUND!)
//
//   XSMT  -> MUST CONNECT TO 3.3V!
//            (Active-LOW Soft Mute pin. If floating or GND -> HARD MUTED!)
//
//   FLT   -> CONNECT TO GND (Normal latency filter)
//   DMP   -> CONNECT TO GND (De-emphasis off)
//   FMT   -> CONNECT TO GND (Standard I2S format)
// =========================================================================
#define I2S_DOUT      27  // DIN / DOUT (GPIO 27)
#define I2S_BCLK      26  // BCK (GPIO 26)
#define I2S_LRC       25  // LCK / LRCK / WS (GPIO 25)

// ST7735 1.8" Color TFT Display (SPI Bus + Hardware DMA)
#define TFT_MOSI      23  // SPI MOSI (SDA)
#define TFT_SCLK      18  // SPI SCK (SCL)
#define TFT_CS        5   // Display Chip Select
#define TFT_DC        2   // Data/Command (A0 / DC / RS)
#define TFT_RST       4   // Hardware Reset Line
#define TFT_BL        -1  // Backlight (Tied to 3.3V or PWM)

// MicroSD Card Reader Module (Shared SPI Bus)
#define SD_MISO       19  // SPI MISO (DO)
#define SD_MOSI       23  // Shared SPI MOSI (DI)
#define SD_SCK        18  // Shared SPI SCK (CLK)
#define SD_CS         15  // SD Chip Select

// SD Card SPI Speeds & Limits
#define SD_SPI_SPEED_FAST         10000000 // 10 MHz (Standard fast read)
#define SD_SPI_SPEED_NORMAL        4000000 // 4 MHz (High-reliability fallback)
#define SD_SPI_SPEED_FALLBACK      1000000 // 1 MHz (Safe slow mode)
#define MAX_SD_TRACKS                  128 // Maximum indexed tracks in memory

// HW-322 FM Radio Module (I2C Bus: Si4703 / TEA5767 / RDA5807)
#define FM_SDA        21  // I2C SDA
#define FM_SCL        22  // I2C SCL
#define FM_RST        16  // FM Hardware Reset

// 5-Pin Rotary Encoder (KY-040 Detent Switch)
#define ENCODER_CLK   32  // Quadrature Channel A (CLK)
#define ENCODER_DT    33  // Quadrature Channel B (DT)
#define ENCODER_SW    17  // Encoder Push Button (SW) - Active LOW

// Encoder State Machine Timing
#define ENCODER_LONG_PRESS_MS     10000  // 10-second long press for Hardware Diagnostics
#define ENCODER_SHORT_PRESS_MIN   35     // Debounce threshold (ms)
#define ENCODER_SHORT_PRESS_MAX   900    // Single press detection (ms)
#define FM_TUNE_WINDOW_MS         5000   // 5-second FM Tuning override window

// -------------------------------------------------------------------------
// FreeRTOS Multi-Core Architecture Directives
// -------------------------------------------------------------------------

// Core Allocation: Core 1 is strictly dedicated to audio decoding & DMA streaming
// Core 0 handles UI state machine, display rendering, encoder, and network stack
#define AUDIO_CORE                1  // Core 1: Strictly Audio Decoding & I2S DAC Streaming
#define CONTROL_CORE              0  // Core 0: Display Rendering, UI Calculations, Diagnostics
#define NETWORK_CORE              0  // Core 0: Wi-Fi Stack, WebSocket Server & Provisioning

// FreeRTOS Task Priorities
#define AUDIO_TASK_PRIORITY       5  // Highest real-time priority (prevents buffer underruns)
#define CONTROL_TASK_PRIORITY     3  // High responsiveness for encoder and controls
#define DISPLAY_TASK_PRIORITY     2  // Non-blocking DMA-driven display rendering
#define NETWORK_TASK_PRIORITY     2  // Network events and WebSocket

// FreeRTOS Task Stack Sizes (Statically bounded)
#define AUDIO_STACK_SIZE          8192
#define CONTROL_STACK_SIZE        4096
#define DISPLAY_STACK_SIZE        4096
#define NETWORK_STACK_SIZE        4096

// -------------------------------------------------------------------------
// Audio Subsystem Configuration
// -------------------------------------------------------------------------

#define AUDIO_SAMPLE_RATE         44100
#define AUDIO_BITS_PER_SAMPLE     16
#define AUDIO_CHANNELS            2
#define I2S_BUFFER_SIZE           1024   // DMA ring buffer (bytes)
#define I2S_BUFFER_COUNT          8      // Number of DMA descriptors

// Input Modes (Single-Press Cycling Order: Bluetooth -> SD Card -> FM -> Web FM)
enum AudioSource {
    SOURCE_NONE = 0,
    SOURCE_BLUETOOTH = 1,     // 1. Bluetooth Audio (A2DP Sink)
    SOURCE_SD_CARD = 2,       // 2. SD Card Playback
    SOURCE_FM_RADIO = 3,      // 3. HW-322 FM Radio
    SOURCE_WEB_STREAM = 4     // 4. Web FM / Network Streaming
};

// Bluetooth Connection States
enum BluetoothState {
    BT_STATE_DISCONNECTED = 0, // Blinking red indicator
    BT_STATE_CONNECTED = 1,    // Solid blue indicator
    BT_STATE_PAIRING = 2       // Blinking blue indicator
};

// UI Screen State Machine
enum UIScreenState {
    UI_SCREEN_BOOT_SPLASH = 0,
    UI_SCREEN_HOME = 1,
    UI_SCREEN_HARDWARE_DIAGNOSTICS = 2,
    UI_SCREEN_PROVISIONING_QR = 3
};

// -------------------------------------------------------------------------
// Dual-Mode Networking & Provisioning Settings
// -------------------------------------------------------------------------

// Initial Boot SoftAP Settings
#define AP_SSID_PREFIX            "Croenix-Setup"
#define AP_SSID                   "Croenix-Setup"
#define AP_PASSWORD               "croenix1234"  // WPA2 Pre-Shared Key
#define AP_CHANNEL                1
#define AP_MAX_CONNECTIONS        4
#define AP_STATIC_IP              "192.168.4.1"

// mDNS & WebSocket
#define MDNS_HOSTNAME             "croenix"
#define MDNS_SERVICE_NAME         "croenix-audio"
#define MDNS_SERVICE_PROTOCOL     "tcp"
#define MDNS_SERVICE_PORT         8080
#define WS_PORT                   8080
#define WS_PING_INTERVAL          15000  // ms

// -------------------------------------------------------------------------
// Display & DMA Configuration
// -------------------------------------------------------------------------

#define TFT_WIDTH                 128
#define TFT_HEIGHT                160
#define TFT_ROTATION              1      // Landscape (160x128 pixels)
#define TFT_DMA_CHANNEL           1      // Hardware DMA channel for ST7735 SPI
#define DISPLAY_UPDATE_RATE       60     // ms (~16 FPS non-blocking dirty render)

// -------------------------------------------------------------------------
// Non-Volatile Storage (NVS) Keys
// -------------------------------------------------------------------------

#define NVS_NAMESPACE             "croenix_cfg"
#define NVS_LAST_SOURCE           "src"
#define NVS_VOLUME                "vol"
#define NVS_FM_FREQUENCY          "fm_freq"
#define NVS_SD_TRACK              "sd_trk"
#define NVS_WIFI_SSID             "w_ssid"
#define NVS_WIFI_PASS             "w_pass"
#define NVS_EQ_PRESET             "eq"

// -------------------------------------------------------------------------
// FM Radio Tuning Parameters
// -------------------------------------------------------------------------

#define FM_MIN_FREQUENCY          87.5f  // MHz
#define FM_MAX_FREQUENCY          108.0f // MHz
#define FM_STEP_FREQUENCY         0.1f   // MHz per detent
#define FM_MAX_PRESETS            20

// -------------------------------------------------------------------------
// Limits & Static Buffer Sizes
// -------------------------------------------------------------------------

#define MAX_METADATA_LENGTH       96
#define MAX_SSID_LENGTH           32
#define MAX_PASSWORD_LENGTH       64
#define MAX_URL_LENGTH            192

// -------------------------------------------------------------------------
// Debug Configuration
// -------------------------------------------------------------------------

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
