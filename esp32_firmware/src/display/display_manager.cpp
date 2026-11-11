#include "display_manager.h"
#include "device_state.h"
#include "encoder_manager.h"
#include "fm_radio.h"
#include "sd_manager.h"
#include "qrcode.h"

// TFT instance
TFT_eSPI tft = TFT_eSPI();

// Display state tracking
static bool displayInitialized = false;
static unsigned long lastUpdate = 0;
static bool needFullRedraw = true;
static bool wasOverlayActive = false;
static UIScreenState currentScreen = UI_SCREEN_BOOT_SPLASH;

// Previous state tracking for differential flicker-free updates
static AudioSource lastSource = SOURCE_NONE;
static bool lastPlaying = false;
static bool lastWifi = false;
static int lastVolume = -1;
static uint32_t lastPos = 0;
static float lastFMFreq = -1.0f;
static bool lastFMTuneState = false;
static char lastTitle[MAX_METADATA_LENGTH] = "";
static BluetoothState lastBTState = (BluetoothState)255;
static uint8_t lastDiagModule = 255;
static unsigned long lastBlinkToggle = 0;
static bool blinkRedState = false;
static bool blinkBlueState = false;

// Audio Spectrum Visualizer levels
static const int VIZ_BARS = 12;
static uint8_t vizLevels[VIZ_BARS] = {0};
static uint8_t vizPeaks[VIZ_BARS] = {0};

// Cyber RGB565 Color Palette
#define COLOR_DEEP_BG       0x0000  // Solid Pure Black (Zero-State)
#define COLOR_PANEL_BG      0x1084  // Deep Cyber Slate (R:2, G:4, B:4)
#define COLOR_BORDER        0x2128  // Subtle Glass Border
#define COLOR_PRIMARY_CYAN  0x07FF  // Electric Cyan
#define COLOR_NEON_PURPLE   0x999F  // Neon Violet
#define COLOR_NEON_PINK     0xF819  // Neon Magenta
#define COLOR_NEON_GREEN    0x07E0  // Electric Green
#define COLOR_AMBER         0xFDA0  // Warm Amber
#define COLOR_WHITE         0xFFFF  // Pure White
#define COLOR_MUTED_GRAY    0x8410  // Soft Muted Text
#define COLOR_RED_ALERT     0xF800  // Warning Red
#define COLOR_BLUE_ACTIVE   0x001F  // Solid Blue for BT Connected
#define COLOR_BLUE_CYAN     0x3DFF  // Electric Light Blue for BT Pairing

// -------------------------------------------------------------
// Zero-State Boot Initialization
// -------------------------------------------------------------

void initDisplayManager() {
    DEBUG_PRINTLN("[DISPLAY] Initializing ST7735 with Zero-State Black Boot...");

    // Deselect both SPI slaves before bus setup
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    // 1. Initialize hardware SPI display
    tft.init();
    tft.setRotation(TFT_ROTATION); // 1 = Landscape (160x128 pixels)

    // 2. ZERO-STATE BOOT: Instantly blank display to solid black background.
    // Prevents any white flashes, memory garbage, or default graphic artifacts.
    tft.fillScreen(COLOR_DEEP_BG);
    digitalWrite(TFT_CS, HIGH);
    delay(50); // Allow TFT power rails and charge pumps to stabilize

    displayInitialized = true;

    // 3. Render centralized Splash Screen
    showSplashScreen();
    digitalWrite(TFT_CS, HIGH);

    needFullRedraw = true;
    currentScreen = UI_SCREEN_HOME;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_deviceState.activeScreen = UI_SCREEN_HOME;
        xSemaphoreGive(g_stateMutex);
    }
}

// -------------------------------------------------------------
// Splash Screen: "CROENIX AUDIOS"
// -------------------------------------------------------------

