# Croenix Lossless Audio Pipeline

## 1. Architecture

```
                  ┌───────────────────────────────────────────┐
                  │          Audio Sources                    │
                  │  ┌─────────────┐  ┌────────────────────┐  │
                  │  │ Bluetooth   │  │ MicroSD Card       │  │
                  │  │ A2DP Sink   │  │ WAV / FLAC Decoder │  │
                  │  └──────┬──────┘  └─────────┬──────────┘  │
                  └─────────┼───────────────────┼─────────────┘
                            │                   │
                            ▼                   ▼
                  ┌───────────────────────────────────────────┐
                  │         AudioManager (Core Engine)        │
                  │  - Source arbitration & active selection  │
                  │  - Digital volume attenuation (0-100)     │
                  │  - Software mute & smooth ramp            │
                  └─────────────────────┬─────────────────────┘
                                        │
                                        ▼ 16-bit 44.1kHz Stereo PCM
                  ┌───────────────────────────────────────────┐
                  │                 I2SManager                │
                  │  BCLK: GPIO 26, LRCK: GPIO 27, DOUT: 22   │
                  └─────────────────────┬─────────────────────┘
                                        │
                                        ▼
                  ┌───────────────────────────────────────────┐
                  │          CJMCU-5102 I2S DAC               │
                  │          (Burr-Brown PCM5102A)            │
                  └─────────────────────┬─────────────────────┘
                                        │ Analog Stereo L/R
                                        ▼
                  ┌───────────────────────────────────────────┐
                  │      3× Gold A542 Stereo Amplifiers       │
                  │         (6 High-Power Channels)           │
                  └───────────────────────────────────────────┘
```

---

## 2. Always-On Bluetooth Classic A2DP Sink

The Bluetooth Classic A2DP receiver runs continuously in the background using the native ESP32 Bluedroid engine:
- **Profile**: A2DP Audio Sink (`esp_a2dp_api.h`).
- **Connection Mode**: Connectable and discoverable as `Croenix Audio`.
- **Decoding**: Hardware-accelerated SBC / AAC decoding to raw 44.1kHz 16-bit stereo PCM.
- **Data Forwarding**: Data callback forwards audio buffers directly to `I2SManager` for low-latency playback.
- **State Integration**: Player state updates automatically to `source: "bluetooth"` and `status: "playing"` upon receiving audio frames.

