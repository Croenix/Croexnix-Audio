#ifndef CONFIG_H
#define CONFIG_H

// ===========================
// CROENIX FIRMWARE CONFIG
// Version: 1.0.0
// ===========================

// Device Identity
#define DEVICE_NAME "Croenix"
#define FIRMWARE_VERSION "1.0.0"
#define DEVICE_HOSTNAME "croenix"

// Network Configuration
#define AP_SSID "Croenix-Audio"
#define AP_PASSWORD "Croenix@123"
#define AP_CHANNEL 1
#define AP_MAX_CONNECTIONS 4
#define AP_IP_ADDRESS IPAddress(192, 168, 4, 1)
#define AP_GATEWAY IPAddress(192, 168, 4, 1)
#define AP_SUBNET IPAddress(255, 255, 255, 0)

// Connection Timing
#define WIFI_CONNECT_TIMEOUT_MS 10000
#define WIFI_RECONNECT_INTERVAL_MS 5000
#define WIFI_MAX_RECONNECT_INTERVAL_MS 60000
#define WIFI_STATUS_CHECK_INTERVAL_MS 1000

// HTTP Server
#define HTTP_PORT 80
#define MAX_FAILED_AUTH_ATTEMPTS 10
#define AUTH_LOCKOUT_DURATION_MS 60000
#define PAIRING_TIMEOUT_MS 30000

// mDNS
#define MDNS_SERVICE_NAME "_croenix"
#define MDNS_PROTOCOL "_tcp"

// UDP Discovery
#define UDP_DISCOVERY_PORT 5353
#define DISCOVERY_MAGIC "CROENIX_DISCOVER"
#define DISCOVERY_RESPONSE "CROENIX_HERE"

// NVS (Non-Volatile Storage) Keys
#define NVS_NAMESPACE "croenix"
#define NVS_KEY_AUTH_TOKEN "auth_token"
#define NVS_KEY_WIFI_SSID "wifi_ssid"
#define NVS_KEY_WIFI_PASSWORD "wifi_pass"
#define NVS_KEY_DEVICE_NAME "device_name"
#define NVS_KEY_PAIRING_MODE "pair_mode"

// Pin Definitions (ESP32-WROOM-32 38-pin NodeMCU)
// To be configured when hardware is wired

// Rotary Encoder (KY-040)
#define PIN_ENCODER_CLK -1    // Not yet wired
#define PIN_ENCODER_DT -1     // Not yet wired
#define PIN_ENCODER_SW -1     // Not yet wired

// TFT Display (1.8" SPI)
#define PIN_TFT_CS -1         // Not yet wired
#define PIN_TFT_DC -1         // Not yet wired
#define PIN_TFT_RST -1        // Not yet wired
#define PIN_TFT_MOSI -1       // SPI MOSI
#define PIN_TFT_SCLK -1       // SPI SCK

// I2S DAC (CJMCU-5102)
#define PIN_I2S_BCLK -1       // Not yet wired
#define PIN_I2S_LRCK -1       // Not yet wired
#define PIN_I2S_DOUT -1       // Not yet wired

// SD Card (SPI)
#define PIN_SD_CS -1          // Not yet wired
#define PIN_SD_MOSI -1        // SPI MOSI
#define PIN_SD_MISO -1        // SPI MISO
#define PIN_SD_SCK -1         // SPI SCK

// System Monitoring
#define SYSTEM_MONITOR_INTERVAL_MS 5000
#define HEAP_WARNING_THRESHOLD 20000  // Bytes

// Debug
#define DEBUG_SERIAL true
#define SERIAL_BAUD_RATE 115200

// Macros
#define DEBUG_PRINT(x) if(DEBUG_SERIAL) { Serial.print(x); }
#define DEBUG_PRINTLN(x) if(DEBUG_SERIAL) { Serial.println(x); }

#endif // CONFIG_H