void showSplashScreen() {
    tft.fillScreen(COLOR_DEEP_BG);

    int w = tft.width();
    int h = tft.height();

    // Geometric accent lines
    tft.drawFastHLine(0, 0, w, COLOR_PRIMARY_CYAN);
    tft.drawFastHLine(0, 1, w, COLOR_NEON_PURPLE);
    tft.drawFastHLine(0, h - 2, w, COLOR_NEON_PURPLE);
    tft.drawFastHLine(0, h - 1, w, COLOR_PRIMARY_CYAN);

    // Center Branding Box
    int centerX = w / 2;
    int logoY = 24;

    tft.drawRoundRect(centerX - 70, logoY - 14, 140, 48, 6, COLOR_BORDER);
    tft.drawRoundRect(centerX - 68, logoY - 12, 136, 44, 5, COLOR_PRIMARY_CYAN);

    // Main Brand Name
    tft.setTextSize(2);
    tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
    tft.drawString("CROENIX", centerX - 42, logoY - 4);

    tft.setTextSize(1);
    tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
    tft.drawString("AUDIOS", centerX - 18, logoY + 16);

    // Subtitle
    tft.setTextColor(COLOR_NEON_PURPLE, COLOR_DEEP_BG);
    tft.drawString("HI-FI DSP ENGINE", centerX - 46, logoY + 40);

    // Fast zero-delay loading sequence
    int barY = 86;
    int barW = w - 40;
    int barX = 20;
    tft.drawRoundRect(barX - 1, barY - 1, barW + 2, 8, 3, COLOR_BORDER);

    const char* bootLabels[] = {
        "INIT HARDWARE...",
        "WCMCU I2S DAC   [OK]",
        "HW-322 FM RADIO [OK]",
        "SD SPI INTERFACE[OK]",
        "ROTARY DETENT   [OK]",
        "CROENIX READY!"
    };

    for (int step = 0; step < 6; step++) {
        tft.fillRect(10, barY + 14, w - 20, 12, COLOR_DEEP_BG);
        tft.setTextSize(1);
        tft.setTextColor((step == 5) ? COLOR_NEON_GREEN : COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
        tft.drawString(bootLabels[step], 16, barY + 14);

        float progress = (step + 1) / 6.0f;
        int fillW = (int)(barW * progress);
        if (fillW > 0) {
            uint16_t col = (step < 3) ? COLOR_PRIMARY_CYAN : ((step < 5) ? COLOR_NEON_PURPLE : COLOR_NEON_GREEN);
            tft.fillRoundRect(barX, barY, fillW, 6, 2, col);
        }
        delay(180);
    }

    delay(200);
}

// -------------------------------------------------------------
// Non-Blocking State Machine UI Update Loop
// -------------------------------------------------------------

void updateDisplay() {
    if (!displayInitialized) return;

    unsigned long now = millis();
    if (now - lastUpdate < DISPLAY_UPDATE_RATE) return;
    lastUpdate = now;

    // Bluetooth LED blink ticker (Red: 500ms, Blue Pairing: 250ms)
    if (now - lastBlinkToggle >= 250) {
        lastBlinkToggle = now;
        blinkBlueState = !blinkBlueState;
        static uint8_t redCount = 0;
        if (++redCount >= 2) {
            redCount = 0;
            blinkRedState = !blinkRedState;
        }
    }

    // Check volume HUD overlay
    if (isVolumeOverlayActive()) {
        wasOverlayActive = true;
        drawVolumeHUD(getVolumeOverlayValue());
        return;
    } else if (wasOverlayActive) {
        wasOverlayActive = false;
        needFullRedraw = true;
    }

    // State Machine View Navigation
    UIScreenState active = g_deviceState.activeScreen;
    if (active != currentScreen || g_deviceState.needDisplayRedraw) {
        needFullRedraw = true;
        currentScreen = active;
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            g_deviceState.needDisplayRedraw = false;
            xSemaphoreGive(g_stateMutex);
        }
    }

    switch (currentScreen) {
        case UI_SCREEN_HARDWARE_DIAGNOSTICS:
            showHardwareDiagnosticsScreen();
            break;
        case UI_SCREEN_PROVISIONING_QR:
            showProvisioningQRScreen();
            break;
        case UI_SCREEN_HOME:
        default:
            showMainScreen();
            break;
    }
}

// -------------------------------------------------------------
// 1. HARDWARE STATUS & DIAGNOSTICS (10-Second Long Press)
// -------------------------------------------------------------

