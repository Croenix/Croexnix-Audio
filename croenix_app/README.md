# Croenix Audio - Flutter Mobile App

## Features

- **Glassmorphism UI Design** - Modern translucent interface with smooth animations
- **Real-time Device Communication** - WebSocket-based bidirectional communication
- **Auto Device Discovery** - mDNS service discovery for zero-configuration pairing
- **Audio Source Management** - Bluetooth, SD Card, FM Radio, Web Streaming
- **Live Visualizer** - Animated audio spectrum visualization
- **Wi-Fi Provisioning** - Easy network setup wizard
- **Remote Control** - Volume, playback, EQ, source switching
- **File Browser** - Navigate and play audio files from SD card
- **FM Radio Tuner** - Frequency selection with station presets

## Setup

### Prerequisites

- Flutter SDK (3.0.0 or higher)
- Android Studio / Xcode
- ESP32 device running Croenix firmware

### Installation

```bash
cd croenix_app
flutter pub get
flutter run
```

### Build Release

```bash
# Android
flutter build apk --release

# iOS
flutter build ios --release
```

## Project Structure

```
lib/
├── app.dart                    # Main app widget
├── main.dart                   # Entry point
├── core/
│   ├── theme/
│   │   ├── croenix_colors.dart # Color palette & glassmorphism
│   │   └── croenix_theme.dart  # Material theme
│   └── constants/
├── data/
│   ├── models/
│   │   ├── device_state.dart   # Device state model
│   │   └── discovered_device.dart
│   └── services/
│       ├── websocket_service.dart    # WebSocket client
│       └── discovery_service.dart    # mDNS discovery
└── features/
    ├── home/
    │   ├── home_screen.dart
    │   └── widgets/
    ├── player/
    │   ├── player_screen.dart
    │   └── widgets/
    │       ├── audio_visualizer.dart
    │       ├── control_buttons.dart
    │       ├── track_progress_bar.dart
    │       └── volume_control.dart
    ├── wifi_setup/
    │   ├── wifi_setup_screen.dart
    │   └── widgets/
    └── settings/
        ├── settings_screen.dart
        └── widgets/
```

## Architecture

- **State Management**: Provider / Riverpod
- **Network**: WebSocket + HTTP
- **Device Discovery**: Multicast DNS
- **Storage**: Hive + SharedPreferences
- **UI**: Material 3 + Custom Glassmorphism

## Configuration

### WebSocket Connection

The app automatically discovers ESP32 devices on the network via mDNS (`esp32-audio.local:8080`).

### Manual Connection

If auto-discovery fails, you can manually enter the IP address in the Wi-Fi setup screen.

## Features Breakdown

### Home Screen
- Connection status indicator
- Now playing card with quick access
- Quick controls for source switching
- Navigation to full player

### Player Screen
- Full-screen audio visualizer
- Track metadata display
- Playback controls (play/pause/skip)
- Volume slider
- EQ preset selector
- Source switcher

### Wi-Fi Setup
- Device discovery via mDNS
- Network scanner
- Wi-Fi configuration wizard
- Connection status

### Settings
- Device information
- Network configuration
- Storage management
- System info (CPU, memory, uptime)
- About & licenses

## Customization

### Colors

Edit `lib/core/theme/croenix_colors.dart` to customize the color palette.

### Theme

Modify `lib/core/theme/croenix_theme.dart` for theme adjustments.

## Dependencies

Key packages used:
- `web_socket_channel` - WebSocket communication
- `multicast_dns` - Device discovery
- `provider` - State management
- `shared_preferences` - Local storage
- `glassmorphism` - Glass effect widgets

## License

MIT License - See LICENSE file for details
