# Quick Start: Arduino IDE Flash Guide

## 🚀 Fast Track (5 Steps)

### 1. Install Arduino IDE
Download from: https://www.arduino.cc/en/software

### 2. Add ESP32 Support
- Open Arduino IDE → `File` → `Preferences`
- Paste this URL into "Additional Boards Manager URLs":
  ```
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
  ```
- Click OK
- Go to `Tools` → `Board` → `Boards Manager`
- Search "esp32" → Install **"esp32 by Espressif Systems"**

### 3. Install Libraries
Go to `Tools` → `Manage Libraries`, search and install:
- ✅ **ESP32-audioI2S** (by schreibfaul1)
- ✅ **TFT_eSPI** (by Bodmer)
- ✅ **ArduinoJson** (by Benoit Blanchon) v6.x
- ✅ **WebSockets** (by Markus Sattler)
- ✅ **SparkFun Si4703 Arduino Library**

### 4. Convert Project
**Windows**: Double-click `convert_to_arduino.bat`
**Mac/Linux**: Run `./convert_to_arduino.sh`

This creates folder: `Croenix_Audio_Firmware_Arduino`

### 5. Configure & Upload
1. Open `Croenix_Audio_Firmware_Arduino.ino` in Arduino IDE
2. `Tools` → `Board` → **"ESP32 Dev Module"**
3. `Tools` → `Port` → Select your ESP32 (COM3, COM4, etc.)
4. Click **Upload** (→ button)
5. If upload fails: **Hold BOOT button** during upload

---

## 📋 TFT Display Configuration (REQUIRED)

After installing TFT_eSPI library:

1. Find library folder:
   - **Windows**: `C:\Users\YOUR_NAME\Documents\Arduino\libraries\TFT_eSPI`
   - **Mac**: `~/Documents/Arduino/libraries/TFT_eSPI`

2. Open `User_Setup.h`

3. **Replace entire file** with this:

```cpp
#define ST7735_DRIVER
#define TFT_WIDTH 128
#define TFT_HEIGHT 160
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 2
#define TFT_RST 4
#define SPI_FREQUENCY 27000000
#define TFT_INVERSION_OFF
#define TFT_RGB_ORDER TFT_BGR
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SMOOTH_FONT
```

4. Save and restart Arduino IDE

---

## 🔧 Troubleshooting

### Upload Failed
- **Hold BOOT button** on ESP32 while clicking Upload
- When you see "Connecting..." release BOOT

### Port Not Found
- Install USB driver:
  - **CP210x**: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
  - **CH340**: Google "CH340 driver download"

### Compilation Error
- Check all 5 libraries installed
- Restart Arduino IDE
- Verify TFT_eSPI configured

### "Sketch Too Big"
- `Tools` → `Partition Scheme` → **"Huge APP (3MB)"**

---

## ✅ Success Indicators

After upload:
1. Open Serial Monitor (`Ctrl+Shift+M`)
2. Set baud rate: **115200**
3. Press **RESET** on ESP32
4. You should see:
   ```
   ===========================================
   ESP32 High-Fidelity Audio Player
   ===========================================
   [WIFI] SSID: ESP32-Audio-Setup
   ```

5. Display shows "CROENIX" splash screen
6. Wi-Fi "ESP32-Audio-Setup" appears on phone

---

## 📞 Need Help?

See full guide: `ARDUINO_IDE_FLASH_GUIDE.md`

**Current Status**: 2026-11-10
