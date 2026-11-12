#ifndef CONFIG_H
#define CONFIG_H

// =========================================================================
// CROENIX AUDIO PLAYER - OPTIMIZED FIRMWARE v3.0
// High-Performance, Low-Load Architecture for ESP32-WROOM-32
// =========================================================================

#define DEVICE_NAME               "Croenix Audio"
#define FIRMWARE_VERSION          "3.0.0-OPTIMIZED"
#define BT_DEVICE_NAME            "Croenix BT"

// -------------------------------------------------------------------------
// HARDWARE PIN DEFINITIONS (From Connection Pin List)
// -------------------------------------------------------------------------

// =========================================================================
// I²S DAC - CJMCU-5102 (PCM5102A)
// Module: https://robu.in/product/cjmcu-5102-i2s-player-module/
// =========================================================================
// CRITICAL HARDWARE SETUP:
//   H1L Jumper → MUST be soldered to L position (SCK to GND)
//   H3 Jumper  → MUST be soldered to H position (XSMT to VCC - unmute)
//   VCC → 3.3V or 5V depending on module regulator
// =========================================================================
#define I2S_BCLK      26  // Bit Clock (BCK) - GPIO 26
#define I2S_DOUT      25  // Data Out (DIN) - GPIO 25 (Matches user hardware wiring)
#define I2S_LRC       22  // Left/Right Clock (LCK/WS) - GPIO 22 (Matches user hardware wiring)

// =========================================================================
// ST7735 1.8" TFT Display (128x160 SPI)
// =========================================================================
#define TFT_MOSI      23  // SPI MOSI (SDA) - Shared with SD
#define TFT_SCLK      18  // SPI SCK - Shared with SD
#define TFT_CS         5  // Chip Select
#define TFT_DC         2  // Data/Command (A0/DC/RS)
#define TFT_RST        4  // Hardware Reset
#define TFT_BL        -1  // Backlight (usually tied to 3.3V)

// =========================================================================
// MicroSD Card Reader (Separate SPI Module)
// Module: https://robu.in/product/micro-sd-card-module/
// =========================================================================
// POWER: VCC to 3.3V (if module supports) or 5V (if module has LDO)
// This module typically has level shifters and works with both
// =========================================================================
#define SD_MISO       19  // SPI MISO (DO)
#define SD_MOSI       23  // SPI MOSI (DI) - Shared with TFT
#define SD_SCK        18  // SPI SCK (CLK) - Shared with TFT
#define SD_CS         15  // Chip Select

// =========================================================================
// Si4703 FM Radio Module (I²C)
// =========================================================================
#define FM_SDA        21  // I²C Data (SDIO)
#define FM_SCL        27  // I²C Clock (SCLK) - Moved to GPIO 27 to avoid pin conflict with I2S LCK on GPIO 22
#define FM_RST        16  // Hardware Reset

// =========================================================================
// KY-040 Rotary Encoder (5-pin with button)
// =========================================================================
#define ENCODER_CLK   32  // Quadrature Channel A
#define ENCODER_DT    33  // Quadrature Channel B
#define ENCODER_SW    17  // Push Button - VERIFIED ASSIGNMENT

// -------------------------------------------------------------------------
// OPTIMIZED PERFORMANCE SETTINGS
// -------------------------------------------------------------------------

// FreeRTOS Core Allocation (Optimized for minimal interference)
#define AUDIO_CORE                1  // Core 1: Audio decoding only
#define CONTROL_CORE              0  // Core 0: Everything else
#define NETWORK_CORE              0  // Core 0: Network stack (same as control)

// Task Priorities (Balanced for responsiveness + smooth audio)
#define AUDIO_TASK_PRIORITY       5  // Highest - prevent buffer underruns
#define NETWORK_TASK_PRIORITY     3  // High - responsive app control
#define CONTROL_TASK_PRIORITY     2  // Medium - encoder/buttons
#define DISPLAY_TASK_PRIORITY     1  // Lowest - can tolerate delays

// Stack Sizes (Optimized - reduced from original)
#define AUDIO_STACK_SIZE          6144  // Reduced but sufficient
#define CONTROL_STACK_SIZE        3072  // Reduced
#define DISPLAY_STACK_SIZE        3072  // Reduced
#define NETWORK_STACK_SIZE        4096  // Adequate for WebSocket

// -------------------------------------------------------------------------
// AUDIO SUBSYSTEM - OPTIMIZED FOR SMOOTH PLAYBACK
// -------------------------------------------------------------------------

#define AUDIO_SAMPLE_RATE         44100
#define AUDIO_BITS_PER_SAMPLE     16
#define AUDIO_CHANNELS            2

