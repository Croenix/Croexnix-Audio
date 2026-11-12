# Croenix Audio - Complete Build & Integration Guide
**Version 3.0 (Production Release)**  
High-Performance, Low-Memory Embedded Architecture for ESP32-WROOM-32 & Flutter Mobile Companion

---

## 1. Hardware Specifications & Components

| Component | Part / Model | Purpose / Interface |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32-WROOM-32 (38-Pin DevKit) | Dual-core Tensilica Xtensa 240 MHz, 520 KB SRAM |
| **I²S Hi-Fi DAC** | CJMCU-5102 / WCMCU (TI PCM5102A) | 32-bit / 384 kHz Stereo Audio DAC via I²S DMA |
| **SPI Display** | 1.8" Color TFT LCD (ST7735) | 160x128 SPI color screen with cyber glassmorphism UI |
| **MicroSD Reader** | MicroSD SPI Card Reader Module | FAT32 Local MP3 / FLAC / AAC audio storage |
| **FM Radio Tuner** | HW-322 (Silicon Labs Si4703) | 87.5–108 MHz RDS/FM receiver via I²C bus |
| **Rotary Controller**| KY-040 5-Pin Rotary Encoder | Quadrature input with push-button interrupt logic |
| **Audio Amplifier** | Gold A542 / TPA3116D2 Class-D | Stereo speaker drive |

---

## 2. Complete Hardware Wiring & Pin Mapping

> [!IMPORTANT]
> **GPIO 22 vs GPIO 27 Resolution:**  
> The Si4703 FM module's I²C clock (**SCL**) is assigned to **GPIO 27** to guarantee zero bus contention with the PCM5102A DAC's Word Select Clock (**LCK**) on **GPIO 22**.

### A. I²S DAC (TI PCM5102A / CJMCU-5102)

| PCM5102A Pin | ESP32 Pin / Rail | Function | Notes |
| :--- | :--- | :--- | :--- |
| **VCC** | **VIN (5V)** or **3.3V** | Power rail | Connect to 5V (if module has 3.3V LDO) or 3.3V |
| **GND** | **GND** | Power ground | Common system ground |
| **BCK** | **GPIO 26** | I²S Bit Clock | Hardware Bit Clock |
| **DIN** | **GPIO 25** | I²S Serial Data | Audio data payload |
| **LCK** | **GPIO 22** | I²S Word Select (`WS` / `LRCK`) | Left/Right frame clock |
| **SCK / SCL** | **GND** | Internal PLL Master Clock | **CRITICAL:** Tie to GND so PLL derives system clock from BCK |
| **XMT** | **3.3V** | Soft-Unmute / Audio Enable | **CRITICAL:** Tie to 3.3V (HIGH) to unmute audio output |
| **LOUT** | **AMP Left In** | Analog Left Channel | Route to Class-D amplifier |
| **ROUT** | **AMP Right In** | Analog Right Channel | Route to Class-D amplifier |

### B. 1.8" ST7735 TFT Display (SPI)

| ST7735 Pin | ESP32 Pin | Function / Description |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | Display logic power |
| **GND** | **GND** | Display ground |
| **CS** | **GPIO 5** | Dedicated Display Chip Select |
| **RESET / RST** | **GPIO 4** | Hardware display reset |
| **A0 / DC** | **GPIO 2** | Data / Command register select |
| **SDA / MOSI** | **GPIO 23** | SPI Master-Out-Slave-In (Shared bus) |
| **SCK / SCL** | **GPIO 18** | SPI Bus Clock (Shared bus) |
| **LED / BL** | **3.3V** | Backlight anode (direct or via 100Ω resistor) |

### C. MicroSD Card Adapter (SPI - Shared Bus with TFT)

| MicroSD Module Pin | ESP32 Pin | Function / Description |
| :--- | :--- | :--- |
| **VCC** | **5V** or **3.3V** | Power (Check module regulator: 5V for buffered boards) |
| **GND** | **GND** | Common ground |
| **CS** | **GPIO 15** | Dedicated SD Card Chip Select |
| **MOSI / DI** | **GPIO 23** | SPI Master-Out-Slave-In (Shared with TFT) |
| **SCK / CLK** | **GPIO 18** | SPI Bus Clock (Shared with TFT) |
| **MISO / DO** | **GPIO 19** | SPI Master-In-Slave-Out (Internal pull-up enabled) |

### D. HW-322 FM Radio (Si4703 - I²C)

