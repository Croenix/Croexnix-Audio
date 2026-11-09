# Build and Flash Instructions for Croenix ESP32

## 1. Prerequisites

### Tools
- **Arduino IDE 2.x** OR **arduino-cli** (v1.5.1+)
- **ESP32 Arduino Board Support**: Version 3.0+ (Tested with 3.3.12)
- USB to Micro-B data cable

### Library Installation
In Arduino IDE (Library Manager) or via folder placement:
1. `ArduinoJson` (v7.x)
2. `ESP Async WebServer` (v3.12.1+ by ESP32Async)
3. `AsyncTCP` (v1.1.4+)

---

## 2. Arduino IDE 2.x Build & Flash

1. Open `croenix_firmware/croenix_firmware.ino` in Arduino IDE.
2. Connect your ESP32-WROOM-32 board via USB.
3. In **Tools → Board**, select **ESP32 Dev Module**.
4. In **Tools → Port**, select your board's serial port (e.g. `COM3` on Windows, `/dev/ttyUSB0` on Linux).
5. In **Tools → Upload Speed**, select `921600` (or `115200` if cable is long/unshielded).
6. **MANDATORY**: In **Tools → Partition Scheme**, select:
   ```
   Huge APP (3MB No OTA / 1MB SPIFFS)
   ```
7. Click the **Upload** button (`Ctrl + U`).
8. Open **Serial Monitor** at **115200 baud** to view startup diagnostics and connection details.

---

## 3. CLI Build & Flash (`arduino-cli`)

Compile:
```powershell
& "C:\Users\achub\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile `
  --fqbn esp32:esp32:esp32:PartitionScheme=huge_app `
  --libraries "C:\Users\achub\OneDrive\Documents\Arduino\libraries" `
  "c:\Users\achub\OneDrive\Documents\Music Player\croenix_firmware"
```

Flash:
```powershell
& "C:\Users\achub\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" upload `
  -p COM3 `
  --fqbn esp32:esp32:esp32:PartitionScheme=huge_app `
  "c:\Users\achub\OneDrive\Documents\Music Player\croenix_firmware"
```

---

## 4. Boot Verification

Upon flashing and restarting, the Serial Monitor outputs:
```
==================================================
          CROENIX FIRMWARE v1.0.0
          Target: ESP32-WROOM-32
   Dual AP+STA Wi-Fi / BLE / WebSocket / Audio   
==================================================
[Setup] Initializing Storage...
[Setup] Initializing PlayerStateManager...
[Setup] Initializing System Diagnostics...
[Setup] Initializing Authentication Engine...
[Setup] Initializing Wi-Fi Subsystem (AP + STA)...
[Setup] Initializing Discovery Manager...
[Setup] Starting HTTP REST API & WebSocket Engine...
[Setup] Starting BLE Setup & Recovery Engine...
[Setup] Initializing KY-040 Rotary Encoder...
[Setup] Initializing TFT Screen Manager...
[Setup] Initializing MicroSD Card Interface...
[Setup] Initializing I2S Audio Pipeline...
[Setup] Initializing Power & Status LED Engine...

--------------------------------------------------
 CROENIX IS READY
 Device ID:      croenix-xxxxxx
 Access Point:   Croenix-Audio (IP: 192.168.4.1)
 BLE Advertising: Croenix
 REST Endpoints: 8 active on port 80
 WebSocket:      ws://<ip>:80/ws
 Free Heap:      ~261 KB
--------------------------------------------------
```
The onboard blue LED (GPIO 2) begins its **5-second heartbeat pulse**, confirming healthy execution.