// Optimized I²S DMA Configuration
// These settings provide smooth audio with minimal CPU load
#define I2S_BUFFER_SIZE           1024   // DMA buffer size (bytes)
#define I2S_BUFFER_COUNT          8      // Number of DMA buffers
#define AUDIO_LOOP_ITERATIONS     6      // Audio pump iterations per cycle

// Audio Processing Timing (ms)
#define AUDIO_UPDATE_INTERVAL     5      // Fast audio loop (200Hz)
#define METADATA_UPDATE_INTERVAL  250    // Metadata updates (4Hz)

// -------------------------------------------------------------------------
// SD CARD CONFIGURATION - OPTIMIZED MOUNTING
// -------------------------------------------------------------------------

// SPI Speed Settings (Progressive fallback)
#define SD_SPI_SPEED_FAST         8000000  // 8 MHz - fastest
#define SD_SPI_SPEED_NORMAL       4000000  // 4 MHz - recommended
#define SD_SPI_SPEED_SLOW         1000000  // 1 MHz - safe mode
#define SD_SPI_SPEED_INIT          400000  // 400 kHz - SD spec init

// Track Management
#define MAX_SD_TRACKS             256      // Increased capacity
#define SD_SCAN_TIMEOUT_MS        10000    // 10 second scan timeout

// -------------------------------------------------------------------------
// DISPLAY CONFIGURATION - LOW CPU USAGE
// -------------------------------------------------------------------------

#define TFT_WIDTH                 128
#define TFT_HEIGHT                160
#define TFT_ROTATION              1        // Landscape (160x128)
#define DISPLAY_UPDATE_RATE       50       // 20 FPS (reduced for lower CPU)
#define DISPLAY_TIMEOUT_MS        30000    // 30s idle before dimming

// -------------------------------------------------------------------------
// ENCODER CONFIGURATION
// -------------------------------------------------------------------------

#define ENCODER_DEBOUNCE_MS       5        // Fast debounce
#define ENCODER_LONG_PRESS_MS     2000     // 2s for long press
#define ENCODER_SHORT_PRESS_MIN   50       // Min press time
#define ENCODER_SHORT_PRESS_MAX   800      // Max short press time

// FM Radio Tuning Window
#define FM_TUNE_WINDOW_MS         5000     // 5s FM tuning override window

// -------------------------------------------------------------------------
// AUDIO SOURCE MODES
// -------------------------------------------------------------------------

enum AudioSource {
    SOURCE_NONE = 0,
    SOURCE_BLUETOOTH = 1,
    SOURCE_SD_CARD = 2,
    SOURCE_FM_RADIO = 3,
    SOURCE_WEB_STREAM = 4
};

// -------------------------------------------------------------------------
// BLUETOOTH STATES
// -------------------------------------------------------------------------

enum BluetoothState {
    BT_STATE_DISCONNECTED = 0,
    BT_STATE_CONNECTED = 1,
    BT_STATE_PAIRING = 2
};

// -------------------------------------------------------------------------
// UI SCREEN STATES
// -------------------------------------------------------------------------

enum UIScreenState {
    UI_SCREEN_BOOT_SPLASH = 0,
    UI_SCREEN_HOME = 1,
    UI_SCREEN_PLAYLIST = 2,
    UI_SCREEN_SETTINGS = 3,
    UI_SCREEN_DIAGNOSTICS = 4,
    UI_SCREEN_HARDWARE_DIAGNOSTICS = 4,  // Alias for compatibility
    UI_SCREEN_PROVISIONING_QR = 5
};

// -------------------------------------------------------------------------
// NETWORK CONFIGURATION
// -------------------------------------------------------------------------

// SoftAP Configuration
#define AP_SSID                   "Croenix-Audio"
#define AP_SSID_PREFIX            "Croenix-Setup"  // Legacy compatibility
#define AP_PASSWORD               "croenix1234"
#define AP_CHANNEL                6
#define AP_MAX_CONNECTIONS        3
#define AP_IP                     "192.168.4.1"
#define AP_STATIC_IP              "192.168.4.1"    // Legacy compatibility

// WebSocket Configuration
#define WS_PORT                   8080
#define WS_PING_INTERVAL_MS       10000    // 10s keep-alive
#define WS_MAX_CLIENTS            3
#define WS_BUFFER_SIZE            1024

// mDNS Configuration
#define MDNS_HOSTNAME             "croenix"
#define MDNS_SERVICE_NAME         "croenix-audio"
#define MDNS_SERVICE_PROTOCOL     "tcp"
#define MDNS_SERVICE_PORT         8080