| Si4703 Pin | ESP32 Pin | Function / Description |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | Module logic power |
| **GND** | **GND** | Common ground |
| **SDIO / SDA** | **GPIO 21** | I²C Data line |
| **SCLK / SCL** | **GPIO 27** | I²C Clock line (Assigned to 27) |
| **RST** | **GPIO 16** | Hardware chip reset |
| **ANT** | **External Wire** | 75 cm wire or telescopic FM antenna |

### E. KY-040 Rotary Encoder

| KY-040 Pin | ESP32 Pin | Function / Description |
| :--- | :--- | :--- |
| **CLK (A)** | **GPIO 32** | Quadrature interrupt channel A |
| **DT (B)** | **GPIO 33** | Quadrature interrupt channel B |
| **SW (Key)** | **GPIO 17** | Push-button state switch interrupt |
| **+ / VCC** | **3.3V** | Pull-up supply |
| **GND** | **GND** | Common ground |

---

## 3. Rotary Encoder Navigation & Diagnostics State Machine

The firmware uses interrupt-driven quadrature decoding with a hardware debounce state machine:

1. **Volume Adjustment (Default):**
   - Rotating clockwise/counter-clockwise adjusts volume (0% to 100%).
   - Instantly triggers a temporary glassmorphism Volume HUD overlay on the display without interrupting audio playback.
2. **Input Mode Cycling (Single Click):**
   - Single-pressing the encoder button cycles sequentially through input modes:
     `Bluetooth (BT)` → `SD Card` → `FM Radio` → `SoundCloud Web Stream`.
3. **FM Tuning Override (In FM Mode):**
   - In FM mode, pressing the encoder opens a **5-second Tuning Window**.
   - During these 5 seconds, rotating the encoder steps or seeks radio frequencies.
   - After 5 seconds of inactivity, the encoder automatically reverts to volume control.
4. **Hardware Diagnostic Portal (10-Second Long Press):**
   - Long-pressing the encoder button for 10 seconds from any screen enters the **Hardware Status & Diagnostics Portal**.
   - Allows live status checks of:
     1. **WCMCU I²S DAC** (TI PCM5102A DMA streaming integrity, sampling rate, gain)
     2. **HW-322 FM Radio** (Si4703 I²C bus probe, frequency, RSSI)
     3. **SD Card Reader** (Filesystem mount, card capacity, track count, FAT32 validation)
     4. **System & Display** (Core 0/1 CPU load, FreeRTOS heap memory, WiFi telemetry)

---

## 4. ESP32 Firmware Installation

### Prerequisites & Library Dependencies

Ensure the following libraries are installed in Arduino IDE (**Tools → Manage Libraries**):
* `TFT_eSPI` by Bodmer
* `ESP32-audioI2S` by schreibfaul1
* `PU2CLR_SI470X` by Ricardo Lima Caratti
* `ArduinoJson` (v6.x or v7.x) by Benoit Blanchon
* `WebSockets` by Markus Sattler

### Configuring TFT_eSPI
Edit `Arduino/libraries/TFT_eSPI/User_Setup.h`:
```cpp
#define ST7735_DRIVER
#define TFT_WIDTH  128
#define TFT_HEIGHT 160
#define ST7735_BLACKTAB

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
```

### Arduino IDE Build & Upload Steps