void showHardwareDiagnosticsScreen() {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(15)) != pdTRUE) return;

    int w = tft.width();
    int h = tft.height();
    uint8_t mod = g_deviceState.selectedDiagModule;

    if (needFullRedraw || mod != lastDiagModule) {
        tft.fillScreen(COLOR_DEEP_BG);

        // Header Panel
        tft.fillRect(0, 0, w, 18, COLOR_PANEL_BG);
        tft.drawFastHLine(0, 18, w, COLOR_NEON_PURPLE);

        tft.setTextSize(1);
        tft.setTextColor(COLOR_AMBER, COLOR_PANEL_BG);
        tft.drawString("HARDWARE STATUS", 6, 5);

        // Module index indicator [1/4]
        char idxStr[12];
        snprintf(idxStr, sizeof(idxStr), "[%d/4]", mod + 1);
        tft.setTextColor(COLOR_WHITE, COLOR_PANEL_BG);
        tft.drawString(idxStr, w - 36, 5);

        // Content Card Container
        tft.drawRoundRect(4, 22, w - 8, 86, 4, COLOR_BORDER);
        tft.fillRect(5, 23, w - 10, 84, COLOR_DEEP_BG);

        tft.setTextSize(1);

        switch (mod) {
            case 0: { // WCMCU DAC
                tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
                tft.drawString("1. WCMCU I2S DAC", 10, 24);
                tft.drawFastHLine(10, 34, w - 20, COLOR_BORDER);

                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                tft.drawString("TI PCM5102A (I2S DMA)", 10, 38);
                tft.drawString("DIN27 BCK26 LRC25", 10, 48);

                // Hardware Jumper Wiring Directives
                tft.setTextColor(COLOR_AMBER, COLOR_DEEP_BG);
                tft.drawString("SCK->GND (PLL ON)", 10, 58);
                tft.drawString("XSMT->3.3V (UNMUTE)", 10, 68);

                char volDbg[28];
                snprintf(volDbg, sizeof(volDbg), "Vol:%d%% | I2S Gain:%d",
                         g_deviceState.volume, map(g_deviceState.volume, 0, 100, 0, 21));
                tft.drawString(volDbg, 10, 78);

                uint16_t dacCol = g_deviceState.isPlaying ? COLOR_NEON_GREEN : COLOR_PRIMARY_CYAN;
                tft.fillRoundRect(10, 89, 82, 13, 3, dacCol);
                tft.setTextColor(COLOR_DEEP_BG, dacCol);
                tft.drawString(g_deviceState.isPlaying ? "STREAMING" : "I2S READY", 14, 91);
                break;
            }

            case 1: { // HW-322 FM Module
                tft.setTextColor(COLOR_NEON_PINK, COLOR_DEEP_BG);
                tft.drawString("2. HW-322 FM RADIO", 10, 26);
                tft.drawFastHLine(10, 37, w - 20, COLOR_BORDER);

                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                tft.drawString("Chip: Si4703/TEA (I2C)", 10, 42);
                tft.drawString("Bus: SDA21, SCL22, RST16", 10, 53);

                char freqStr[28];
                snprintf(freqStr, sizeof(freqStr), "Tuned Freq: %.1f MHz", g_deviceState.fmFrequency);
                tft.drawString(freqStr, 10, 64);

                char rssiStr[28];
                snprintf(rssiStr, sizeof(rssiStr), "Signal Level: %d%% RSSI", g_deviceState.fmSignalStrength);
                tft.drawString(rssiStr, 10, 75);

                bool isOk = g_deviceState.fmOperational || checkFMIntegrity();
                uint16_t statusCol = isOk ? COLOR_NEON_GREEN : COLOR_AMBER;
                tft.fillRoundRect(10, 88, 82, 14, 3, statusCol);
                tft.setTextColor(COLOR_DEEP_BG, statusCol);
                tft.drawString(isOk ? "ONLINE OK" : "CHECK I2C", 16, 91);
                break;
            }

            case 2: { // SD Card Reader
                tft.setTextColor(COLOR_AMBER, COLOR_DEEP_BG);
                tft.drawString("3. SD CARD READER", 10, 24);
                tft.drawFastHLine(10, 34, w - 20, COLOR_BORDER);

                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                tft.drawString("SPI: CS15 SCK18 MOSI23 MISO19", 10, 38);

                // Auto-retry mount if currently not mounted
                if (!g_deviceState.sdCardMounted) {
                    checkSDIntegrity();
                }

                if (g_deviceState.sdCardMounted) {
                    char sizeStr[28];
                    snprintf(sizeStr, sizeof(sizeStr), "Capacity: %llu MB", g_deviceState.sdCardSize / (1024 * 1024));
                    tft.drawString(sizeStr, 10, 50);

                    char trkStr[28];
                    snprintf(trkStr, sizeof(trkStr), "Tracks: %u Audio Files", g_deviceState.sdTotalTracks);
                    tft.drawString(trkStr, 10, 62);
                    tft.drawString("FS Format: FAT32 OK", 10, 74);

                    tft.fillRoundRect(10, 89, 82, 13, 3, COLOR_NEON_GREEN);
                    tft.setTextColor(COLOR_DEEP_BG, COLOR_NEON_GREEN);
                    tft.drawString("MOUNTED OK", 14, 91);
                } else {
                    tft.setTextColor(COLOR_RED_ALERT, COLOR_DEEP_BG);
                    tft.drawString("Status: Not Mounted", 10, 50);
                    tft.drawString("Check: Card inserted?", 10, 62);
                    tft.drawString("Check: FAT32 formatted?", 10, 74);

                    tft.fillRoundRect(10, 89, 82, 13, 3, COLOR_RED_ALERT);
                    tft.setTextColor(COLOR_WHITE, COLOR_RED_ALERT);
                    tft.drawString("NO MEDIA", 20, 91);
                }
                break;
            }

            case 3: { // System & Display
                tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
                tft.drawString("4. ESP32 SYSTEM", 10, 26);
                tft.drawFastHLine(10, 37, w - 20, COLOR_BORDER);

                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                char heapStr[28];
                snprintf(heapStr, sizeof(heapStr), "Free Heap: %u KB", g_deviceState.freeHeap / 1024);
                tft.drawString(heapStr, 10, 42);

                tft.drawString("Audio Core: Core 1 [DMA]", 10, 53);
                tft.drawString("Control: Core 0 [RTOS]", 10, 64);
                tft.drawString("Display: ST7735 DMA OK", 10, 75);

                tft.fillRoundRect(10, 88, 82, 14, 3, COLOR_NEON_GREEN);
                tft.setTextColor(COLOR_DEEP_BG, COLOR_NEON_GREEN);
                tft.drawString("PASS (OK)", 24, 91);
                break;
            }
        }

        // Bottom Footer
        tft.fillRect(0, h - 18, w, 18, COLOR_PANEL_BG);
        tft.drawFastHLine(0, h - 19, w, COLOR_BORDER);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_MUTED_GRAY, COLOR_PANEL_BG);
        tft.drawString("Turn: Scroll | Click: Exit", 12, h - 14);

        lastDiagModule = mod;
        needFullRedraw = false;
    }

    xSemaphoreGive(g_stateMutex);
}

