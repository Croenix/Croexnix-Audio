# Quick Fix Guide - Croenix Audio

## 🚨 CRITICAL FIX: GPIO 22 Conflict

### Problem
GPIO 22 is used for BOTH I2S audio data AND FM radio I²C clock.

### Solution (5 minutes)
1. **Update config file**:
   - File already updated: `esp32_firmware/src/core/config.h`
   - Line 11 changed: `I2S_DOUT` from 22 → 32

2. **Rewire hardware**:
   - Move I2S DAC DATA wire from GPIO 22 to GPIO 32
   - All other connections stay the same

### Verification
After fix:
- ✅ I2S audio uses GPIO 32
- ✅ FM radio uses GPIO 22 (I²C SCL)
- ✅ No conflicts

---

## 📋 Pre-Flight Checklist

### Before Uploading Firmware
- [ ] Fix applied: I2S DATA moved to GPIO 32
- [ ] All wires connected per BUILD_GUIDE.md
- [ ] Power supply connected (5V for ESP32, 12V for amp)
- [ ] SD card inserted (FAT32 formatted, optional)
- [ ] USB cable connected for upload

### Upload Firmware
```bash
cd esp32_firmware
pio run --target upload
pio device monitor
```

**Expected output**:
```
===========================================
ESP32 High-Fidelity Audio Player
===========================================
[INIT] Creating FreeRTOS tasks...
[WIFI] Starting in Access Point mode...
[WIFI] SSID: ESP32-Audio-Setup
```

### Install Flutter App
```bash
cd croenix_app
flutter pub get
flutter run
```

### Test Connection
1. Phone sees "ESP32-Audio-Setup" Wi-Fi (or discovers via mDNS)
2. App connects to ESP32
3. Configure home Wi-Fi in app
4. Test audio playback

---

## 🔧 If Something Doesn't Work

### No Serial Output
- Check USB cable (must support data)
- Try different COM port
- Hold BOOT button while uploading

### Display Blank
- Check 3.3V power to display
- Verify SPI pins (MOSI=23, SCK=18, CS=5, DC=2, RST=4)
- Check TFT backlight LED connection

### No Audio
- Verify I2S wiring (DATA=32, BCK=26, LRC=25)
- Check DAC 3.3V power
- Test with headphones directly on DAC output
- Verify amplifier connections

### App Can't Find Device
- ESP32 must be powered on
- Check Wi-Fi: either connect to "ESP32-Audio-Setup" or same home network
- Try manual IP entry in app
- Check firewall settings

### FM Radio Not Working
- After fixing GPIO 22 conflict, it should work
- Connect proper FM antenna (wire antenna minimum)
- Check I²C pins (SDA=21, SCL=22)

---

## ⚡ Quick Test Commands

### Test ESP32 is alive
```bash
pio device monitor
# Should see boot messages
```

### Test Flutter app compiles
```bash
cd croenix_app
flutter doctor
flutter pub get
flutter analyze
```

### Test hardware individually
1. **Display**: Should show splash screen on boot
2. **SD Card**: Check serial monitor for mount message
3. **FM Radio**: Serial monitor shows init message
4. **Audio**: Play test tone (requires code modification)

---

## 📞 Getting Help

1. **Check serial monitor** first (most errors show there)
2. **Review VERIFICATION_REPORT.md** for detailed info
3. **See BUILD_GUIDE.md** for complete troubleshooting
4. **Check wiring** against pin tables

---

## ✅ Success Indicators

You know it's working when:
- ✅ Serial monitor shows all subsystems initialized
- ✅ Display shows main screen with info
- ✅ Phone discovers ESP32 device
- ✅ App connects and shows "Connected"
- ✅ Audio plays from any source
- ✅ All controls respond in app

**First test**: Play audio via Bluetooth (easiest to verify)

---

**Last Updated**: 2026-11-10
