# Flashing Croenix Audio Firmware to ESP32 Using Arduino IDE

## Prerequisites

1. **Download Arduino IDE**
   - Get it from: https://www.arduino.cc/en/software
   - Version 2.x recommended

2. **Install ESP32 Board Support**
   - Open Arduino IDE
   - Go to `File` → `Preferences`
   - Add this URL to "Additional Boards Manager URLs":
     ```
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     ```
   - Click OK
   - Go to `Tools` → `Board` → `Boards Manager`
   - Search for "esp32"
   - Install **"esp32 by Espressif Systems"**
   - Wait for installation to complete

3. **Install Required Libraries**
   - Go to `Tools` → `Manage Libraries` (or `Sketch` → `Include Library` → `Manage Libraries`)
   - Install these libraries one by one:

   ```
   Search and Install:
   ✅ ESP32-audioI2S (by schreibfaul1)
   ✅ TFT_eSPI (by Bodmer)
   ✅ ArduinoJson (by Benoit Blanchon) - version 6.x
   ✅ WebSockets (by Markus Sattler)
   ✅ SparkFun Si4703 Arduino Library
   ```

---

## Step 1: Convert PlatformIO Project to Arduino

Since your firmware is in PlatformIO format, we need to reorganize it for Arduino IDE.

### Create Arduino Sketch Folder

1. Create a new folder: `Croenix_Audio_Firmware`
2. Inside it, create a file: `Croenix_Audio_Firmware.ino`

### Copy Files

Copy all files from `esp32_firmware/src/` into the Arduino sketch folder:

```
Croenix_Audio_Firmware/
├── Croenix_Audio_Firmware.ino   (rename main.cpp to this)
├── config.h
├── device_state.h
├── device_state.cpp
├── audio_manager.h
├── audio_manager.cpp
├── wifi_manager.h
├── wifi_manager.cpp
├── websocket_server.h
├── websocket_server.cpp
├── display_manager.h
├── display_manager.cpp
├── fm_radio.h
├── fm_radio.cpp
├── sd_manager.h
└── sd_manager.cpp
```

### Update File Includes

In `Croenix_Audio_Firmware.ino`, change all includes from:
```cpp
#include "core/config.h"
#include "audio/audio_manager.h"
```

To:
```cpp
#include "config.h"
#include "audio_manager.h"
```

---

## Step 2: Configure TFT_eSPI Library

**IMPORTANT**: TFT_eSPI needs pin configuration

1. Find TFT_eSPI library folder:
   - Windows: `C:\Users\YOUR_NAME\Documents\Arduino\libraries\TFT_eSPI`
   - Mac: `~/Documents/Arduino/libraries/TFT_eSPI`

2. Open `User_Setup.h` in that folder

3. Replace ALL contents with:

```cpp
#define ST7735_DRIVER      

#define TFT_WIDTH  128
#define TFT_HEIGHT 160

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000

#define TFT_INVERSION_OFF
#define TFT_RGB_ORDER TFT_BGR

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define SMOOTH_FONT
```

4. Save and close

---

## Step 3: Configure Arduino IDE

1. **Select Board**
   - `Tools` → `Board` → `ESP32 Arduino` → **"ESP32 Dev Module"**

2. **Configure Settings**
   ```
   Board: "ESP32 Dev Module"
   Upload Speed: "921600"
   CPU Frequency: "240MHz (WiFi/BT)"
   Flash Frequency: "80MHz"
   Flash Mode: "QIO"
   Flash Size: "4MB (32Mb)"
   Partition Scheme: "Default 4MB with spiffs"
   Core Debug Level: "None"
   PSRAM: "Disabled"
   ```

3. **Select Port**
   - Connect ESP32 via USB
   - `Tools` → `Port` → Select your ESP32's COM port
   - Windows: Usually `COM3` or higher
   - Mac: `/dev/cu.usbserial-XXXX`

---

## Step 4: Upload Firmware

### Method 1: Normal Upload

1. Click **Verify** (✓ button) to compile
2. Wait for compilation (may take 2-3 minutes first time)
3. If successful, click **Upload** (→ button)
4. Wait for upload to complete

### Method 2: If Upload Fails

If you see "Failed to connect" or timeout errors:

1. **Hold BOOT button** on ESP32
2. Click **Upload** in Arduino IDE
3. When you see "Connecting....." in the console
4. **Release BOOT button**
5. Upload should proceed

---

## Step 5: Monitor Serial Output

1. After upload completes, open Serial Monitor:
   - `Tools` → `Serial Monitor`
   - Or press `Ctrl+Shift+M`

2. Set baud rate to **115200**

3. Press **RESET button** on ESP32

4. You should see:
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
   [WIFI] SSID: ESP32-Audio-Setup
   [WIFI] IP Address: 192.168.4.1
   ```

---

## Common Issues & Solutions

### Issue 1: "Compilation Error - File Not Found"

**Problem**: Missing library or wrong include path

**Solution**:
- Verify all libraries installed
- Check file includes don't have folder paths
- Make sure all `.h` and `.cpp` files are in same folder as `.ino`

### Issue 2: "Port Not Found" or "Device Not Recognized"

**Problem**: USB driver not installed

**Solution**:
- Install CP210x or CH340 USB driver
- CP210x: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
- CH340: Search "CH340 driver download"
- Restart computer after installation

### Issue 3: "Failed to Connect to ESP32"

**Problem**: ESP32 not in bootloader mode

**Solution**:
1. Hold **BOOT** button
2. Press **RESET** button briefly
3. Release **RESET** (keep holding BOOT)
4. Click Upload
5. When you see "Connecting..." release BOOT

### Issue 4: "Brownout Detector Triggered"

**Problem**: Insufficient power

**Solution**:
- Use USB cable that supports data AND power
- Try different USB port (USB 3.0 ports provide more power)
- Use external 5V power supply

### Issue 5: "Sketch Too Big"

**Problem**: Partition scheme too small

**Solution**:
- Change partition scheme to **"Huge APP (3MB No OTA)"**
- Or **"No OTA (2MB APP/2MB SPIFFS)"**

### Issue 6: TFT_eSPI Compilation Errors

**Problem**: Display library not configured

**Solution**:
- Make sure you edited `User_Setup.h` in TFT_eSPI library folder
- Delete any `User_Setup_Select.h` if it exists
- Restart Arduino IDE

---

## Verification Checklist

After successful upload:

- [ ] Serial monitor shows boot messages
- [ ] No error messages in serial output
- [ ] Display shows "CROENIX" splash screen
- [ ] Wi-Fi AP "ESP32-Audio-Setup" appears on phone
- [ ] ESP32 LED blinks (if it has one)

---

## Quick Reference: Arduino IDE Upload Process

```
1. Open Croenix_Audio_Firmware.ino
2. Select Board: ESP32 Dev Module
3. Select Port: COM3 (or your port)
4. Click Verify (✓) - wait for compilation
5. Click Upload (→) - hold BOOT if needed
6. Open Serial Monitor (115200 baud)
7. Press RESET button on ESP32
8. Verify boot messages
```

---

## Alternative: Use Pre-compiled Binary

If Arduino IDE continues to have issues, I can provide instructions to:
1. Compile in PlatformIO
2. Export binary file
3. Flash using ESP32 Flash Tool

Let me know if you need this option!

---

## Next Steps After Successful Flash

1. ✅ ESP32 is now running Croenix Audio firmware
2. ✅ Display should show main screen
3. ✅ Ready to install Flutter app on phone
4. ✅ Connect phone to "ESP32-Audio-Setup" to configure Wi-Fi

**Need help with Flutter app installation? Just ask!**
