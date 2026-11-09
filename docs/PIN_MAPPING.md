# Croenix Pin Mapping & Hardware Wiring Reference

Target Board: **ESP32-WROOM-32 (38-pin NodeMCU)**

All pins are centrally configured in [`croenix_firmware/src/PinConfig.h`](file:///c:/Users/achub/OneDrive/Documents/Music%20Player/croenix_firmware/src/PinConfig.h).

---

## 1. Pin Assignment Table

| Pin Name | GPIO | Type | Connected Peripheral | Wiring Notes |
|---|---|---|---|---|
| `PIN_STATUS_LED` | `GPIO 2` | Output | Onboard Blue LED | High = ON (5s heartbeat / 1s error) |
| `PIN_ENCODER_CLK` | `GPIO 32` | Input Pullup | KY-040 CLK (A) | Hardware interrupt attached |
| `PIN_ENCODER_DT` | `GPIO 33` | Input Pullup | KY-040 DT (B) | Quadrature direction read |
| `PIN_ENCODER_SW` | `GPIO 25` | Input Pullup | KY-040 Switch | Active LOW. Short: Mute, Long: Pair |
| `PIN_I2S_BCLK` | `GPIO 26` | Output | CJMCU-5102 BCK | I2S Bit Clock |
| `PIN_I2S_LRCK` | `GPIO 27` | Output | CJMCU-5102 LCK | I2S Word Select / Left-Right Clock |
| `PIN_I2S_DOUT` | `GPIO 22` | Output | CJMCU-5102 DIN | I2S Serial PCM Data Out |
| `PIN_TFT_CS` | `GPIO 5` | Output | 1.8" SPI TFT CS | Chip Select |
| `PIN_TFT_DC` | `GPIO 16` | Output | 1.8" SPI TFT DC / A0 | Data / Command Select |
| `PIN_TFT_RST` | `GPIO 17` | Output | 1.8" SPI TFT RST | Hardware Reset |
| `PIN_TFT_MOSI` | `GPIO 23` | Output | 1.8" SPI TFT SDA | Shared SPI MOSI |
| `PIN_TFT_SCLK` | `GPIO 18` | Output | 1.8" SPI TFT SCK | Shared SPI Clock |
| `PIN_SD_CS` | `GPIO 4` | Output | MicroSD SPI CS | Dedicated SD Chip Select |
| `PIN_SD_MOSI` | `GPIO 23` | Output | MicroSD SPI MOSI | Shared SPI MOSI |
| `PIN_SD_MISO` | `GPIO 19` | Input Pullup | MicroSD SPI MISO | SPI Master-In Slave-Out |
| `PIN_SD_SCK` | `GPIO 18` | Output | MicroSD SPI SCK | Shared SPI Clock |

---

## 2. Power Requirements

- **ESP32-WROOM-32**: 5V via USB Micro-B or 5V pin (drawing up to 500mA peak with Wi-Fi & BT active).
- **CJMCU-5102 (PCM5102A)**: 3.3V or 5V (onboard 3.3V regulator). Connect SCK to GND to activate internal PLL.
- **3× Gold A542 Amplifiers**: 12V–24V DC dedicated power supply. Common ground with ESP32 DAC GND.
- **1.8" SPI TFT**: 3.3V VCC. Backlight (LED/BL) to 3.3V.
