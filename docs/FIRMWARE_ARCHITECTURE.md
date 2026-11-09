# Croenix ESP32 Firmware Architecture

## 1. System Overview

The Croenix firmware is organized into clean, decoupled layers adhering to embedded systems best practices:

```
croenix_firmware/
├── croenix_firmware.ino         # Main setup(), loop(), and initialization sequence
└── src/
    ├── Config.h                 # Master firmware settings, timings, UUIDs, network parameters
    ├── PinConfig.h              # Centralized GPIO pin mappings & active levels
    ├── Version.h                # Semantic firmware version & build metadata
    │
    ├── core/                    # Central state and command execution
    │   ├── PlayerState.h        # Authoritative state structures matching Flutter models
    │   ├── PlayerStateManager   # Thread-safe (mutex-protected) central state manager
    │   ├── EventManager         # Publish/Subscribe real-time event broker
    │   └── CommandManager       # Capability-aware unified command dispatcher
    │
    ├── connectivity/            # Networking, discovery, and communication
    │   ├── CroenixWiFiManager   # Non-blocking AP+STA dual Wi-Fi engine with auto-recovery
    │   ├── CroenixBLEManager    # BLE GATT server for provisioning, status, and recovery
    │   ├── DiscoveryManager     # mDNS responder (croenix.local) & UDP broadcast (5353)
    │   ├── HttpServerManager    # Asynchronous REST API exposing the 8 endpoints
    │   ├── WebSocketManager     # WebSocket (/ws) channel for real-time state push
    │   └── AuthManager          # Bearer tokens, rate-limiting, and pairing lifecycle
    │
    ├── audio/                   # Lossless audio playback pipeline
    │   ├── AudioSource.h        # Pure virtual audio source interface
    │   ├── AudioManager         # Central audio controller and source selector
    │   ├── I2SManager           # Low-level I2S driver for CJMCU-5102 DAC
    │   ├── BluetoothAudioSource # Always-on Bluetooth Classic A2DP audio sink
    │   ├── SdAudioSource        # MicroSD lossless playback architecture
    │   ├── FmAudioSource        # Hardware FM tuner abstraction
    │   └── WebRadioSource       # Internet stream & web radio client
    │
    ├── hardware/                # Physical controls & peripherals
    │   ├── StatusLed            # 5s normal heartbeat / 1s error cadence LED engine
    │   ├── RotaryEncoder        # KY-040 debounced encoder & push-button handler
    │   ├── DisplayManager       # 1.8" SPI TFT screen manager & status renderers
    │   └── SDManager            # MicroSD card detection and capacity reporting
    │
    ├── diagnostics/             # System health & fault management
    │   ├── SystemMonitor        # Live heap tracking, uptime, and reset reasons
    │   └── ErrorManager         # Rolling error recorder & fault notifications
    │
    └── storage/                 # Non-volatile persistence
        └── SettingsManager      # NVS Preferences storage for Wi-Fi, tokens, and audio state
```

---

## 2. Core Principles

1. **Non-Blocking Operation**: No synchronous busy-waits (`while` loops) or long `delay()` calls in any network or web callbacks. Wi-Fi reconnection uses asynchronous state transitions with exponential backoff and jitter.
2. **Single Authoritative State**: `PlayerStateManager` holds the single source of truth for volume, mute, playback status, network status, and hardware states. Physical controls and remote commands update the same state.
3. **Event-Driven Architecture**: `EventManager` distributes state changes to WebSocket clients, BLE subscribers, and physical displays simultaneously.
4. **Resilient Coexistence**: The local Wi-Fi Access Point remains active independently of router connection status. Bluetooth A2DP audio operates continuously alongside Wi-Fi and BLE.

