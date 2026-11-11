# Hardware Setup & Troubleshooting Guide
## Croenix Audio Player - Complete Wiring & Configuration

**Last Updated:** 2026-11-11

---

## 📦 Required Components

| Component | Product Link | Quantity | Notes |
|-----------|--------------|----------|-------|
| ESP32-WROOM-32 (38-pin) | - | 1 | Development board with dual-core |
| CJMCU-5102 I²S DAC | [Robu.in Link](https://robu.in/product/cjmcu-5102-i2s-player-module/) | 1 | PCM5102A chip with onboard regulators |
| MicroSD Card Module | [Robu.in Link](https://robu.in/product/micro-sd-card-module/) | 1 | With level shifters and 3.3V LDO |
| ST7735 1.8" TFT Display | - | 1 | 128x160 SPI LCD |
| Si4703/TEA5767 FM Module | - | 1 | I²C FM radio receiver |
| Rotary Encoder (KY-040) | - | 1 | 5-pin with push button |

---

## 🔌 Complete Pin Configuration

### 1. CJMCU-5102 I²S DAC Module
**Module Documentation:** https://robu.in/product/cjmcu-5102-i2s-player-module/

#### Pin Connections:
```
ESP32 Pin    →    CJMCU-5102 Pin    Function
─────────────────────────────────────────────
5V           →    VIN               Power (uses onboard LDO)
GND          →    GND               Ground
GPIO 26      →    BCK               I²S Bit Clock
GPIO 25      →    LCK               I²S Left/Right Clock (Word Select)
GPIO 27      →    DIN               I²S Serial Data Input
```

#### ⚠️ CRITICAL: Onboard Jumper Configuration

The CJMCU-5102 module has **4 solder jumpers** that MUST be configured correctly. Most "no audio" issues are caused by incorrect jumper settings!

**H1L (System Clock Source):**
- **MUST solder to L position** (connects SCK pin to GND)
- This enables the internal PLL to generate master clock (MCLK) from BCLK
- ❌ If open or in H position → **NO AUDIO OUTPUT!**

**H2L (Bit Clock Source):**
- **Leave OPEN** (not soldered)
- BCK will be driven by ESP32 GPIO 26

**H3 (Soft Mute Control):**
- **MUST solder to H position** (connects XSMT to VCC/3.3V)
- This permanently unmutes the DAC (XSMT is active-LOW)
- ❌ If open or in L position → **AUDIO WILL BE MUTED!**

**H4 (Filter Select):**
- **Solder to L position** (connects FLT to GND)
- Selects normal latency filter
- This is usually the default setting

#### Visual Jumper Configuration:
```
┌─────────────────────────────────┐
│  CJMCU-5102 Module (Top View)  │
│                                 │
│  H1L:  [L]●─●[ ]  ← MUST be L  │
│  H2L:  [ ]   [ ]  ← OPEN       │
│  H3:   [ ]─●[H]●  ← MUST be H  │
│  H4:   [L]●─●[ ]  ← Should be L│
└─────────────────────────────────┘
```

#### Audio Output Connections:
```
CJMCU-5102    →    Class-D Amplifier
────────────────────────────────────
LOUT (Left)   →    Amplifier Left Input
ROUT (Right)  →    Amplifier Right Input
GND           →    Amplifier Ground
```

---

### 2. MicroSD Card Module
**Module Documentation:** https://robu.in/product/micro-sd-card-module/

#### Pin Connections:
```
ESP32 Pin    →    SD Module Pin    Function
─────────────────────────────────────────────
5V           →    VCC              Power (IMPORTANT: Use 5V!)
GND          →    GND              Ground
GPIO 23      →    MOSI (DI)        SPI Data In (shared with TFT)
GPIO 19      →    MISO (DO)        SPI Data Out
GPIO 18      →    SCK (CLK)        SPI Clock (shared with TFT)
GPIO 15      →    CS               Chip Select
```

#### ⚠️ IMPORTANT Notes for This Module:

1. **Power Supply:** 
   - **ALWAYS connect VCC to 5V, NOT 3.3V!**
   - The module has an onboard 3.3V LDO regulator (AMS1117-3.3)
   - The regulator provides clean 3.3V to the SD card

2. **Level Shifters:**
   - Module includes 74HC125 or 74LVC125 level shifters
   - Safe to connect 3.3V ESP32 GPIO pins directly to module

3. **Pull-up Resistors:**
   - Module has 10kΩ pull-ups on all data lines
   - ESP32 internal pull-ups also enabled for MISO

4. **SD Card Compatibility:**
   - ✅ Recommended: SanDisk, Samsung cards ≤ 32GB
   - ✅ Format: FAT32 (NOT exFAT or NTFS)
   - ❌ Avoid: No-name cards, SDHC/SDXC cards > 32GB

---

### 3. ST7735 1.8" TFT Display

#### Pin Connections:
```
ESP32 Pin    →    TFT Pin          Function
─────────────────────────────────────────────
3.3V         →    VCC              Power
GND          →    GND              Ground
GPIO 23      →    SDA (MOSI)       SPI Data (shared with SD)
GPIO 18      →    SCL (SCK)        SPI Clock (shared with SD)
GPIO 5       →    CS               Chip Select
GPIO 2       →    DC (A0/RS)       Data/Command
GPIO 4       →    RST              Hardware Reset
3.3V         →    BL (if separate) Backlight (or leave open)
```

---

### 4. FM Radio Module (Si4703 / TEA5767)

#### Pin Connections:
```
ESP32 Pin    →    FM Module Pin    Function
─────────────────────────────────────────────
3.3V         →    VCC              Power
GND          →    GND              Ground
GPIO 21      →    SDA              I²C Data
GPIO 22      →    SCL              I²C Clock
GPIO 16      →    RST              Hardware Reset (Si4703 only)
```

---

### 5. Rotary Encoder (KY-040)

#### Pin Connections:
```
ESP32 Pin    →    Encoder Pin      Function
─────────────────────────────────────────────
3.3V         →    +               Power
GND          →    GND              Ground
GPIO 32      →    CLK              Quadrature Channel A
GPIO 33      →    DT               Quadrature Channel B
GPIO 17      →    SW               Push Button (Active LOW)
```

---

## 🔧 Troubleshooting Guide

### Problem 1: No Audio Output from DAC

#### Symptom:
- ESP32 boots normally
- Display shows playback status
- But no sound from speakers/headphones

#### Solutions (in order):

1. **Check CJMCU-5102 Jumpers** (Most Common Issue!)
   ```
   ✓ Verify H1L is soldered to L position (SCK to GND)
   ✓ Verify H3 is soldered to H position (XSMT to VCC)
   ```

2. **Verify Pin Connections:**
   ```
   ✓ BCK → GPIO 26
   ✓ LCK → GPIO 25  
   ✓ DIN → GPIO 27
   ✓ VIN → 5V (or 3.3V if module has no LDO)
   ✓ GND → GND
   ```

3. **Check Audio Output:**
   - Use multimeter in AC voltage mode on LOUT/ROUT while playing
   - Should see ~1-2V AC when audio is playing
   - If 0V → jumper issue or bad module

4. **Test with Headphones:**
   - Connect headphones directly to LOUT/GND (left) or ROUT/GND (right)
   - Volume will be low but should be audible
   - If no sound → module defective

5. **Serial Monitor Debug:**
   ```
   Look for: "[AUDIO] I2S initialized successfully"
   Should show: "BCLK=26, LRC=25, DOUT=27"
   ```

---

### Problem 2: SD Card Not Detected

#### Symptom:
- Serial monitor shows "MOUNT FAILED: No card detected"
- Display shows "No SD Card"

#### Solutions (in order):

1. **Verify Power Connection:**
   ```
   ⚠️ CRITICAL: Module VCC MUST be connected to 5V!
   - The module has a 3.3V LDO regulator
   - Connecting to 3.3V will cause brownout
   ```

2. **Check Pin Connections:**
   ```
   ✓ MOSI → GPIO 23
   ✓ MISO → GPIO 19 (most important - check solder joints!)
   ✓ SCK  → GPIO 18
   ✓ CS   → GPIO 15
   ✓ GND  → GND
   ```

3. **Verify SD Card:**
   - Remove and reinsert SD card firmly
   - Try a different SD card (SanDisk/Samsung recommended)
   - Format card as FAT32 on computer first
   - Use cards ≤ 32GB for best compatibility

4. **Test with Different SPI Speed:**
   - The firmware automatically tries 400kHz, 1MHz, 2MHz, 4MHz
   - Watch serial monitor for which speed succeeds
   - If all fail → wiring issue

5. **Check for Hardware Faults:**
   ```
   - Inspect module for bent pins
   - Check solder joints on ESP32 header pins
   - Try different GPIO pins (update config.h)
   - Test module with another microcontroller
   ```

---

### Problem 3: SD Card Detected but Mount Failed

#### Symptom:
- Serial shows "HARDWARE DETECTED! ...but FAT32 mount failed"

#### Solutions:

1. **Reformat SD Card:**
   ```
   Windows:
   - Right-click SD card in Explorer
   - Format → File system: FAT32
   - Allocation unit: 4096 bytes (default)
   
   Mac/Linux:
   - Use Disk Utility or fdisk
   - Format as MS-DOS (FAT) / FAT32
   ```

2. **Check Card Size:**
   - ESP32 SD library works best with ≤ 32GB cards
   - Larger cards often use exFAT (not supported)

3. **Test Different Card:**
   - Some cards have compatibility issues
   - Recommended brands: SanDisk, Samsung, Kingston

---

### Problem 4: Audio Stuttering / Crackling

#### Symptom:
- Audio plays but with dropouts, stuttering, or crackling

#### Solutions:

1. **Reduce SPI Activity:**
   - The firmware uses mutex locking to prevent SPI bus conflicts
   - Try lowering display update rate in config.h

2. **Check SD Card Quality:**
   - Slow/fake SD cards cause read delays
   - Use Class 10 or UHS-I cards
   - Try lower bitrate MP3 files (128-192 kbps)

3. **Verify Power Supply:**
   - Ensure stable 5V supply with sufficient current (≥500mA)
   - Use quality USB cable or dedicated power adapter
   - Add 100µF capacitor near ESP32 VIN

4. **Optimize Firmware:**
   - Already optimized with larger I2S buffers (2048 bytes)
   - More DMA descriptors (12) for smoother playback
   - Increased audio loop iterations

---

### Problem 5: Display Not Working

#### Symptom:
- Display stays white/black or shows garbage

#### Solutions:

1. **Check Pin Connections:**
   ```
   ✓ MOSI → GPIO 23 (shared with SD)
   ✓ SCK  → GPIO 18 (shared with SD)
   ✓ CS   → GPIO 5  (dedicated)
   ✓ DC   → GPIO 2
   ✓ RST  → GPIO 4
   ```

2. **Verify Display Type:**
   - Ensure it's ST7735 (not ST7789 or ILI9341)
   - Check display initialization flags in code

3. **Test with Example Sketch:**
   - Load Adafruit_ST7735 library example
   - Verify display works independently

---

## 📊 System Resource Usage

```
Memory Map:
├─ Audio Buffers:     ~24KB (I2S DMA)
├─ SD Track Index:    ~14KB (128 tracks × 112 bytes)
├─ Display Buffer:    ~40KB (160×128×16bit)
├─ Network Stack:     ~60KB (Wi-Fi + WebSocket)
└─ FreeRTOS Tasks:    ~20KB (4 tasks)
    Total:            ~158KB / 520KB RAM available
```

---

## 🔍 Serial Debug Output Reference

### Successful Boot Sequence:
```
[AUDIO] Initializing I2S audio subsystem (CJMCU-5102 / PCM5102A)...
[AUDIO] I2S initialized successfully
[AUDIO] BCLK=26, LRC=25, DOUT=27
[AUDIO] Buffer: 2048 bytes, Volume=15/21
[SD] Initializing SPI bus and mounting MicroSD adapter...
[SD] Attempting SD.begin at 400 kHz...
[SD] Success! Card Type: SDHC, Size: 7580 MB, Used: 3241 MB
[SD] Scanning SD card for audio tracks...
[SD] Found 47 tracks on SD card
[DISPLAY] ST7735 TFT initialized successfully
[WiFi] Starting SoftAP: Croenix-Setup
```

### Common Error Messages:

**SD Card Errors:**
```
"[SD] ❌ MOUNT FAILED: No card detected"
→ Check wiring, especially VCC to 5V and MISO to GPIO19

"[SD] ❌ HARDWARE DETECTED! ...but FAT32 mount failed"
→ Reformat card as FAT32

"[SD] Found 0 tracks on SD card"
→ Add MP3/WAV/FLAC files to card root or /MUSIC folder
```

**Audio Errors:**
```
"[AUDIO] Failed to load MP3"
→ File corrupted or unsupported codec

"[AUDIO] ⚠️ VERIFY DAC JUMPERS"
→ Check H1L and H3 jumper positions on CJMCU-5102
```

---

## 📈 Performance Optimization Tips

1. **Audio Quality:**
   - Use 128-320 kbps MP3 files for best balance
   - FLAC/WAV work but use more CPU
   - Keep sample rate at 44.1kHz or 48kHz

2. **SD Card Performance:**
   - Use Class 10 or UHS-I cards
   - Avoid fragmented filesystems (reformat periodically)
   - Keep track count under 128 for fast indexing

3. **Power Supply:**
   - Use quality 5V/1A+ power supply
   - Short, thick USB cables (<1m)
   - Add decoupling capacitors (100µF + 0.1µF) near ESP32

4. **Wiring:**
   - Keep I²S data wires short (<15cm)
   - Twist BCK/LCK/DIN together to reduce EMI
   - Keep away from high-current/noisy wires

---

## 🧪 Hardware Testing Procedure

### 1. Power-On Test:
```
□ ESP32 LED lights up
□ TFT display backlight turns on
□ Serial monitor shows boot messages
```

### 2. Display Test:
```
□ Boot splash appears
□ UI elements render correctly
□ No flickering or artifacts
```

### 3. SD Card Test:
```
□ Insert formatted FAT32 card with MP3 files
□ Wait 5 seconds for initialization
□ Serial shows "SD Success! Card Type: SDHC..."
□ Serial shows "Found X tracks on SD card"
```

### 4. Audio Test:
```
□ Rotate encoder to select SD source
□ Press encoder to play
□ Audio should start within 2 seconds
□ Check for clear sound, no distortion
□ Adjust volume with encoder
```

### 5. Wi-Fi Test:
```
□ Long-press encoder for 10 seconds
□ Display shows QR code for provisioning
□ Connect phone to "Croenix-Setup" AP
□ Browse to 192.168.4.1
```

---

## 📞 Support & Additional Resources

**Firmware Issues:** Check `esp32_firmware/Croenix_Audio_Firmware/` code  
**Module Datasheets:**
- CJMCU-5102: https://robu.in/product/cjmcu-5102-i2s-player-module/
- SD Module: https://robu.in/product/micro-sd-card-module/
- PCM5102A: Search "TI PCM5102A datasheet"

**Common Issues Repository:** See GitHub issues or README.md

---

**Document Version:** 2.0  
**Last Tested:** 2026-11-11  
**Compatible Firmware:** v2.0.0-PROD and above