// -------------------------------------------------------------------------
// FM RADIO CONFIGURATION
// -------------------------------------------------------------------------

#define FM_MIN_FREQUENCY          87.5f
#define FM_MAX_FREQUENCY          108.0f
#define FM_STEP_FREQUENCY         0.1f
#define FM_MAX_PRESETS            20
#define FM_TUNE_TIMEOUT_MS        5000

// -------------------------------------------------------------------------
// NON-VOLATILE STORAGE (NVS) KEYS
// -------------------------------------------------------------------------

#define NVS_NAMESPACE             "croenix"
#define NVS_LAST_SOURCE           "source"
#define NVS_VOLUME                "volume"
#define NVS_FM_FREQUENCY          "fm_freq"
#define NVS_SD_TRACK              "sd_track"
#define NVS_WIFI_SSID             "wifi_ssid"
#define NVS_WIFI_PASS             "wifi_pass"
#define NVS_EQ_PRESET             "eq_preset"
#define NVS_BRIGHTNESS            "brightness"

// -------------------------------------------------------------------------
// BUFFER LIMITS
// -------------------------------------------------------------------------

#define MAX_METADATA_LENGTH       96
#define MAX_SSID_LENGTH           32
#define MAX_PASSWORD_LENGTH       64
#define MAX_URL_LENGTH            256
#define MAX_TRACK_NAME_LENGTH     64
#define MAX_ARTIST_LENGTH         64

// -------------------------------------------------------------------------
// SYSTEM TIMING (Optimized for low CPU load)
// -------------------------------------------------------------------------

#define MAIN_LOOP_DELAY_MS        1        // Minimal delay for yielding
#define IDLE_CHECK_INTERVAL_MS    1000     // Check idle state every 1s
#define WATCHDOG_TIMEOUT_MS       5000     // 5s watchdog
#define BOOT_DELAY_MS             2000     // 2s boot splash

// -------------------------------------------------------------------------
// MEMORY OPTIMIZATION FLAGS
// -------------------------------------------------------------------------

#define ENABLE_MEMORY_MONITOR     false    // Disable for production
#define ENABLE_CPU_PROFILING      false    // Disable for production
#define ENABLE_DEEP_SLEEP         true     // Enable power saving
#define ENABLE_WIFI_POWER_SAVE    true     // WiFi power management

// -------------------------------------------------------------------------
// DEBUG CONFIGURATION (Set to false for production)
// -------------------------------------------------------------------------

#define DEBUG_SERIAL              true
#define DEBUG_BAUD_RATE           115200
#define DEBUG_AUDIO               false     // Verbose audio logs
#define DEBUG_NETWORK             false     // Verbose network logs
#define DEBUG_SD                  true      // SD card logs
#define DEBUG_DISPLAY             false     // Display logs

#if DEBUG_SERIAL
    #define DEBUG_PRINT(x)        Serial.print(x)
    #define DEBUG_PRINTLN(x)      Serial.println(x)
    #define DEBUG_PRINTF(...)     Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

// Conditional debug macros
#define DEBUG_AUDIO_PRINT(x)      if(DEBUG_AUDIO) Serial.print(x)
#define DEBUG_AUDIO_PRINTLN(x)    if(DEBUG_AUDIO) Serial.println(x)
#define DEBUG_AUDIO_PRINTF(...)   if(DEBUG_AUDIO) Serial.printf(__VA_ARGS__)

#define DEBUG_NET_PRINT(x)        if(DEBUG_NETWORK) Serial.print(x)
#define DEBUG_NET_PRINTLN(x)      if(DEBUG_NETWORK) Serial.println(x)
#define DEBUG_NET_PRINTF(...)     if(DEBUG_NETWORK) Serial.printf(__VA_ARGS__)

#define DEBUG_SD_PRINT(x)         if(DEBUG_SD) Serial.print(x)
#define DEBUG_SD_PRINTLN(x)       if(DEBUG_SD) Serial.println(x)
#define DEBUG_SD_PRINTF(...)      if(DEBUG_SD) Serial.printf(__VA_ARGS__)

// -------------------------------------------------------------------------
// FEATURE FLAGS
// -------------------------------------------------------------------------

#define ENABLE_BLUETOOTH          true     // Bluetooth A2DP sink
#define ENABLE_FM_RADIO           true     // FM radio support
#define ENABLE_WEB_STREAM         true     // Network streaming
#define ENABLE_EQUALIZER          true     // EQ presets
#define ENABLE_PLAYLIST           true     // Playlist support
#define ENABLE_METADATA           true     // ID3/metadata parsing
#define ENABLE_SPECTRUM           false    // Audio spectrum (high CPU)

#endif // CONFIG_H
