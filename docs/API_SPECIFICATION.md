# Croenix REST API Specification

Base URLs:
- Direct Hotspot: `http://192.168.4.1`
- mDNS Hostname: `http://croenix.local`
- Local Router IP: `http://<assigned-ip>`

All protected endpoints require the header:
```http
Authorization: Bearer <token>
```

---

## 1. GET /api/device
- **Auth**: Required
- **Response**:
```json
{
  "name": "Croenix",
  "device_id": "croenix-4a2b1c",
  "firmware_version": "1.0.0",
  "hardware": "ESP32-WROOM-32",
  "uptime_seconds": 182,
  "capabilities": [
    "wifi_ap_sta",
    "ble",
    "websocket",
    "audio_control"
  ]
}
```

---

## 2. GET /api/status
- **Auth**: Required
- **Response**: Strictly matches the Flutter `DeviceState.fromJson()` model:
```json
{
  "device": {
    "name": "Croenix",
    "firmware_version": "1.0.0",
    "uptime_seconds": 182
  },
  "connection": {
    "router_connected": true,
    "router_ip": "192.168.1.105",
    "router_ssid": "HomeWiFi",
    "router_rssi": -58,
    "hotspot_active": true,
    "hotspot_clients": 1,
    "active_interface": "router"
  },
  "audio": {
    "source": "bluetooth",
    "status": "playing",
    "volume": 75,
    "muted": false,
    "codec": "PCM",
    "sample_rate": 44100,
    "bit_depth": 16,
    "channels": 2
  },
  "storage": {
    "sd_inserted": false,
    "total_bytes": 0,
    "used_bytes": 0,
    "free_bytes": 0
  },
  "system": {
    "free_heap": 215400,
    "total_heap": 327680,
    "min_free_heap": 198200,
    "restart_reason": "power_on"
  }
}
```

---

## 3. GET /api/network
- **Auth**: Required
- **Response**:
```json
{
  "ap": {
    "active": true,
    "ssid": "Croenix-Audio",
    "ip": "192.168.4.1",
    "clients": 1
  },
  "router": {
    "connected": true,
    "ssid": "HomeWiFi",
    "ip": "192.168.1.105",
    "rssi": -58
  },
  "ble": {
    "active": true,
    "connected": false
  },
  "active_interface": "router"
}
```

---

## 4. GET /api/system
- **Auth**: Required
- **Response**:
```json
{
  "free_heap": 215400,
  "total_heap": 327680,
  "min_free_heap": 198200,
  "uptime_seconds": 182,
  "restart_reason": "power_on",
  "wifi_reconnect_count": 0,
  "firmware_version": "1.0.0",
  "hardware": "ESP32-WROOM-32"
}
```

---

## 5. POST /api/pair
- **Auth**: None (Unauthenticated)
- **Response (Success - Pairing Active)**:
```json
{
  "success": true,
  "token": "a1b2c3d4e5f67890123456789abcdef0",
  "message": "Pairing successful"
}
```
- **Response (Pairing Inactive)**: HTTP 403 Forbidden
```json
{
  "error": {
    "code": "PAIRING_INACTIVE",
    "message": "Pairing mode not active. Press encoder button on device."
  }
}
```

---

## 6. POST /api/commands
- **Auth**: Required
- **Payload**:
```json
{
  "command": "set_volume",
  "params": {
    "value": 80
  }
}
```
- **Response (Success)**:
```json
{
  "success": true,
  "command": "set_volume",
  "applied_value": 80
}
```
- **Response (Unsupported / Not Installed)**:
```json
{
  "success": false,
  "error": {
    "code": "NOT_IMPLEMENTED",
    "message": "Audio source 'fm' hardware pipeline is not yet attached"
  }
}
```

---

## 7. POST /api/network/config
- **Auth**: Required
- **Payload**:
```json
{
  "ssid": "MyWiFi",
  "password": "MySecretPassword"
}
```
- **Response**:
```json
{
  "success": true,
  "message": "WiFi configuration accepted. Connecting to router..."
}
```

---

## 8. POST /api/device/restart
- **Auth**: Required
- **Response**:
```json
{
  "success": true,
  "message": "Croenix player is restarting..."
}
```
*(Device reboots safely 1.5 seconds later after the HTTP response packet is delivered).*

