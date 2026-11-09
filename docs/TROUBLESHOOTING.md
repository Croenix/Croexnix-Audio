# Croenix Troubleshooting Guide

## 1. Status LED Diagnostic Indicators

The onboard LED on GPIO 2 provides immediate visual feedback on board health:
- **Gentle pulse every 5 seconds**: System is operating normally. Wi-Fi Access Point is active, heap memory is healthy (>25 KB), and no system faults are detected.
- **Rapid flash every 1 second**: Abnormal condition or error state. Possible causes:
  - Wi-Fi AP failed to start (check RF interference or power rail).
  - Free heap dropped below threshold (<25 KB).
  - Hard reset or brownout reset occurred.

---

## 2. Common Build & Upload Issues

### Error: `Sketch uses ... bytes (139%). Sketch too big`
- **Cause**: Arduino IDE default partition scheme limits sketch size to 1.2 MB.
- **Solution**: In Arduino IDE, go to **Tools → Partition Scheme** and choose **"Huge APP (3MB No OTA / 1MB SPIFFS)"**.

### Error: `A fatal error occurred: Failed to connect to ESP32: Timed out waiting for packet header`
- **Cause**: ESP32 bootloader requires GPIO 0 to be pulled LOW during reset.
- **Solution**: Hold down the physical **BOOT** button on the ESP32 board while clicking Upload, and release it when "Connecting..." appears in the console.

---

## 3. Wi-Fi & App Connection Issues

### App Cannot Find `Croenix-Audio` Hotspot
1. Ensure the ESP32 is powered via a clean 5V power supply (min 500mA).
2. ESP32 Wi-Fi operates strictly on the **2.4 GHz band**. Verify that your smartphone's Wi-Fi search includes 2.4 GHz.
3. Check the Serial Monitor (115200 baud) for `[WiFiManager] AP Active: SSID='Croenix-Audio', IP='192.168.4.1'`.

### HTTP 403: "Pairing mode not active"
- **Cause**: The pairing window has closed to prevent unauthorized pairing.
- **Solution**: Long-press the KY-040 rotary encoder push-button for 2 seconds. The console will display `[AuthManager] Pairing mode ENABLED for 30000 ms`, allowing the phone to pair.
