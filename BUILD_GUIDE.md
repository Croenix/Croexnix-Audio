# Croenix Audio Player — Hardware Modules & GPIO Guide

---

## 📦 1. Hardware Modules Overview

| Module | Chipset / Model | Interface | Operating Voltage | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| **Microcontroller** | ESP32-WROOM-32 (38-pin / 30-pin) | Dual-core Tensilica Xtensa 240MHz | 5V (USB / VIN) / 3.3V logic | Main system controller, audio decoder, Wi-Fi web & WebSocket server |
| **I²S Audio DAC** | CJMCU-5102 (PCM5102A) | I²S Bus (BCK, LCK, DIN) | 5V VIN (onboard 3.3V LDO) | 32-bit / 384kHz Hi-Fi stereo DAC with line-out audio jack |
| **Storage** | MicroSD Card Module | Hardware SPI (shared bus) | 5V VCC (onboard 3.3V LDO) | High-speed FAT32 audio file storage (MP3, WAV, FLAC, AAC) |
| **Display** | ST7735 1.8" Color TFT | Hardware SPI (shared bus) | 3.3V VCC & Backlight | 160×128 landscape cyberpunk graphical interface & visualizer |
| **Rotary Encoder** | KY-040 | Digital GPIO with Pull-ups | 3.3V VCC | Hardware volume & track control with integrated push-button |

---

## 🔌 2. Master GPIO Pin Connection Table

All peripherals have been mapped without any pin contention or bus collision:

| ESP32 Pin | Connected Module | Module Pin | Pin Function / Signal | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **GPIO 26** | CJMCU-5102 DAC | **BCK** | I²S Bit Clock | Dedicated audio clock |
| **GPIO 22** | CJMCU-5102 DAC | **LCK** | I²S Left/Right Word Select Clock | Dedicated audio sync |
| **GPIO 27** | CJMCU-5102 DAC | **DIN** | I²S Serial Audio Data | Dedicated audio data |
| **GPIO 23** | MicroSD & ST7735 TFT | **MOSI / SDA** | SPI Master Out Slave In | Shared SPI Data line |
| **GPIO 18** | MicroSD & ST7735 TFT | **SCK / SCL** | SPI Serial Clock | Shared SPI Clock line |
| **GPIO 19** | MicroSD Module | **MISO / DO** | SPI Master In Slave Out | Dedicated SD read line |
| **GPIO 15** | MicroSD Module | **CS** | SD Chip Select | Active LOW |
| **GPIO 5** | ST7735 TFT | **CS** | Display Chip Select | Active LOW |
| **GPIO 2** | ST7735 TFT | **DC (A0 / RS)** | Display Data / Command | Command: LOW, Data: HIGH |
| **GPIO 4** | ST7735 TFT | **RST** | Display Hardware Reset | Active LOW |
| **GPIO 32** | KY-040 Encoder | **CLK** | Rotary Quadrature Clock | Internal PULLUP enabled |
| **GPIO 33** | KY-040 Encoder | **DT** | Rotary Quadrature Data | Internal PULLUP enabled |
| **GPIO 34** | KY-040 Encoder | **SW** | Push Button Switch | Input only, active LOW |

---

## ⚡ 3. Power Distribution Rail

```
                 ┌───────────────────────────────────────────────────────────┐
                 │                 ESP32 Power Rail                          │
                 └───────┬───────────────────────────────────┬───────────────┘
                         │ 5V Rail                           │ 3.3V Rail
                         ▼                                   ▼
          ┌──────────────────────────────┐    ┌──────────────────────────────┐
          │ • CJMCU-5102 DAC (VIN)       │    │ • ST7735 TFT Display (VCC)   │
          │ • MicroSD Card Module (VCC)  │    │ • ST7735 TFT Backlight (BL)  │
          └──────────────────────────────┘    │ • KY-040 Rotary Encoder (VCC)│
                                              └──────────────────────────────┘
                         │                                   │
                         └─────────────────┬─────────────────┘
                                           ▼
                                 COMMON GROUND (GND)
                  (Connect all module GND pins to ESP32 GND)
```

