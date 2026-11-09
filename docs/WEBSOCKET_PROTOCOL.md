# Croenix WebSocket Protocol Specification

Endpoint:
```
ws://192.168.4.1/ws
ws://croenix.local/ws
ws://<router-ip>/ws
```

---

## 1. Connection Lifecycle

1. Client connects via standard WebSocket handshake.
2. Upon connection, the ESP32 automatically transmits a complete `state_snapshot` event containing the full player state.
3. Client can authenticate by sending an auth command:
   ```json
   {
     "command": "auth",
     "token": "your_32_character_token"
   }
   ```

---

## 2. Server-Pushed Live Events

Each broadcast event includes `event`, sequence number `seq`, `timestamp` in milliseconds, and `payload`:

### `volume_changed`
```json
{
  "event": "volume_changed",
  "seq": 14,
  "timestamp": 128450,
  "payload": {
    "volume": 75
  }
}
```

### `mute_changed`
```json
{
  "event": "mute_changed",
  "seq": 15,
  "timestamp": 130120,
  "payload": {
    "muted": true
  }
}
```

### `source_changed`
```json
{
  "event": "source_changed",
  "seq": 16,
  "timestamp": 135400,
  "payload": {
    "source": "bluetooth"
  }
}
```

### `playback_changed`
```json
{
  "event": "playback_changed",
  "seq": 17,
  "timestamp": 135420,
  "payload": {
    "status": "playing"
  }
}
```

### `track_changed`
```json
{
  "event": "track_changed",
  "seq": 18,
  "timestamp": 135450,
  "payload": {
    "title": "Hotel California",
    "artist": "Eagles",
    "duration_ms": 390000
  }
}
```

### `wifi_changed`
```json
{
  "event": "wifi_changed",
  "seq": 19,
  "timestamp": 140000,
  "payload": {
    "router_connected": true,
    "router_ip": "192.168.1.105",
    "router_ssid": "HomeWiFi",
    "router_rssi": -55,
    "hotspot_active": true,
    "hotspot_clients": 1,
    "active_interface": "router"
  }
}
```

---

## 3. Client Command Dispatch via WebSocket

Clients can send remote control commands directly through the WebSocket:
```json
{
  "command": "set_volume",
  "params": {
    "value": 85
  },
  "request_id": "ws-cmd-101"
}
```

Server replies with command execution result:
```json
{
  "success": true,
  "command": "set_volume",
  "request_id": "ws-cmd-101",
  "applied_value": 85
}
```
