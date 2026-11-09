# Croenix ESP32 Music Player Firmware

Production firmware for the **Croenix Lossless Audio Music Player** paired with the **Croenix Android App**.

Target Controller: **ESP32-WROOM-32** (38-pin NodeMCU development board)

---

## 🚀 Key Capabilities

- **Simultaneous Wi-Fi AP + STA**: Direct hotspot (`Croenix-Audio` at `192.168.4.1`) always on for Android app connection without router dependence, plus independent home router connection with exponential backoff & jitter.
- **Always-On Bluetooth Classic A2DP Sink**: Phone pairs with `Croenix Audio` over Bluetooth to stream lossless music directly to the CJMCU-5102 I2S DAC.
- **Bluetooth Low Energy (BLE)**: Custom GATT service for device discovery, provisioning, pairing, and recovery without Wi-Fi.
- **Real-Time WebSocket (`/ws`)**: Bidirectional state synchronization, command execution, and live event broadcasts (`volume_changed`, `track_changed`, `playback_changed`, `wifi_changed`, etc.).
- **8 REST Endpoints**: 100% compatible with the Flutter Croenix app (`/api/device`, `/api/status`, `/api/network`, `/api/system`, `/api/pair`, `/api/commands`, `/api/network/config`, `/api/device/restart`).
- **Security & Authentication**: 32-character cryptographically secure token exchange, Bearer authorization, physical encoder confirmation, and brute-force rate-limiting lockout.
- **Power / Status LED**: Onboard GPIO 2 blinks a gentle heartbeat pulse every **5 seconds** during normal healthy operation, and flashes every **1 second** if abnormal or in an error state.
- **Hardware Integration (HAL)**: Centralized `PinConfig.h` with interrupt-driven KY-040 rotary encoder, 1.8" SPI TFT screen manager, MicroSD card interface, and I2S CJMCU-5102 DAC pipeline.

---

## 🛠️ Hardware Inventory & Wiring

| Subsystem | Hardware Component | ESP32 GPIO Pins | Notes |
|---|---|---|---|
| **Main MCU** | ESP32-WROOM-32 (38-pin) | - | Dual-core 240MHz, 4MB Flash |
| **Status LED** | Onboard LED | `GPIO 2` | 5s normal heartbeat / 1s error cadence |
| **Rotary Encoder** | KY-040 | `CLK: GPIO 32`<br>`DT: GPIO 33`<br>`SW: GPIO 25` | Hardware interrupt debouncing, long-press triggers pairing |
| **I2S DAC** | CJMCU-5102 (PCM5102A) | `BCLK: GPIO 26`<br>`LRCK: GPIO 27`<br>`DOUT: GPIO 22` | 16/24-bit stereo 44.1/48kHz PCM |
| **Amplifiers** | 3× Gold A542 (Class-D) | Audio in from DAC | 6 amplified channels |
| **TFT Display** | 1.8" SPI LCD (128×160) | `CS: 5`, `DC: 16`, `RST: 17`<br>`MOSI: 23`, `SCK: 18` | Non-blocking display refresh |
| **MicroSD Card** | SPI TF Card Socket | `CS: 4`, `MOSI: 23`<br>`MISO: 19`, `SCK: 18` | Auto card insertion detect |

*All pin assignments are defined in [`src/PinConfig.h`](src/PinConfig.h).*

---

## 💻 Build & Flash Guide

### 1. Requirements

- **Arduino IDE 2.x** or **arduino-cli**
- **ESP32 Arduino Core** v3.0+ (Tested on v3.3.12)
- Installed Libraries:
  - `ArduinoJson` (v7.x)
  - `ESP Async WebServer` (v3.12.1+ from ESP32Async)
  - `AsyncTCP` (v1.1.4+)

### 2. Arduino IDE Configuration

1. Connect the ESP32 board via USB.
2. Select **Tools → Board → ESP32 Arduino → ESP32 Dev Module**.
3. Select **Tools → Upload Speed → 921600** (or 115200 for reliability).
4. **CRITICAL**: Select **Tools → Partition Scheme → "Huge APP (3MB No OTA / 1MB SPIFFS)"**.
   *(Required because simultaneous Wi-Fi AP+STA and dual-mode Bluetooth require ~1.8 MB Flash space).*
5. Select the matching **Port** (e.g. `COM3` on Windows).
6. Click **Upload** (or press `Ctrl+U`).

### 3. Command Line Build (`arduino-cli`)

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app "c:\Users\achub\OneDrive\Documents\Music Player\croenix_firmware"
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32:PartitionScheme=huge_app "c:\Users\achub\OneDrive\Documents\Music Player\croenix_firmware"
```

---

## 📱 Connecting to the Android App

### Direct Hotspot Connection (No Router Needed)

1. Power on the ESP32 board. Observe the onboard LED: after initialization, it pulses every **5 seconds**.
2. On your Android phone, connect to Wi-Fi:
   - **SSID**: `Croenix-Audio`
   - **Password**: `Croenix@123`
3. Launch the **Croenix** app. The app automatically discovers the device at `192.168.4.1`.
4. Tap **Connect**. The app requests `/api/pair` and receives a secure authentication token.
5. The live Dashboard appears displaying real device status, free heap, uptime, and volume.

### Connecting to Home Wi-Fi Router

1. In the app or via HTTP POST `/api/network/config`, send your router credentials:
   ```json
   {
     "ssid": "MyHomeWiFi",
     "password": "MySecretPassword"
   }
   ```
2. The ESP32 connects to the router asynchronously in the background.
3. The direct hotspot (`Croenix-Audio`) **remains active**, so you never lose connection even if the router is offline.

---

## 🎵 Streaming Music via Bluetooth A2DP

1. Open Bluetooth settings on your smartphone.
2. Select **"Croenix Audio"** from the discovered devices list and pair.
3. Play music on any app (Spotify, Apple Music, YouTube, local player).
4. Audio streams continuously to the ESP32 and outputs via the CJMCU-5102 DAC.
5. The Android app dashboard automatically reflects Bluetooth as the active audio source.
