#ifndef CROENIX_PIN_CONFIG_H
#define CROENIX_PIN_CONFIG_H

#include <Arduino.h>

// ==============================================================================
// CROENIX PIN CONFIGURATION
// Target: ESP32-WROOM-32 (38-pin NodeMCU development board)
// Unverified or disconnected hardware lines are configured with -1.
// Do not drive GPIOs without verifying hardware wiring.
// ==============================================================================

// ------------------------------------------------------------------------------
// 1. KY-040 Rotary Encoder
// ------------------------------------------------------------------------------
// Default assignment proposal:
// CLK: GPIO 32
// DT:  GPIO 33
// SW:  GPIO 25 (Push switch)
#define PIN_ENCODER_CLK         32
#define PIN_ENCODER_DT          33
#define PIN_ENCODER_SW          25
#define ENCODER_ACTIVE_LOW      true
#define ENCODER_ENABLE_PULLUP   true

// ------------------------------------------------------------------------------
// 2. CJMCU-5102 I2S DAC (PCM5102A)
// ------------------------------------------------------------------------------
// Standard ESP32 I2S output pins
#define PIN_I2S_BCLK            26  // Bit clock (BCK)
#define PIN_I2S_LRCK            27  // Word select / Left-Right clock (WS/LCK)
#define PIN_I2S_DOUT            22  // Serial data out (DIN)
#define PIN_I2S_MCLK            -1  // Master clock (PCM5102 generates internally if tied to GND)

// ------------------------------------------------------------------------------
// 3. 1.8-inch SPI TFT LCD (128x160)
// ------------------------------------------------------------------------------
// SPI bus pins (VSPI / HSPI)
#define PIN_TFT_CS              5   // Chip select
#define PIN_TFT_DC              16  // Data / Command
#define PIN_TFT_RST             17  // Reset
#define PIN_TFT_MOSI            23  // Master Out Slave In
#define PIN_TFT_SCLK            18  // Clock
#define PIN_TFT_BL              -1  // Backlight control (optional / tied to 3.3V)

// ------------------------------------------------------------------------------
// 4. MicroSD Card Interface (SPI)
// ------------------------------------------------------------------------------
#define PIN_SD_CS               4   // SD Chip select
#define PIN_SD_MOSI             23  // Shared or dedicated SPI
#define PIN_SD_MISO             19  // Master In Slave Out
#define PIN_SD_SCK              18  // SPI Clock
#define PIN_SD_DETECT           -1  // Card detect switch (optional)

// ------------------------------------------------------------------------------
// 5. Physical FM Tuner (Hardware model TBD)
// ------------------------------------------------------------------------------
// I2C interface lines for future TEA5767 / RDA5807 / Si4703 tuner module
#define PIN_FM_SDA              21  // Standard I2C SDA
#define PIN_FM_SCL              22  // Standard I2C SCL (Note: verify conflict if DOUT uses 22)
#define PIN_FM_RST              -1  // Reset line
#define PIN_FM_ENABLED          false

// ------------------------------------------------------------------------------
// 6. Amplifiers & Mute Controls (3x Gold A542 Stereo Amplifiers)
// ------------------------------------------------------------------------------
#define PIN_AMP_MUTE            -1  // Amplifier shutdown / mute pin
#define PIN_AMP_STANDBY         -1  // Power standby pin

// ------------------------------------------------------------------------------
// 7. Status LEDs & Indicators
// ------------------------------------------------------------------------------
#define PIN_STATUS_LED          2   // Onboard LED (GPIO 2 on NodeMCU ESP32)
#define STATUS_LED_ACTIVE_HIGH  true

#endif // CROENIX_PIN_CONFIG_H