// -------------------------------------------------------------
// 2. DIRECT ON-SCREEN PROVISIONING QR CODE
// -------------------------------------------------------------

void showProvisioningQRScreen() {
    int w = tft.width();
    int h = tft.height();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);

        // Header
        tft.fillRect(0, 0, w, 18, COLOR_PANEL_BG);
        tft.drawFastHLine(0, 18, w, COLOR_PRIMARY_CYAN);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_PANEL_BG);
        tft.drawString("PROVISIONING PORTAL", 8, 5);

        // Dynamic QR generation
        QRCode qrcode;
        uint8_t qrcodeBytes[128];
        char payload[96];
        snprintf(payload, sizeof(payload), "{\"net\":\"CRX\",\"ap\":\"%s\",\"ip\":\"%s\",\"tok\":\"%s\"}",
                 AP_SSID_PREFIX, AP_STATIC_IP, g_deviceState.provisioningToken);

        qrcode_initText(&qrcode, qrcodeBytes, 3, ECC_LOW, payload);

        // Center on screen
        int startX = (w - (qrcode.size * 2)) / 2;
        int startY = 24;

        // White Quiet Zone border
        tft.fillRect(startX - 4, startY - 4, (qrcode.size * 2) + 8, (qrcode.size * 2) + 8, COLOR_WHITE);

        for (uint8_t y = 0; y < qrcode.size; y++) {
            for (uint8_t x = 0; x < qrcode.size; x++) {
                uint16_t c = qrcode_getModule(&qrcode, x, y) ? COLOR_DEEP_BG : COLOR_WHITE;
                tft.fillRect(startX + (x * 2), startY + (y * 2), 2, 2, c);
            }
        }

        // Scan Instructions below QR code
        tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
        tft.drawString("SCAN IN CROENIX APP", 18, startY + (qrcode.size * 2) + 8);
        tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);
        tft.drawString("Zero-Handoff Wi-Fi Sync", 16, startY + (qrcode.size * 2) + 20);

        // Bottom Footer
        tft.fillRect(0, h - 16, w, 16, COLOR_PANEL_BG);
        tft.drawFastHLine(0, h - 17, w, COLOR_BORDER);
        tft.setTextColor(COLOR_AMBER, COLOR_PANEL_BG);
        tft.drawString("Click: Back to Home", 22, h - 12);

        needFullRedraw = false;
    }
}