> **⚠️ CRITICAL POWER RULE:**
> - Connect both **CJMCU-5102** and the **MicroSD Module** to **5V**, NOT 3.3V. Both modules have onboard AMS1117-3.3V regulators that drop 5V to a clean, isolated 3.3V line. Powering them from the ESP32's 3.3V rail will cause brownouts and SD mount failures.

---

## 🔍 4. Module-by-Module Wiring Details

### A. CJMCU-5102 (PCM5102A) I²S DAC Module
```
ESP32 Pin                CJMCU-5102 Module
──────────────────────────────────────────
5V (VIN)         ─────►  VIN
GND              ─────►  GND
GPIO 26          ─────►  BCK  (Bit Clock)
GPIO 22          ─────►  LCK  (Left/Right Word Select Clock)
GPIO 27          ─────►  DIN  (Digital Audio Data)
```

#### ⚠️ Mandatory Solder Jumper Configuration
The CJMCU-5102 module has **4 solder jumpers** on the back/top. If these are not configured, audio will be completely silent or muted:

```
┌──────────────────────────────────────────────────┐
│              CJMCU-5102 Solder Jumpers           │
│                                                  │
│   H1L (System Clock / SCK):  [L]●─●[ ]  (MUST solder to L / GND)
│   H2L (BCK Clock Source):    [ ]   [ ]  (LEAVE OPEN / unsoldered)
│   H3  (Soft Mute / XSMT):    [ ]─●[H]●  (MUST solder to H / VCC)
│   H4  (De-emphasis Filter):  [L]●─●[ ]  (Solder to L / Normal)
└──────────────────────────────────────────────────┘
```
1. **H1L:** Solder bridge to **L** (connects SCK to GND; triggers internal PLL to derive master clock from BCK).
2. **H2L:** Leave **OPEN** (BCK is provided externally by ESP32 GPIO 26).
3. **H3:** Solder bridge to **H** (pulls XSMT to 3.3V, disabling hardware mute).
4. **H4:** Solder bridge to **L** (selects normal latency digital filter).

---

### B. MicroSD Card Module (SPI)
```
ESP32 Pin                MicroSD Module
──────────────────────────────────────────
5V               ─────►  VCC  (Uses onboard 3.3V LDO)
GND              ─────►  GND
GPIO 23          ─────►  MOSI / DI  (Shared with TFT)
GPIO 19          ─────►  MISO / DO  (Dedicated SD data line)
GPIO 18          ─────►  SCK / CLK  (Shared with TFT)
GPIO 15          ─────►  CS   (Chip Select)
```

- **Card Requirement:** MicroSD / MicroSDHC formatted to **FAT32** (32GB or smaller recommended).

---

### C. ST7735 1.8" Color TFT Display (160×128 SPI)
```
ESP32 Pin                ST7735 TFT Module
──────────────────────────────────────────
3.3V             ─────►  VCC
GND              ─────►  GND
GPIO 23          ─────►  SDA / MOSI  (Shared with SD)
GPIO 18          ─────►  SCL / SCK   (Shared with SD)
GPIO 5           ─────►  CS
GPIO 2           ─────►  DC / A0 / RS
GPIO 4           ─────►  RES / RST
3.3V             ─────►  BLK / BL / LED (Backlight)
```

---

### D. KY-040 Rotary Encoder (Optional)
```
ESP32 Pin                KY-040 Encoder
──────────────────────────────────────────
3.3V             ─────►  VCC / +
GND              ─────►  GND
GPIO 32          ─────►  CLK
GPIO 33          ─────►  DT
GPIO 34          ─────►  SW
```
- **Turning:** Advances to Next or Previous Track.
- **Clicking:** Toggles Play / Pause.