1. Open [`esp32_firmware/Croenix_Audio_Firmware/Croenix_Audio_Firmware.ino`](file:///c:/Users/achub/OneDrive/Documents/Music%20Player/esp32_firmware/Croenix_Audio_Firmware/Croenix_Audio_Firmware.ino).
2. Go to **Tools → Board → ESP32 Arduino → ESP32 Dev Module**.
3. **CRITICAL SETTING:** Go to **Tools → Partition Scheme** and select:
   ```
   Huge APP (3MB No OTA/1MB SPIFFS)
   ```
   *(Selecting the default partition will cause a flash overflow error because the firmware with FreeRTOS, I²S audio streaming, Wi-Fi, and WebSockets requires ~2.06 MB).*
4. Select your COM Port and click **Upload**.

---

## 5. Flutter Companion Application Setup

The Flutter application provides a cyber glassmorphism interface with zero-latency WebSocket control, live ID3 metadata streaming, and a dedicated SoundCloud continuous playlist engine.

### Setup & Run

1. Open a terminal in the application folder:
   ```bash
   cd croenix_app
   flutter pub get
   ```
2. Verify code integrity:
   ```bash
   flutter analyze
   ```
3. Run on connected Android / iOS device or emulator:
   ```bash
   flutter run
   ```
4. Build release APK for Android:
   ```bash
   flutter build apk --release
   ```
   *Output binary location: `croenix_app/build/app/outputs/flutter-apk/app-release.apk`*

---

## 6. Zero-Handoff QR Provisioning Protocol

1. **Unprovisioned Boot:**
   - If no Wi-Fi credentials are stored in NVS, the ESP32 hosts a secure Access Point:
     - **SSID:** `CROENIX-AUDIO-SETUP`
     - **Static IP:** `192.168.4.1`
   - The ST7735 display renders a dynamic QR code containing encrypted connection parameters.
2. **Mobile Scan & Sync:**
   - Open the Croenix app and tap **Scan QR Code**.
   - Point the camera at the TFT display. The app automatically connects to the AP, transmits your home Wi-Fi credentials, and triggers the ESP32 to transition to Station Mode.
3. **mDNS & WebSocket Reconnect:**
   - Once connected to your router, the ESP32 broadcasts via mDNS at `croenix-audio.local`.
   - The mobile app establishes a bidirectional WebSocket connection on port `8080` (`ws://croenix-audio.local:8080`).

---

## 7. SoundCloud Playlist Streaming Engine

The application features a dedicated SoundCloud streaming suite:
* **Curated Continuous Channels:**
  - *Synthwave / Retro Electro*
  - *Lofi Chillhop Beats*
  - *Cyberpunk Industrial Bass*
  - *Melodic Deep House*
  - *Drum & Bass Energy*
  - *Acoustic Indie Sessions*
* **"Play All Songs" Continuous Engine:**
  - Tap **Play All Songs** in the SoundCloud tab to start sequential, gapless playback.
  - The firmware automatically requests the next track upon completion, broadcasting real-time ID3 tags (Track Title, Artist, Duration, Bitrate) to both the TFT display and mobile app.
* **Custom Stream Input:**
  - Enter any direct audio stream or SoundCloud feed URL in the app to route playback through the PCM5102A I²S DAC.

---

## 8. Hardware Diagnostics & Troubleshooting

### Display Stuck on "CROENIX AUDIOS" Splash Screen
* **Cause:** The FM radio module probe hung on I²C clock-stretching.
* **Solution:** Firmware v3.0 includes `Wire.setTimeOut(20)` non-blocking bus probing. If no Si4703 is detected, it cleanly disables FM radio and boots immediately to `UI_SCREEN_HOME`.

### PCM5102A DAC Has Only Red LED / No Sound
* **Check Jumpers:**
  1. `SCK / SCL` must be tied directly to **GND** (enables internal PLL from BCK).
  2. `XMT` must be tied directly to **3.3V** (disables soft mute).
  3. `FLT`, `DMP`, `FMT` must be tied to **GND**.
* **Check Wiring:**
  - `BCK` → **GPIO 26**
  - `DIN` → **GPIO 25**
  - `LCK` → **GPIO 22**

### MicroSD Card Mount Failed
* Format the MicroSD card as **FAT32** (exFAT and NTFS are not supported).
* Use cards 32 GB or smaller for standard SPI compatibility.
* Ensure `MISO` (GPIO 19) is firmly connected; the firmware enables internal pull-ups on this pin.
* Long-press the rotary encoder for 10 seconds to inspect the SD Card diagnostic card on the TFT screen.

### Compilation Fails ("Text Section Exceeds Space")
* In Arduino IDE, change **Tools → Partition Scheme** to **Huge APP (3MB No OTA/1MB SPIFFS)**.

---

## 9. Firmware Multi-Core Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                 ESP32 DUAL-CORE ARCHITECTURE                │
├──────────────────────────────┬──────────────────────────────┤
│    CORE 1: DEDICATED AUDIO   │    CORE 0: CONTROL & COMM    │
├──────────────────────────────┼──────────────────────────────┤
│ • I²S DMA Buffer Streaming   │ • Non-blocking ST7735 SPI UI │
│ • MP3/FLAC/AAC Audio Decoder │ • Wi-Fi Station & SoftAP     │
│ • Zero Latency Buffer Loop   │ • WebSocket Server (8080)    │
│ • Uninterrupted Hi-Fi Sound  │ • Rotary Encoder ISR Logic   │
│                              │ • mDNS Discovery Service     │
└──────────────────────────────┴──────────────────────────────┘
```
