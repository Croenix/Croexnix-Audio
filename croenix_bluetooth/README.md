# CROENIX Dual-Mode Bluetooth Firmware (ESP32-WROOM-32)

Ultra-low-latency, dual-mode Bluetooth audio receiver and mobile telemetry controller engineered specifically for the **Croenix Lossless Audio Player**.

---

## 1. Key Architectural Features

| Subsystem | Specification | Description |
| :--- | :--- | :--- |
| **BT Classic Audio** | A2DP Sink (SBC Profile) | Streams 44.1 kHz / 48 kHz stereo music from Android & iPhone |
| **Media Controls** | AVRCP Target & Controller | Volume sync, play/pause/track skip, ID3 metadata telemetry |
| **Mobile Companion** | BLE GATT Server | Instant auto-discovery & bidirectional JSON/ASCII remote control |
| **DAC Interface** | I²S Master (PCM5102A) | BCK: `GPIO 26`, DIN: `GPIO 25`, LCK: `GPIO 22` |
| **DMA Pipeline** | FreeRTOS Ringbuffer | 24 KB lock-free byte ring buffer with dedicated Core 1 audio task |
| **Coexistence** | BTDM Controller | Zero mutual disconnections between Classic audio & BLE telemetry |

---

## 2. Hardware Wiring (PCM5102A DAC to ESP32)

| PCM5102A Pin | ESP32-WROOM-32 Pin | Function / Description |
| :--- | :--- | :--- |
| **VCC** | `VIN` / `5V` | Module DC power input |
| **3.3V** | `NC` | Internal 3.3V LDO output (leave open) |
| **GND** | `GND` | Common ground reference |
| **FLT** | `GND` | Normal latency filter mode |
| **DMP** | `GND` | De-emphasis filter disabled |
| **SCL / SCK**| `GND` | Module internal PLL master clock generation |
| **BCK** | `GPIO 26` | I²S Continuous Bit Clock |
| **DIN** | `GPIO 25` | I²S Serial Audio Data |
| **LCK** | `GPIO 22` | I²S Word Select (Left/Right Clock) |
| **FMT** | `GND` | Standard I²S format select |
| **XMT** | `3.3V` | Soft-unmute (High = Output Enabled) |

> **IMPORTANT**: The red LED on the PCM5102A board is a power indicator. Audio requires `XMT` tied to `3.3V` to disable the hardware mute circuit, and `SCL` connected to `GND` so the internal PLL generates the master clock.

---

## 3. BLE GATT Service & Characteristics

- **Primary Service UUID**: `0x00FF`
- **Device Advertisement Name**: `CROENIX`

| Characteristic | UUID | Permissions | Description / Payload |
| :--- | :--- | :--- | :--- |
| **Command** | `0xFF01` | Write | Text commands: `PLAY`, `PAUSE`, `NEXT`, `PREV`, `VOL:xx`, `MUTE`, `STATUS` |
| **Status Telemetry** | `0xFF02` | Read, Notify | JSON: `{"state":"PLAYING","vol":75,"title":"...","artist":"..."}` |
| **Device Info** | `0xFF03` | Read | Device metadata: `CROENIX|3.1.0-BT-DUAL|PCM5102A` |
| **Connection State**| `0xFF04` | Read, Notify | Telemetry: `{"a2dp":true,"peer":"OnePlus 12","ble":true}` |

---

## 4. Building and Flashing

### Requirements
- **ESP-IDF v5.0+** (or v4.4+) installed and configured in your environment.

### Build Steps

```bash
# 1. Navigate to the project directory
cd croenix_bluetooth

# 2. Set the target chip
idf.py set-target esp32

# 3. Build the firmware
idf.py build

# 4. Flash and open serial monitor (replace COMx with your port)
idf.py -p COMx flash monitor
```

---

## 5. Dual-Mode Operation Workflow

1. **Power On**: ESP32 boots and initializes the PCM5102A I²S DAC, the BT controller in BTDM mode, and starts BLE advertising as `CROENIX`.
2. **Mobile App Control (BLE)**: Open the Croenix mobile app. The app discovers the BLE advertisement immediately without requiring Wi-Fi hotspot mode or disconnecting mobile data.
3. **Audio Streaming (Classic A2DP)**: Go to your phone's native Bluetooth settings, select **CROENIX**, and pair. Play music from Spotify, Apple Music, YouTube, etc.
4. **Simultaneous Flow**: You can control playback, adjust hardware volume, and read track metadata through the mobile application over BLE while audio streams flawlessly through Classic A2DP to your Hi-Fi amplifier via the PCM5102A DAC.

