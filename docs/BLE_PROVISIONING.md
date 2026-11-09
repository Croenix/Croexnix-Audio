# Croenix BLE Setup & Provisioning Protocol

## 1. Bluetooth Low Energy Architecture

Croenix provides an out-of-band setup and recovery channel using Bluetooth Low Energy (BLE). This allows the Flutter app to configure Wi-Fi credentials and retrieve diagnostic data even when Wi-Fi is disabled or unconfigured.

- **Advertised Name**: `Croenix`
- **Primary Service UUID**: `19b10000-e8f2-537e-4f6c-d104768a1214`

---

## 2. GATT Characteristics

| Characteristic | UUID | Properties | Purpose | Payload Format |
|---|---|---|---|---|
| **Device Info** | `19b10001-...` | `READ` | Device name, firmware version, hardware ID | JSON string |
| **Provisioning** | `19b10002-...` | `WRITE` | Submit Wi-Fi router credentials | `{"ssid":"...","password":"..."}` |
| **Network Status** | `19b10003-...` | `READ`, `NOTIFY` | Current IP, SSID, RSSI, and client counts | JSON string |
| **Pairing / Token** | `19b10004-...` | `READ`, `WRITE` | Request pairing token or enable pairing | `{"token":"..."}` |
| **Device Events** | `19b10005-...` | `NOTIFY` | Stream live state changes (volume, playback) | JSON string |

---

## 3. Wi-Fi Provisioning Flow via BLE

1. The Android app scans for BLE peripherals advertising Service UUID `19b10000-...`.
2. Connects to `Croenix` and reads the Device Info characteristic (`19b10001-...`).
3. Writes the target Wi-Fi credentials JSON to Provisioning (`19b10002-...`):
   ```json
   {
     "ssid": "MyHomeNetwork",
     "password": "RouterPassword123"
   }
   ```
4. The ESP32 triggers `connectToRouterAsync()`.
5. When the ESP32 acquires an IP address from the router, it pushes a notification on Network Status (`19b10003-...`) containing the router IP address.
6. The app automatically transitions to Wi-Fi communication at the assigned IP.

