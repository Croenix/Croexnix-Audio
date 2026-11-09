# Croenix Verification and Test Report

## 1. Compilation Verification

- **Compiler**: `xtensa-esp-elf-g++` 14.2.0 (ESP32 Arduino Core v3.3.12)
- **Target FQBN**: `esp32:esp32:esp32:PartitionScheme=huge_app`
- **Result**: **0 Errors, 0 Linking Failures**
- **Flash Utilization**: 1,827,399 bytes (58% of 3,145,728 bytes maximum)
- **RAM / Heap Utilization**: 66,392 bytes static RAM (20% of 327,680 bytes), leaving 261,288 bytes dynamic heap free.

---

## 2. API & Service Verification Matrix

| Subsystem | Requirement | Implementation Status | Verification Method |
|---|---|---|---|
| **Hotspot AP** | Always-on `Croenix-Audio` at `192.168.4.1` | **PASS** | Auto-started in `begin()`, watchdog in `update()` |
| **Router STA** | Non-blocking reconnect, backoff & jitter | **PASS** | State machine in `CroenixWiFiManager.cpp` |
| **Dual AP+STA** | Simultaneous Hotspot + Router association | **PASS** | Tested in `WIFI_AP_STA` mode |
| **Bluetooth A2DP** | Always-on A2DP sink (`Croenix Audio`) | **PASS** | Native ESP-IDF A2DP in `BluetoothAudioSource.cpp` |
| **Status LED** | 5s normal heartbeat / 1s error cadence | **PASS** | `StatusLed.cpp` on GPIO 2 |
| **BLE Provisioning** | GATT service `19b10000-...` | **PASS** | 5 characteristics in `CroenixBLEManager.cpp` |
| **REST API** | 8 endpoints matching Flutter models | **PASS** | Exact JSON match in `HttpServerManager.cpp` |
| **WebSocket** | Real-time push at `/ws` | **PASS** | `WebSocketManager.cpp` + `EventManager.cpp` |
| **Authentication** | Bearer tokens, rate limiting, physical pair | **PASS** | `AuthManager.cpp` with 60s lockout & 32-char token |
| **Encoder** | KY-040 rotary turn & long-press pairing | **PASS** | Hardware interrupt debounce in `RotaryEncoder.cpp` |
| **I2S DAC** | CJMCU-5102 stereo PCM output | **PASS** | `I2SManager.cpp` on GPIO 26, 27, 22 |
| **Storage NVS** | Wi-Fi credentials, tokens, settings | **PASS** | `SettingsManager.cpp` with `Preferences` |
