# Croenix Audio - Complete Build Guide

## Hardware Setup

### Components Purchased
✅ ESP32-WROOM-32 (38-pin)
✅ CJMCU-5102 I²S DAC
✅ MicroSD SPI reader
✅ Si4703 FM module
✅ 1.8" TFT LCD (ST7735)
✅ Gold A542 Class-D amplifier

### Wiring Diagram

#### I2S DAC (CJMCU-5102)
```
ESP32 GPIO 26 → BCK (Bit Clock)
ESP32 GPIO 25 → WS/LRCK (Word Select)
ESP32 GPIO 22 → DATA
ESP32 3.3V → VCC
ESP32 GND → GND
DAC LOUT → Amplifier Left Input
DAC ROUT → Amplifier Right Input
```

#### SPI Display (ST7735 1.8" TFT)
```
ESP32 GPIO 23 → MOSI
ESP32 GPIO 18 → SCK
ESP32 GPIO 5 → CS
ESP32 GPIO 2 → DC/RS
ESP32 GPIO 4 → RST
ESP32 3.3V → VCC
ESP32 3.3V → LED (with resistor)
ESP32 GND → GND
```

#### SD Card Reader (SPI - shared bus)
```
ESP32 GPIO 23 → MOSI (shared with TFT)
ESP32 GPIO 19 → MISO
ESP32 GPIO 18 → SCK (shared with TFT)
ESP32 GPIO 15 → CS
ESP32 3.3V → VCC
ESP32 GND → GND
```

#### FM Radio (Si4703 I²C)
```
ESP32 GPIO 21 → SDA
ESP32 GPIO 22 → SCL
ESP32 GPIO 16 → RST
ESP32 3.3V → VCC
ESP32 GND → GND
```

#### Amplifier (Gold A542)
```
DAC LOUT → AMP Left Input
DAC ROUT → AMP Right Input
12V Power Supply → AMP VCC
GND → AMP GND
AMP Output → Speakers
```

---

## ESP32 Firmware Installation

