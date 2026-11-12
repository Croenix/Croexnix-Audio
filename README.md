# ESP32 High-Fidelity Audio Player & Smart Mobile Companion

## System Overview

A low-latency, high-fidelity wireless audio playback system featuring:
- ESP32-WROOM-32 microcontroller with dual-core FreeRTOS architecture
- CJMCU-5102 I²S DAC for high-quality audio output
- Si4703 FM radio module
- 1.8" TFT LCD display (ST7735)
- MicroSD card support for local playback
- Flutter mobile app with glassmorphism UI

## Hardware Components

| Component | Quantity | Status |
|-----------|----------|--------|
| ESP32-WROOM-32 (38-pin) | 1 | ✅ Purchased |
| CJMCU-5102 I²S DAC | 1 | ✅ Purchased |
| MicroSD SPI reader | 1 | ✅ Purchased |
| Si4703 FM module | 1 | ✅ Purchased |
| 1.8" TFT LCD | 1 | ✅ Purchased |
| Gold A542 Class-D amplifier | 1 | ✅ Purchased |

## Project Structure

```
├── esp32_firmware/          # ESP32 firmware (PlatformIO)
│   ├── src/
│   │   ├── main.cpp
│   │   ├── audio/           # Audio pipeline & codecs
│   │   ├── network/         # Wi-Fi, WebSocket, mDNS
│   │   ├── display/         # TFT display driver
│   │   ├── peripherals/     # FM radio, SD card
│   │   └── core/            # FreeRTOS tasks
│   └── platformio.ini
│
└── croenix_app/             # Flutter mobile app
    ├── lib/
    │   ├── core/
    │   ├── features/
    │   └── data/
    └── pubspec.yaml
```

## Key Features

### ESP32 Firmware
- **Dual-Core Architecture**: Core 0 handles audio/network, Core 1 handles UI/control
- **Audio Sources**: Bluetooth A2DP, SD Card (FLAC/WAV/MP3), FM Radio, Web Streaming
- **Zero-Latency Audio**: Hardware I2S DMA with uncompressed audio passthrough
- **Wi-Fi Provisioning**: Open SoftAP for first-time setup + dual STA/AP mode
- **Real-time Metadata**: ID3 tags, RDS, track info on display

### Mobile App
- **Instant Pairing**: Auto-discovery via mDNS (esp32-audio.local)
- **Glassmorphism UI**: Modern translucent design with smooth animations
- **Remote Control**: Volume, EQ, source switching, file browsing
- **FM Manager**: Frequency tuning, station presets, auto-scan
- **Web FM**: Custom streaming URL configuration

## Technical Stack

| Component | Technology |
|-----------|-----------|
| Firmware Platform | ESP-IDF / Arduino Framework with FreeRTOS |
| Audio Output | Dual-Channel I2S DMA to CJMCU-5102 DAC |
| Communication | WebSocket over TCP with JSON-RPC |
| Display Protocol | Hardware SPI (ST7735) |
| Storage | NVS Flash + MicroSD Card |
| Mobile App | Flutter (Android/iOS) |

## Pin Configuration

### I2S DAC (CJMCU-5102)
- BCK (Bit Clock): GPIO 26
- WS (Word Select): GPIO 25
- DATA: GPIO 22
- VCC: 3.3V
- GND: GND

### SPI Display (ST7735 1.8" TFT)
- MOSI: GPIO 23
- SCK: GPIO 18
- CS: GPIO 5
- DC: GPIO 2
- RST: GPIO 4
- VCC: 3.3V
- GND: GND

### SD Card Reader (SPI)
- MOSI: GPIO 23 (shared)
- MISO: GPIO 19
- SCK: GPIO 18 (shared)
- CS: GPIO 15
- VCC: 3.3V
- GND: GND

### FM Radio (Si4703 I2C)
- SDA: GPIO 21
- SCL: GPIO 22
- RST: GPIO 16
- VCC: 3.3V
- GND: GND

## Getting Started

### ESP32 Firmware Setup
```bash
cd esp32_firmware
pio run --target upload
pio device monitor
```

### Flutter App Setup
```bash
cd croenix_app
flutter pub get
flutter run
```

## Development Phases

1. ✅ **Phase 1**: Project structure and hardware integration
2. 🔄 **Phase 2**: Audio pipeline implementation
3. ⏳ **Phase 3**: Network provisioning and communication
4. ⏳ **Phase 4**: Mobile app development
5. ⏳ **Phase 5**: Testing and optimization

## License

MIT License - see LICENSE file for details
