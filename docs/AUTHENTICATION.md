# Croenix Security & Authentication Architecture

## 1. Token Model

- **Format**: 32-character cryptographically pseudorandom alphanumeric string.
- **RNG Source**: Hardware true random number generator (`esp_random()`).
- **Persistence**: Committed to non-volatile flash storage using ESP32 `Preferences` in the `croenix` namespace.
- **Client Storage**: Persisted in Android Keystore / `FlutterSecureStorage` by the Croenix Flutter application.

---

## 2. Pairing Workflow

```mermaid
sequenceDiagram
    participant User
    participant App as Croenix Android App
    participant ESP32 as Croenix ESP32 Player

    Note over ESP32: On first boot (no token in NVS)<br/>Pairing mode automatically open
    User->>App: Tap "Connect"
    App->>ESP32: POST /api/pair
    ESP32-->>App: {"success": true, "token": "..."}
    App->>App: Save token in FlutterSecureStorage
    App->>ESP32: GET /api/device (Bearer token)
    ESP32-->>App: Device Info (Access Granted)

    Note over ESP32: On subsequent boots with token saved:
    User->>ESP32: Long press KY-040 Encoder (2 seconds)
    Note over ESP32: Pairing Window OPEN for 30s
    App->>ESP32: POST /api/pair
    ESP32-->>App: {"success": true, "token": "new_token"}
```

---

## 3. Brute-Force Rate Limiting

- **Threshold**: 10 consecutive invalid authorization attempts.
- **Lockout Duration**: 60 seconds (`AUTH_LOCKOUT_DURATION_MS`).
- **Timing Protection**: Token comparison uses constant-time XOR byte iteration to prevent side-channel timing analysis.

