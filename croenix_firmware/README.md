# Croenix ESP32 Firmware - Phase 1 & 2

This firmware implements the network foundation for the Croenix music player, providing WiFi connectivity (AP+STA mode), HTTP REST API, authentication, and device discovery for the Android application.

## Hardware Requirements

- **ESP32-WROOM-32** (38-pin NodeMCU development board)
- USB cable for programming
- Computer with Arduino IDE installed

## Required Arduino Libraries

Install these libraries via **Arduino Library Manager** (Sketch → Include Library → Manage Libraries):

1. **ArduinoJson** by Benoit Blanchon (version 7.x)
2. **ESPAsyncWebServer** by me-no-dev
3. **AsyncTCP** by me-no-dev

## ESP32 Board Setup

1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add ESP32 board manager URL to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp32" and install **"ESP32 by Espressif Systems"**
6. Select your board: **Tools → Board → ESP32 Arduino → ESP32 Dev Module**

## Building and Flashing

1. **Open the sketch:**
   - Open `croenix_firmware.ino` in Arduino IDE
   - All `.h` and `.cpp` files in the same folder will be automatically included

2. **Configure board settings:**
   - **Board**: "ESP32 Dev Module"
   - **Upload Speed**: 921600
   - **Flash Frequency**: 80MHz
   - **Flash Mode**: QIO
   - **Flash Size**: 4MB (32Mb)
   - **Partition Scheme**: "Default 4MB with spiffs"
   - **Core Debug Level**: "None" (or "Info" for debugging)
   - **Port**: Select your ESP32's COM port

3. **Verify/Compile:**
   - Click the checkmark button or press `Ctrl+R`
   - Wait for compilation to complete
   - Check for any errors

4. **Upload to ESP32:**
   - Connect ESP32 via USB
   - Click the arrow button or press `Ctrl+U`
   - Wait for upload to complete (~30 seconds)

5. **Monitor Serial Output:**
   - Open **Tools → Serial Monitor**
   - Set baud rate to **115200**
   - You should see the startup sequence and connection information

## Default Configuration

### Access Point (Hotspot)
- **SSID**: `Croenix-Audio`
- **Password**: `Croenix@123`
- **IP Address**: `192.168.4.1`

### mDNS Hostname
- **Hostname**: `croenix.local`

### HTTP API Port
- **Port**: `80`

## First Boot

On first boot, the firmware will:
1. Start the WiFi Access Point
2. Enable **pairing mode** (no auth token exists yet)
3. Wait for an Android app to connect and pair
4. You have 30 seconds to complete pairing before it times out

## Pairing Process

1. Connect your Android phone to the `Croenix-Audio` WiFi hotspot
2. Open the Croenix Android app
3. The app will discover the device and request pairing
4. The ESP32 will generate and return an authentication token
5. Future connections will use this token

## Connecting to Your Home Router

Via the Android app:
1. Connect to the Croenix hotspot
2. Pair with the device
3. Send your home WiFi credentials via `POST /api/network/config`
4. The ESP32 will connect to your router while keeping the hotspot active
5. The app can now connect via your home network

## API Endpoints

All endpoints except `/api/pair` require authentication.

### Authentication
Add header: `Authorization: Bearer <token>`

### Available Endpoints

| Endpoint | Method | Auth | Description |
|----------|--------|------|-------------|
| `/api/device` | GET | Yes | Device info |
| `/api/status` | GET | Yes | Complete state |
| `/api/network` | GET | Yes | Network status |
| `/api/system` | GET | Yes | System diagnostics |
| `/api/pair` | POST | No | Generate auth token |
| `/api/commands` | POST | Yes | Execute commands |
| `/api/network/config` | POST | Yes | Configure WiFi |
| `/api/device/restart` | POST | Yes | Restart ESP32 |

## Testing the API

Using curl or Postman:

```bash
# Get device info (replace TOKEN with your actual token)
curl -H "Authorization: Bearer TOKEN" http://192.168.4.1/api/device

# Get full status
curl -H "Authorization: Bearer TOKEN" http://192.168.4.1/api/status

# Set volume
curl -X POST -H "Authorization: Bearer TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"command":"set_volume","params":{"value":75}}' \
  http://192.168.4.1/api/commands
```

## Troubleshooting

### Upload Failed
- Check USB cable connection
- Try a different USB port
- Hold the BOOT button while uploading
- Check COM port selection

### No Serial Output
- Check baud rate is set to 115200
- Try pressing the EN (reset) button on the ESP32
- Verify DEBUG_SERIAL is set to true in Config.h

### WiFi AP Not Visible
- Check antenna is connected (if external)
- Move closer to the ESP32
- Check the access point isn't full (max 4 clients)
- Restart the ESP32

### Can't Connect to Router
- Verify WiFi credentials are correct
- Check 2.4GHz band (ESP32 doesn't support 5GHz)
- Check router settings (MAC filtering, hidden SSID)
- Check Serial Monitor for connection errors

### HTTP Requests Timeout
- Verify ESP32 is powered properly (USB should provide enough power)
- Check firewall settings on your computer/phone
- Verify IP address is correct
- Try pinging the ESP32: `ping 192.168.4.1`

## Phase 1 & 2 Limitations

This phase implements **network foundation only**:
- ✅ WiFi AP+STA mode
- ✅ HTTP REST API
- ✅ Authentication and pairing
- ✅ Device discovery (mDNS + UDP)
- ✅ System monitoring

**Not yet implemented** (coming in Phase 3+):
- ❌ Audio playback
- ❌ Rotary encoder input
- ❌ TFT display output
- ❌ SD card access
- ❌ Bluetooth audio
- ❌ FM radio
- ❌ WebSocket real-time updates

## File Structure

```
croenix_firmware/
├── croenix_firmware.ino    # Main sketch
├── Config.h                # Configuration constants
├── StateManager.h/cpp      # Central state management
├── WiFiManager.h/cpp       # AP+STA WiFi management
├── AuthManager.h/cpp       # Authentication and pairing
├── SystemMonitor.h/cpp     # System diagnostics
├── DiscoveryManager.h/cpp  # mDNS and UDP discovery
├── CommandManager.h/cpp    # Command processing
├── WebServerManager.h/cpp  # HTTP REST API server
└── README.md              # This file
```

## Next Steps

After successful firmware upload:
1. Build and install the Android application
2. Test device discovery
3. Complete pairing
4. Test all API endpoints
5. Proceed to Phase 3 (hardware integration)

## Support

For issues or questions, refer to:
- Main project documentation
- ESP32 Arduino Core documentation: https://docs.espressif.com/projects/arduino-esp32/
- ArduinoJson documentation: https://arduinojson.org/