### Prerequisites
- Install [PlatformIO](https://platformio.org/) extension in VS Code
- USB cable for ESP32
- CH340/CP2102 drivers (if needed)

### Steps

1. **Connect ESP32 via USB**
```bash
# Windows: Check device manager for COM port
# Linux/Mac: Check /dev/ttyUSB* or /dev/cu.usbserial*
```

2. **Open Project**
```bash
cd esp32_firmware
```

3. **Build Firmware**
```bash
pio run
```

4. **Upload to ESP32**
```bash
pio run --target upload
```

5. **Monitor Serial Output**
```bash
pio device monitor
```

### Expected Serial Output
```
===========================================
ESP32 High-Fidelity Audio Player
===========================================

[INIT] Creating FreeRTOS tasks...
[AUDIO TASK] Starting on Core 0...
[NETWORK TASK] Starting on Core 0...
[DISPLAY TASK] Starting on Core 1...
[CONTROL TASK] Starting on Core 1...
[WIFI] No saved credentials found
[WIFI] Starting in Access Point mode...
[WIFI] AP started successfully!
[WIFI] SSID: ESP32-Audio-Setup
[WIFI] IP Address: 192.168.4.1
[WEBSOCKET] Server started on port 8080
[DISPLAY] TFT initialized successfully
[FM] Si4703 initialized successfully
[SD] Card mount failed!  // Normal if no SD card inserted
[AUDIO] I2S initialized successfully
```

---

## Flutter App Installation

### Android

1. **Install Flutter**
```bash
# Install Flutter SDK
# https://docs.flutter.dev/get-started/install

flutter doctor
```

2. **Setup Project**
```bash
cd croenix_app
flutter pub get
```

3. **Connect Android Phone**
- Enable Developer Options
- Enable USB Debugging
- Connect via USB

4. **Run App**
```bash
flutter devices  # Check connected devices
flutter run
```

5. **Build APK** (optional)
```bash
flutter build apk --release
# APK location: build/app/outputs/flutter-apk/app-release.apk
```

### iOS

1. **Prerequisites**
- macOS with Xcode installed
- Apple Developer account (for deployment)

2. **Setup**
```bash
cd croenix_app
flutter pub get
cd ios
pod install
```

3. **Run on Simulator/Device**
```bash
flutter run
```

---

## First-Time Setup Walkthrough

### Step 1: Power On ESP32
1. Connect ESP32 to 5V power supply
2. Display should show "CROENIX" splash screen
3. After 2 seconds, main screen appears with "AP Mode" indicator

### Step 2: Connect Mobile App to ESP32
1. Launch Croenix Audio app on phone
2. App shows "Setup Device" screen
3. Wait for device discovery (scans for `ESP32-Audio-Setup`)
4. If not found, connect phone to Wi-Fi: **ESP32-Audio-Setup** (no password)
5. Tap on discovered device to connect

### Step 3: Configure Wi-Fi
1. App shows available Wi-Fi networks
2. Select your home network
3. Enter password
4. Tap "Connect to Network"
5. ESP32 reboots and connects to home Wi-Fi
6. App automatically reconnects via mDNS (`esp32-audio.local`)

### Step 4: Test Audio Playback

#### Test 1: SD Card
1. Insert microSD card with MP3/FLAC files
2. Open app → Player screen
3. Tap "Source" → Select "SD Card"
4. Browse files and select a track
5. Press play

#### Test 2: Bluetooth
1. Open app → Player screen
2. Tap "Source" → Select "Bluetooth"
3. On phone, go to Bluetooth settings
4. Pair with "ESP32-Audio-Player"
5. Play music from any app (Spotify, YouTube, etc.)

#### Test 3: FM Radio
1. Connect FM antenna to Si4703 module
2. Open app → Player screen
3. Tap "Source" → Select "FM Radio"
4. Use frequency tuner to find stations
5. Save favorite stations as presets

#### Test 4: Web Streaming
1. Open app → Player screen
2. Tap "Source" → Select "Web Stream"
3. Enter streaming URL (e.g., internet radio)
4. Press play

---

## Troubleshooting

### ESP32 Won't Upload
- Check USB cable (must support data)
- Hold BOOT button while uploading
- Install CH340 drivers
- Try different COM port

### Display Not Working
- Check SPI connections
- Verify 3.3V power supply
- Check TFT_CS, TFT_DC, TFT_RST pins
- Test with example code first

### No Audio Output
- Check I2S wiring (BCK, WS, DATA)
- Verify DAC power supply (3.3V)
- Check amplifier connections
- Test DAC with headphones first

### SD Card Not Detected
- Use FAT32 formatted card
- Check SPI connections
- Verify CS pin (GPIO 15)
- Try different SD card

### FM Radio No Signal
- Connect proper antenna (dipole or whip)
- Check I2C connections (SDA, SCL)
- Verify Si4703 power
- Move away from interference sources

### App Can't Find Device
- Check ESP32 is powered on
- Verify phone on same Wi-Fi network
- Try manual IP entry
- Check firewall settings
- Restart both devices

### Bluetooth Pairing Issues
- Clear Bluetooth pairing history
- Restart ESP32
- Re-pair device
- Check Bluetooth MAC address

---

## Advanced Configuration

### Adjust Audio Quality
Edit `esp32_firmware/src/core/config.h`:
```cpp
#define AUDIO_SAMPLE_RATE    48000  // Change from 44100
#define AUDIO_BITS_PER_SAMPLE  24   // Change from 16
```

### Change Wi-Fi AP Settings
```cpp
#define AP_SSID           "MyCustomName"
#define AP_PASSWORD       "mypassword"  // Add password
```

### Customize Display Refresh Rate
```cpp
#define DISPLAY_UPDATE_RATE  50  // 50ms (faster updates)
```

### Enable Debug Logging
```cpp
#define DEBUG_SERIAL      true
#define DEBUG_BAUD_RATE   115200
```

---

## Network Diagram

```
[Mobile Phone] <--WiFi--> [Home Router]
                               ^
                               |
                          WiFi/mDNS
                               |
                          [ESP32 Device]
                               |
                         WebSocket (Port 8080)
                               |
                          Local IP/mDNS
                       esp32-audio.local
```

## System Architecture

```
┌─────────────────────────────────────┐
│         Mobile App (Flutter)        │
│  ┌───────────────────────────────┐  │
│  │   Home | Player | Settings    │  │
│  │   Glassmorphism UI             │  │
│  └───────────────┬─────────────── ┘  │
│                  │ WebSocket          │
│         ┌────────┴────────┐          │
│         │  mDNS Discovery  │          │
│         └─────────────────┘          │
└──────────────────┬──────────────────┘
                   │
         Network (WiFi/AP)
                   │
┌──────────────────┴──────────────────┐
│          ESP32 Firmware             │
│                                      │
│  Core 0:  │  Core 1:                │
│  ─────────┤  ─────────              │
│  Audio    │  Display                │
│  Network  │  Control                │
│           │  Peripherals            │
│                                      │
│  ┌──────┐  ┌────┐  ┌──┐  ┌────┐   │
│  │ I2S  │  │TFT │  │SD│  │ FM │   │
│  │ DAC  │  │LCD │  │  │  │Radio│  │
│  └──┬───┘  └────┘  └──┘  └────┘   │
└─────┼──────────────────────────────┘
      │
  ┌───┴───┐
  │  AMP  │
  └───┬───┘
      │
 ┌────┴────┐
 │ Speakers│
 └─────────┘
```

---

## Next Steps

1. ✅ Complete hardware assembly
2. ✅ Flash ESP32 firmware
3. ✅ Install mobile app
4. ✅ Test all audio sources
5. ⬜ Add album art support
6. ⬜ Implement playlist management
7. ⬜ Add OTA firmware updates
8. ⬜ Create custom case/enclosure

## Support

For issues and questions:
- Check serial monitor output
- Review pin connections
- Test components individually
- Read component datasheets

## License

MIT License - See LICENSE file