// -------------------------------------------------------------
// 3. HOME SCREEN WITH CONTEXTUAL RENDERING PER ACTIVE INPUT
// -------------------------------------------------------------

void showMainScreen() {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(15)) != pdTRUE) return;

    int w = tft.width();
    int h = tft.height();

    // Check if mode switched or full redraw is needed
    if (needFullRedraw || g_deviceState.currentSource != lastSource) {
        tft.fillScreen(COLOR_DEEP_BG);

        // Top Status Bar Panel
        tft.fillRect(0, 0, w, 18, COLOR_PANEL_BG);
        tft.drawFastHLine(0, 18, w, COLOR_BORDER);

        // Bottom Footer Bar
        tft.fillRect(0, h - 18, w, 18, COLOR_PANEL_BG);
        tft.drawFastHLine(0, h - 19, w, COLOR_BORDER);

        needFullRedraw = false;
        lastSource = g_deviceState.currentSource;
        lastTitle[0] = '\0';
        lastFMFreq = -1.0f;
        lastPos = 999999;
        lastVolume = -1;
        lastBTState = (BluetoothState)255;
        lastFMTuneState = false;
    }

    // --------------------------------------------
    // TOP STATUS BAR (Source Pill, Play Icon, Wi-Fi)
    // --------------------------------------------
    const char* srcText = "BT";
    uint16_t srcColor = COLOR_PRIMARY_CYAN;

    switch (g_deviceState.currentSource) {
        case SOURCE_BLUETOOTH:   srcText = "BT";    srcColor = COLOR_PRIMARY_CYAN; break;
        case SOURCE_SD_CARD:     srcText = "SD";    srcColor = COLOR_AMBER;        break;
        case SOURCE_FM_RADIO:    srcText = "FM";    srcColor = COLOR_NEON_PINK;    break;
        case SOURCE_WEB_STREAM:  srcText = "WEB";   srcColor = COLOR_NEON_GREEN;   break;
        default:                 srcText = "---";   srcColor = COLOR_MUTED_GRAY;   break;
    }

    // Source Pill
    tft.fillRoundRect(3, 2, 34, 14, 3, srcColor);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_DEEP_BG, srcColor);
    tft.drawString(srcText, (strlen(srcText) <= 2) ? 14 : 6, 5);

    // Play/Pause State Icon
    tft.fillRect(40, 3, 20, 12, COLOR_PANEL_BG);
    if (g_deviceState.isPlaying) {
        tft.fillTriangle(42, 4, 42, 14, 50, 9, COLOR_NEON_GREEN);
    } else {
        tft.fillRect(42, 4, 3, 10, COLOR_RED_ALERT);
        tft.fillRect(47, 4, 3, 10, COLOR_RED_ALERT);
    }

    // Network Status on Right
    tft.fillRect(w - 62, 3, 60, 12, COLOR_PANEL_BG);
    tft.setTextSize(1);
    if (g_deviceState.wifiConnected) {
        tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_PANEL_BG);
        tft.drawString("WiFi:Home", w - 58, 5);
    } else if (g_deviceState.apMode) {
        tft.setTextColor(COLOR_AMBER, COLOR_PANEL_BG);
        tft.drawString("AP:Setup", w - 54, 5);
    } else {
        tft.setTextColor(COLOR_MUTED_GRAY, COLOR_PANEL_BG);
        tft.drawString("OFFLINE", w - 48, 5);
    }

    // --------------------------------------------
    // CONTEXTUAL INPUT RENDERING
    // --------------------------------------------
    switch (g_deviceState.currentSource) {

        // ============================================
        // A. BLUETOOTH MODE (Large BT Branding & Dynamic Status)
        // ============================================
        case SOURCE_BLUETOOTH: {
            BluetoothState bt = g_deviceState.bluetoothState;
            if (bt != lastBTState || (bt != BT_STATE_CONNECTED)) {
                tft.fillRect(4, 20, w - 8, 66, COLOR_DEEP_BG);

                // Indicator Color Code Logic:
                // - Blinking red for disconnected
                // - Solid blue for connected
                // - Blinking blue during pairing
                uint16_t statusColor;
                const char* statusText;

                if (bt == BT_STATE_CONNECTED) {
                    statusColor = COLOR_BLUE_ACTIVE;
                    statusText = "CONNECTED";
                } else if (bt == BT_STATE_PAIRING) {
                    statusColor = blinkBlueState ? COLOR_BLUE_CYAN : COLOR_DEEP_BG;
                    statusText = "PAIRING...";
                } else { // BT_STATE_DISCONNECTED
                    statusColor = blinkRedState ? COLOR_RED_ALERT : COLOR_DEEP_BG;
                    statusText = "DISCONNECTED";
                }

                // Large "BT" Branding
                tft.setTextSize(4);
                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                tft.drawString("BT", 18, 30);

                // Dynamic Status Circle Indicator
                tft.drawCircle(78, 44, 18, COLOR_BORDER);
                tft.drawCircle(78, 44, 14, statusColor);
                tft.fillCircle(78, 44, 8, statusColor);

                // Status Label & Device Metadata
                tft.setTextSize(1);
                tft.setTextColor(statusColor != COLOR_DEEP_BG ? statusColor : COLOR_MUTED_GRAY, COLOR_DEEP_BG);
                tft.drawString(statusText, 104, 34);

                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                tft.drawString(g_deviceState.bluetoothDevice, 104, 48);

                tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);
                tft.drawString("A2DP Sink 44.1k", 104, 62);

                lastBTState = bt;
            }
            break;
        }

        // ============================================
        // B. SD CARD MODE (ID3 Tags & Progress Bar)
        // ============================================
        case SOURCE_SD_CARD: {
            // Real-time ID3 tags
            if (strcmp(g_deviceState.trackTitle, lastTitle) != 0 || needFullRedraw) {
                tft.fillRect(4, 20, w - 8, 32, COLOR_DEEP_BG);

                tft.setTextSize(1);
                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                String title = (strlen(g_deviceState.trackTitle) > 0)
                               ? String(g_deviceState.trackTitle)
                               : (g_deviceState.sdCardMounted ? "SD Audio Track" : "No SD Card");
                if (title.length() > 22) title = title.substring(0, 20) + "..";
                tft.drawString(title, 8, 22);

                // Artist
                tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
                String artist = (strlen(g_deviceState.trackArtist) > 0)
                                ? String(g_deviceState.trackArtist) : "ID3: Croenix SD";
                if (artist.length() > 24) artist = artist.substring(0, 22) + "..";
                tft.drawString(artist, 8, 33);

                // Album / Folder
                tft.setTextColor(COLOR_AMBER, COLOR_DEEP_BG);
                String album = (strlen(g_deviceState.trackAlbum) > 0)
                               ? String(g_deviceState.trackAlbum) : g_deviceState.currentFolder;
                if (album.length() > 24) album = album.substring(0, 22) + "..";
                tft.drawString(album, 8, 44);

                strncpy(lastTitle, g_deviceState.trackTitle, MAX_METADATA_LENGTH - 1);
            }

            // Track Progress Bar & Time Digits
            if (g_deviceState.trackPosition != lastPos || g_deviceState.trackDuration > 0) {
                float progress = (g_deviceState.trackDuration > 0)
                                 ? (float)g_deviceState.trackPosition / (float)g_deviceState.trackDuration
                                 : 0.0f;

                drawModernProgressBar(8, 57, w - 16, 4, progress, COLOR_AMBER, COLOR_BORDER);

                tft.fillRect(8, 64, w - 16, 9, COLOR_DEEP_BG);
                tft.setTextSize(1);
                tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);

                char timeStr[28];
                int pM = g_deviceState.trackPosition / 60;
                int pS = g_deviceState.trackPosition % 60;
                int dM = g_deviceState.trackDuration / 60;
                int dS = g_deviceState.trackDuration % 60;
                snprintf(timeStr, sizeof(timeStr), "%d:%02d / %d:%02d", pM, pS, dM, dS);
                tft.drawString(timeStr, 8, 64);

                // Track browsing hint
                tft.setTextColor(COLOR_AMBER, COLOR_DEEP_BG);
                tft.drawString("[DETENT: TRACK]", w - 88, 64);

                lastPos = g_deviceState.trackPosition;
            }

            // Animated Visualizer
            drawAudioVisualizer(8, 75, w - 16, 12, g_deviceState.isPlaying);
            break;
        }

        // ============================================
        // C. FM RADIO MODE (Frequency, Channel Name, Tuning Override)
        // ============================================
        case SOURCE_FM_RADIO: {
            bool tuneActive = isFMTuneWindowActive();
            if (g_deviceState.fmFrequency != lastFMFreq || tuneActive != lastFMTuneState) {
                tft.fillRect(4, 20, w - 8, 66, COLOR_DEEP_BG);

                // Large Frequency Readout
                tft.setTextSize(3);
                tft.setTextColor(tuneActive ? COLOR_PRIMARY_CYAN : COLOR_NEON_PINK, COLOR_DEEP_BG);
                char freqStr[16];
                snprintf(freqStr, sizeof(freqStr), "%.1f", g_deviceState.fmFrequency);
                tft.drawString(freqStr, 10, 24);

                tft.setTextSize(1);
                tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);
                tft.drawString("MHz FM", 96, 34);

                // Channel Name
                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                tft.drawString(g_deviceState.fmStationName, 10, 48);

                // Tuning State Indicator (5-second override window)
                if (tuneActive) {
                    tft.fillRoundRect(86, 46, 68, 12, 3, COLOR_PRIMARY_CYAN);
                    tft.setTextColor(COLOR_DEEP_BG, COLOR_PRIMARY_CYAN);
                    tft.drawString("TUNE (5s)", 94, 48);
                } else {
                    tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);
                    tft.drawString("VOL MODE", 96, 48);
                }

                // Tuning Dial Scale
                int scaleY = 62;
                tft.drawFastHLine(10, scaleY, w - 20, COLOR_BORDER);
                for (int x = 10; x <= w - 10; x += 14) {
                    tft.drawFastVLine(x, scaleY - 3, 4, COLOR_BORDER);
                }

                // Tuning Pointer Indicator
                float frac = (g_deviceState.fmFrequency - FM_MIN_FREQUENCY) / (FM_MAX_FREQUENCY - FM_MIN_FREQUENCY);
                frac = constrain(frac, 0.0f, 1.0f);
                int pointerX = 10 + (int)(frac * (w - 20));
                tft.fillTriangle(pointerX, scaleY + 1, pointerX - 4, scaleY + 7, pointerX + 4, scaleY + 7,
                                 tuneActive ? COLOR_PRIMARY_CYAN : COLOR_NEON_PINK);

                // Signal Strength Meter
                tft.setTextSize(1);
                tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);
                tft.drawString("SIG", 10, 75);
                drawModernProgressBar(34, 77, w - 46, 4, g_deviceState.fmSignalStrength / 100.0f, COLOR_NEON_PINK, COLOR_BORDER);

                lastFMFreq = g_deviceState.fmFrequency;
                lastFMTuneState = tuneActive;
            }
            break;
        }

        // ============================================
        // D. WEB FM / STREAMING MODE
        // ============================================
        case SOURCE_WEB_STREAM:
        default: {
            if (strcmp(g_deviceState.trackTitle, lastTitle) != 0 || needFullRedraw) {
                tft.fillRect(4, 20, w - 8, 26, COLOR_DEEP_BG);

                tft.setTextSize(1);
                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                String title = (strlen(g_deviceState.trackTitle) > 0)
                               ? String(g_deviceState.trackTitle) : "Web FM Digital Stream";
                if (title.length() > 22) title = title.substring(0, 20) + "..";
                tft.drawString(title, 8, 22);

                tft.setTextColor(COLOR_NEON_GREEN, COLOR_DEEP_BG);
                tft.drawString("Web Stream (44.1kHz / 16-bit)", 8, 34);

                strncpy(lastTitle, g_deviceState.trackTitle, MAX_METADATA_LENGTH - 1);
            }

            drawAudioVisualizer(8, 48, w - 16, 34, g_deviceState.isPlaying);
            break;
        }
    }

    // --------------------------------------------
    // BOTTOM CONTROLS & FOOTER (Volume & Uptime)
    // --------------------------------------------
    if (g_deviceState.volume != lastVolume) {
        tft.fillRect(4, 90, w - 8, 14, COLOR_DEEP_BG);

        tft.setTextSize(1);
        tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
        char volStr[16];
        snprintf(volStr, sizeof(volStr), "VOL %d%%", g_deviceState.volume);
        tft.drawString(volStr, 8, 92);

        drawModernProgressBar(56, 93, w - 64, 6, g_deviceState.volume / 100.0f, COLOR_PRIMARY_CYAN, COLOR_BORDER);
        lastVolume = g_deviceState.volume;
    }

    // Footer Info Bar
    tft.fillRect(4, h - 15, w - 8, 11, COLOR_PANEL_BG);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_MUTED_GRAY, COLOR_PANEL_BG);

    if (g_deviceState.localIP[0] != '\0') {
        tft.drawString(g_deviceState.localIP, 6, h - 14);
    } else {
        tft.drawString("Croenix Audios", 6, h - 14);
    }

    char upStr[16];
    int upM = (g_deviceState.uptime % 3600) / 60;
    int upS = g_deviceState.uptime % 60;
    snprintf(upStr, sizeof(upStr), "%02d:%02d", upM, upS);
    tft.drawString(upStr, w - 36, h - 14);

    xSemaphoreGive(g_stateMutex);
}

