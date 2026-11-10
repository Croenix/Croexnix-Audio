// TFT_eSPI User Setup for ST7735 1.8" Display
// Copy this file to: esp32_firmware/.pio/libdeps/esp32dev/TFT_eSPI/User_Setup.h
// Or create a User_Setup_Select.h that includes this configuration

#define ST7735_DRIVER      // Define driver type

#define TFT_WIDTH  128
#define TFT_HEIGHT 160

// Pin definitions for ESP32
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

// Use hardware SPI
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// Color inversion (try both if colors look wrong)
#define TFT_INVERSION_OFF
// #define TFT_INVERSION_ON

// Display orientation (0-3)
#define TFT_ROTATION 1  // Landscape

// RGB order (try both if colors are swapped)
#define TFT_RGB_ORDER TFT_BGR  // Colour order Blue-Green-Red
// #define TFT_RGB_ORDER TFT_RGB  // Colour order Red-Green-Blue

// Font support
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH

#define SMOOTH_FONT

// FSPI port for ESP32 (VSPI is the default)
#define USE_HSPI_PORT