// -------------------------------------------------------------
// Graphical Helper Routines
// -------------------------------------------------------------

void drawModernProgressBar(int x, int y, int width, int height, float progress, uint16_t color, uint16_t bgColor) {
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    int fillW = (int)(width * progress);

    tft.fillRoundRect(x, y, width, height, height / 2, bgColor);
    if (fillW >= 2) {
        tft.fillRoundRect(x, y, fillW, height, height / 2, color);
    }
}

void drawAudioVisualizer(int x, int y, int width, int height, bool isPlaying) {
    int barSpacing = width / VIZ_BARS;
    int barWidth = barSpacing - 2;
    if (barWidth < 2) barWidth = 2;

    for (int i = 0; i < VIZ_BARS; i++) {
        if (isPlaying) {
            int target = random(3, height);
            vizLevels[i] = (vizLevels[i] * 3 + target) / 4;
            if (vizLevels[i] > vizPeaks[i]) {
                vizPeaks[i] = vizLevels[i];
            } else if (vizPeaks[i] > 1) {
                vizPeaks[i]--;
            }
        } else {
            if (vizLevels[i] > 0) vizLevels[i]--;
            if (vizPeaks[i] > 0) vizPeaks[i]--;
        }

        int barX = x + (i * barSpacing);
        int barH = vizLevels[i];
        int clearH = height - barH;

        if (clearH > 0) {
            tft.fillRect(barX, y, barWidth, clearH, COLOR_DEEP_BG);
        }

        if (barH > 0) {
            uint16_t barCol = (i < 4) ? COLOR_PRIMARY_CYAN : ((i < 8) ? COLOR_NEON_PURPLE : COLOR_NEON_PINK);
            tft.fillRect(barX, y + clearH, barWidth, barH, barCol);
        }

        if (vizPeaks[i] > 1 && vizPeaks[i] < height) {
            tft.drawFastHLine(barX, y + height - vizPeaks[i], barWidth, COLOR_WHITE);
        }
    }
}

void drawVolumeHUD(uint8_t volume) {
    int w = tft.width();
    int h = tft.height();

    int cardW = 120;
    int cardH = 68;
    int cardX = (w - cardW) / 2;
    int cardY = (h - cardH) / 2;

    tft.fillRoundRect(cardX, cardY, cardW, cardH, 8, COLOR_PANEL_BG);
    tft.drawRoundRect(cardX, cardY, cardW, cardH, 8, COLOR_PRIMARY_CYAN);
    tft.drawRoundRect(cardX + 1, cardY + 1, cardW - 2, cardH - 2, 7, COLOR_NEON_PURPLE);

    tft.setTextSize(1);
    tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_PANEL_BG);
    tft.drawString("VOLUME LEVEL", cardX + 22, cardY + 10);

    tft.setTextSize(3);
    tft.setTextColor(COLOR_WHITE, COLOR_PANEL_BG);
    char volStr[8];
    snprintf(volStr, sizeof(volStr), "%d%%", volume);
    int numX = (volume >= 100) ? (cardX + 26) : ((volume >= 10) ? (cardX + 34) : (cardX + 44));
    tft.drawString(volStr, numX, cardY + 24);

    drawModernProgressBar(cardX + 12, cardY + 52, cardW - 24, 6, volume / 100.0f, COLOR_PRIMARY_CYAN, COLOR_BORDER);
}
