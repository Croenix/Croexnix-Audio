#include "display_manager.h"
#include "device_state.h"
#include "encoder_manager.h"
#include "fm_radio.h"
#include "sd_manager.h"

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
static float lastFMFreq = -1.0;
static char lastTitle[MAX_METADATA_LENGTH] = "";
static BluetoothState lastBTState = BT_STATE_DISCONNECTED;
static uint8_t lastDiagModule = 255;
static unsigned long lastBlinkToggle = 0;
static bool blinkRedState = false;
static bool blinkBlueState = false;

// Spectrum Visualizer levels
static const int VIZ_BARS = 12;
static uint8_t vizLevels[VIZ_BARS] = {0};
static uint8_t vizPeaks[VIZ_BARS] = {0};

// Color Palette (RGB565)
#define COLOR_DEEP_BG       0x0000  // Pure Black
#define COLOR_PANEL_BG      0x1084  // Deep Slate Panel
#define COLOR_BORDER        0x2128  // Border Gray
#define COLOR_PRIMARY_CYAN  0x07FF  // Electric Cyan
#define COLOR_NEON_PURPLE   0x999F  // Neon Purple
#define COLOR_NEON_PINK     0xF819  // Neon Magenta
#define COLOR_NEON_GREEN    0x07E0  // Electric Green
#define COLOR_AMBER         0xFDA0  // Amber
#define COLOR_WHITE         0xFFFF  // Pure White
#define COLOR_MUTED_GRAY    0x8410  // Soft Gray
#define COLOR_RED_ALERT     0xF800  // Warning Red
#define COLOR_BLUE_ACTIVE   0x001F  // Solid Blue for BT
#define COLOR_BLUE_LIGHT    0x3BFF  // Bright Cyan Blue

void initDisplayManager() {
    DEBUG_PRINTLN("[DISPLAY] Initializing 1.8\" ST7735 TFT display...");

    tft.init();
    tft.setRotation(TFT_ROTATION); // 1 = Landscape (160x128)

    // Requirement: Upon boot, initialize the display with a pure black background before displaying "CROENIX AUDIOS"
    tft.fillScreen(COLOR_DEEP_BG);
    delay(150); // Ensure black background stabilizes

    displayInitialized = true;

    // Display "CROENIX AUDIOS" boot sequence
    showSplashScreen();

    needFullRedraw = true;
    currentScreen = UI_SCREEN_HOME;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_deviceState.activeScreen = UI_SCREEN_HOME;
        xSemaphoreGive(g_stateMutex);
    }
}

void showSplashScreen() {
    tft.fillScreen(COLOR_DEEP_BG);

    int w = tft.width();
    int h = tft.height();

    // Accent lines
    tft.drawFastHLine(0, 0, w, COLOR_PRIMARY_CYAN);
    tft.drawFastHLine(0, 1, w, COLOR_NEON_PURPLE);
    tft.drawFastHLine(0, h - 2, w, COLOR_NEON_PURPLE);
    tft.drawFastHLine(0, h - 1, w, COLOR_PRIMARY_CYAN);

    // Center Logo Box
    int centerX = w / 2;
    int logoY = 22;

    tft.drawRoundRect(centerX - 68, logoY - 12, 136, 44, 6, COLOR_BORDER);
    tft.drawRoundRect(centerX - 66, logoY - 10, 132, 40, 5, COLOR_PRIMARY_CYAN);

    // Boot Title: "CROENIX AUDIOS"
    tft.setTextSize(2);
    tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
    tft.drawString("CROENIX", centerX - 42, logoY - 4);

    tft.setTextSize(1);
    tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
    tft.drawString("AUDIOS", centerX - 18, logoY + 16);

    // Subtitle
    tft.setTextColor(COLOR_NEON_PURPLE, COLOR_DEEP_BG);
    tft.drawString("HI-FI DSP ENGINE", centerX - 46, logoY + 38);

    // Diagnostics sequence steps
    const char* bootSteps[] = {
        "INIT HARDWARE...",
        "WCMCU I2S DAC     [OK]",
        "HW-322 FM RADIO   [OK]",
        "SD SPI INTERFACE  [OK]",
        "ROTARY DETENT ISR [OK]",
        "SYSTEM READY!"
    };

    int barY = 82;
    int barW = w - 40;
    int barX = 20;

    tft.drawRoundRect(barX - 1, barY - 1, barW + 2, 8, 3, COLOR_BORDER);

    for (int step = 0; step < 6; step++) {
        tft.fillRect(10, barY + 14, w - 20, 12, COLOR_DEEP_BG);
        tft.setTextSize(1);
        tft.setTextColor((step == 5) ? COLOR_NEON_GREEN : COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
        tft.drawString(bootSteps[step], 14, barY + 14);

        float progress = (step + 1) / 6.0f;
        int fillW = (int)(barW * progress);
        if (fillW > 0) {
            uint16_t barCol = (step < 3) ? COLOR_PRIMARY_CYAN : ((step < 5) ? COLOR_NEON_PURPLE : COLOR_NEON_GREEN);
            tft.fillRoundRect(barX, barY, fillW, 6, 2, barCol);
        }
        delay(240);
    }

    delay(300);
}

void updateDisplay() {
    if (!displayInitialized) return;

    unsigned long now = millis();
    if (now - lastUpdate < DISPLAY_UPDATE_RATE) return;
    lastUpdate = now;

    // Check blink toggle for BT indicator (red 500ms, blue pairing 250ms)
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

    // Non-blocking state machine navigation
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
        case UI_SCREEN_HOME:
        default:
            showMainScreen();
            break;
    }
}

// ============================================
// 1. HARDWARE DIAGNOSTICS SCREEN (10s Long-Press)
// ============================================

void showHardwareDiagnosticsScreen() {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(15)) != pdTRUE) return;

    int w = tft.width();
    int h = tft.height();
    uint8_t mod = g_deviceState.selectedDiagModule;

    if (needFullRedraw || mod != lastDiagModule) {
        tft.fillScreen(COLOR_DEEP_BG);

        // Header Bar
        tft.fillRect(0, 0, w, 18, COLOR_PANEL_BG);
        tft.drawFastHLine(0, 18, w, COLOR_NEON_PURPLE);

        tft.setTextSize(1);
        tft.setTextColor(COLOR_AMBER, COLOR_PANEL_BG);
        tft.drawString("HW DIAGNOSTICS", 6, 5);

        // Module index indicator [1/4]
        char idxStr[12];
        snprintf(idxStr, sizeof(idxStr), "[%d/4]", mod + 1);
        tft.setTextColor(COLOR_WHITE, COLOR_PANEL_BG);
        tft.drawString(idxStr, w - 36, 5);

        // Module Content Card Box
        tft.drawRoundRect(4, 22, w - 8, 86, 4, COLOR_BORDER);
        tft.fillRect(5, 23, w - 10, 84, COLOR_DEEP_BG);

        // Render selected module details
        tft.setTextSize(1);

        switch (mod) {
            case 0: { // WCMCU DAC
                tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
                tft.drawString("MODULE 1: WCMCU DAC", 10, 26);
                tft.drawFastHLine(10, 37, w - 20, COLOR_BORDER);

                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                tft.drawString("Chip: PCM5102A (I2S)", 10, 42);
                tft.drawString("Pins: D27, BCK26, LRC25", 10, 53);
                tft.drawString("Stream: 44.1kHz 16-bit", 10, 64);

                char bufStr[28];
                snprintf(bufStr, sizeof(bufStr), "Underruns: %u [0 LAG]", g_deviceState.audioUnderruns);
                tft.drawString(bufStr, 10, 75);

                // Live status badge
                tft.fillRoundRect(10, 88, 70, 14, 3, COLOR_NEON_GREEN);
                tft.setTextColor(COLOR_DEEP_BG, COLOR_NEON_GREEN);
                tft.drawString("OPERATIONAL", 13, 91);
                break;
            }

            case 1: { // HW-322 FM Module
                tft.setTextColor(COLOR_NEON_PINK, COLOR_DEEP_BG);
                tft.drawString("MODULE 2: HW-322 FM", 10, 26);
                tft.drawFastHLine(10, 37, w - 20, COLOR_BORDER);

                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                tft.drawString("Chip: Si4703/TEA (I2C)", 10, 42);
                tft.drawString("Pins: SDA21, SCL22, RST16", 10, 53);

                char freqStr[28];
                snprintf(freqStr, sizeof(freqStr), "Freq: %.1f MHz", g_deviceState.fmFrequency);
                tft.drawString(freqStr, 10, 64);

                char rssiStr[28];
                snprintf(rssiStr, sizeof(rssiStr), "Signal: %d%% RSSI", g_deviceState.fmSignalStrength);
                tft.drawString(rssiStr, 10, 75);

                bool isOk = g_deviceState.fmOperational || checkFMIntegrity();
                uint16_t statusCol = isOk ? COLOR_NEON_GREEN : COLOR_AMBER;
                tft.fillRoundRect(10, 88, 70, 14, 3, statusCol);
                tft.setTextColor(COLOR_DEEP_BG, statusCol);
                tft.drawString(isOk ? "ONLINE OK" : "CHECK I2C", 16, 91);
                break;
            }

            case 2: { // SD Card Reader
                tft.setTextColor(COLOR_AMBER, COLOR_DEEP_BG);
                tft.drawString("MODULE 3: SD READER", 10, 26);
                tft.drawFastHLine(10, 37, w - 20, COLOR_BORDER);

                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                tft.drawString("Bus: SPI (CS15, CLK18)", 10, 42);
                tft.drawString("Lines: MOSI23, MISO19", 10, 53);

                bool mounted = g_deviceState.sdCardMounted;
                if (mounted) {
                    char sizeStr[28];
                    snprintf(sizeStr, sizeof(sizeStr), "Capacity: %llu MB", g_deviceState.sdCardSize / (1024 * 1024));
                    tft.drawString(sizeStr, 10, 64);
                    tft.drawString("FS Format: FAT32 OK", 10, 75);

                    tft.fillRoundRect(10, 88, 70, 14, 3, COLOR_NEON_GREEN);
                    tft.setTextColor(COLOR_DEEP_BG, COLOR_NEON_GREEN);
                    tft.drawString("MOUNTED", 20, 91);
                } else {
                    tft.drawString("Card: No card inserted", 10, 64);
                    tft.drawString("Status: Standby", 10, 75);

                    tft.fillRoundRect(10, 88, 70, 14, 3, COLOR_RED_ALERT);
                    tft.setTextColor(COLOR_WHITE, COLOR_RED_ALERT);
                    tft.drawString("NO CARD", 20, 91);
                }
                break;
            }

            case 3: { // System & Display
                tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
                tft.drawString("MODULE 4: ESP32 CORE", 10, 26);
                tft.drawFastHLine(10, 37, w - 20, COLOR_BORDER);

                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                char heapStr[28];
                snprintf(heapStr, sizeof(heapStr), "Free Heap: %u KB", g_deviceState.freeHeap / 1024);
                tft.drawString(heapStr, 10, 42);

                char cpuStr[28];
                snprintf(cpuStr, sizeof(cpuStr), "CPU: Core0:%d%% C1:%d%%", g_deviceState.cpuUsageCore0, g_deviceState.cpuUsageCore1);
                tft.drawString(cpuStr, 10, 53);

                char netStr[28];
                snprintf(netStr, sizeof(netStr), "Net: %s", g_deviceState.wifiConnected ? "STA Home" : (g_deviceState.apMode ? "Secure AP" : "Offline"));
                tft.drawString(netStr, 10, 64);

                tft.drawString("Display: ST7735 SPI OK", 10, 75);

                tft.fillRoundRect(10, 88, 70, 14, 3, COLOR_NEON_GREEN);
                tft.setTextColor(COLOR_DEEP_BG, COLOR_NEON_GREEN);
                tft.drawString("HEALTHY", 22, 91);
                break;
            }
        }

        // Bottom Navigation Footer
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

// ============================================
// 2. MAIN HOME SCREEN WITH DYNAMIC INPUT GRAPHICS
// ============================================

void showMainScreen() {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(15)) != pdTRUE) return;

    int w = tft.width();
    int h = tft.height();

    // Check if source changed or full redraw needed
    if (needFullRedraw || g_deviceState.currentSource != lastSource) {
        tft.fillScreen(COLOR_DEEP_BG);

        // Header Panel
        tft.fillRect(0, 0, w, 18, COLOR_PANEL_BG);
        tft.drawFastHLine(0, 18, w, COLOR_BORDER);

        // Bottom Footer Bar
        tft.fillRect(0, h - 18, w, 18, COLOR_PANEL_BG);
        tft.drawFastHLine(0, h - 19, w, COLOR_BORDER);

        needFullRedraw = false;
        lastSource = g_deviceState.currentSource;
        lastTitle[0] = '\0';
        lastFMFreq = -1.0;
        lastPos = 999999;
        lastVolume = -1;
        lastBTState = (BluetoothState)255;
    }

    // --------------------------------------------
    // TOP STATUS BAR (Pill Badge, Play Icon, Wi-Fi)
    // --------------------------------------------
    const char* srcText = "FM";
    uint16_t srcColor = COLOR_NEON_PINK;

    switch (g_deviceState.currentSource) {
        case SOURCE_FM_RADIO:    srcText = "FM";    srcColor = COLOR_NEON_PINK;    break;
        case SOURCE_WEB_STREAM:  srcText = "MUSIC"; srcColor = COLOR_NEON_GREEN;   break;
        case SOURCE_BLUETOOTH:   srcText = "BT";    srcColor = COLOR_PRIMARY_CYAN; break;
        case SOURCE_SD_CARD:     srcText = "SD";    srcColor = COLOR_AMBER;        break;
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

    // Wi-Fi Status on Right
    tft.fillRect(w - 60, 3, 58, 12, COLOR_PANEL_BG);
    tft.setTextSize(1);
    if (g_deviceState.wifiConnected) {
        tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_PANEL_BG);
        tft.drawString("WiFi:OK", w - 48, 5);
    } else if (g_deviceState.apMode) {
        tft.setTextColor(COLOR_AMBER, COLOR_PANEL_BG);
        tft.drawString("AP:Setup", w - 52, 5);
    } else {
        tft.setTextColor(COLOR_MUTED_GRAY, COLOR_PANEL_BG);
        tft.drawString("OFFLINE", w - 48, 5);
    }

    // --------------------------------------------
    // DYNAMIC ACTIVE INPUT GRAPHICS
    // --------------------------------------------
    switch (g_deviceState.currentSource) {

        // ============================================
        // A. FM RADIO MODE (Controls & Tuning Graphics)
        // ============================================
        case SOURCE_FM_RADIO: {
            if (g_deviceState.fmFrequency != lastFMFreq) {
                tft.fillRect(4, 20, w - 8, 66, COLOR_DEEP_BG);

                // Big Frequency Readout
                tft.setTextSize(3);
                tft.setTextColor(COLOR_NEON_PINK, COLOR_DEEP_BG);
                char freqStr[16];
                snprintf(freqStr, sizeof(freqStr), "%.1f", g_deviceState.fmFrequency);
                tft.drawString(freqStr, 10, 24);

                tft.setTextSize(1);
                tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);
                tft.drawString("MHz FM", 96, 34);

                // Graphical Tuner Scale Bar
                int scaleY = 52;
                tft.drawFastHLine(10, scaleY, w - 20, COLOR_BORDER);
                for (int x = 10; x <= w - 10; x += 14) {
                    tft.drawFastVLine(x, scaleY - 3, 4, COLOR_BORDER);
                }

                // Tuning Pointer Indicator
                float frac = (g_deviceState.fmFrequency - FM_MIN_FREQUENCY) / (FM_MAX_FREQUENCY - FM_MIN_FREQUENCY);
                frac = constrain(frac, 0.0f, 1.0f);
                int pointerX = 10 + (int)(frac * (w - 20));
                tft.fillTriangle(pointerX, scaleY + 1, pointerX - 4, scaleY + 7, pointerX + 4, scaleY + 7, COLOR_NEON_PINK);

                // Control status: Step Tuning / Seek indicator
                tft.setTextSize(1);
                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                tft.drawString("<< 0.1 MHz DETENT >>", 14, 64);

                // Signal Strength Meter
                tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);
                tft.drawString("SIG", 10, 76);
                drawModernProgressBar(34, 78, w - 46, 4, g_deviceState.fmSignalStrength / 100.0f, COLOR_NEON_PINK, COLOR_BORDER);

                lastFMFreq = g_deviceState.fmFrequency;
            }
            break;
        }

        // ============================================
        // B. BLUETOOTH MODE (Color-Coded Indicators)
        // ============================================
        case SOURCE_BLUETOOTH: {
            BluetoothState bt = g_deviceState.bluetoothState;
            if (bt != lastBTState || (bt != BT_STATE_CONNECTED)) {
                tft.fillRect(4, 20, w - 8, 66, COLOR_DEEP_BG);

                // Stylized Bluetooth Emblem
                int btCenterX = 30;
                int btCenterY = 50;

                // Color-coded Indicator logic:
                // - Blinking red for disconnected
                // - Blinking blue for pairing
                // - Solid blue for connected
                uint16_t statusColor;
                const char* statusLabel;

                if (bt == BT_STATE_CONNECTED) {
                    statusColor = COLOR_BLUE_LIGHT;
                    statusLabel = "CONNECTED";
                } else if (bt == BT_STATE_PAIRING) {
                    statusColor = blinkBlueState ? COLOR_BLUE_LIGHT : COLOR_DEEP_BG;
                    statusLabel = "PAIRING...";
                } else { // BT_STATE_DISCONNECTED
                    statusColor = blinkRedState ? COLOR_RED_ALERT : COLOR_DEEP_BG;
                    statusLabel = "DISCONNECTED";
                }

                // Radar rings
                tft.drawCircle(btCenterX, btCenterY, 18, (bt == BT_STATE_CONNECTED) ? COLOR_BLUE_LIGHT : COLOR_BORDER);
                tft.drawCircle(btCenterX, btCenterY, 12, statusColor);
                tft.fillCircle(btCenterX, btCenterY, 5, statusColor);

                // Bluetooth Icon Glyph
                tft.drawFastVLine(btCenterX, btCenterY - 9, 18, COLOR_WHITE);
                tft.drawLine(btCenterX, btCenterY - 9, btCenterX + 6, btCenterY - 4, COLOR_WHITE);
                tft.drawLine(btCenterX + 6, btCenterY - 4, btCenterX - 6, btCenterY + 4, COLOR_WHITE);
                tft.drawLine(btCenterX - 6, btCenterY - 4, btCenterX + 6, btCenterY + 4, COLOR_WHITE);
                tft.drawLine(btCenterX + 6, btCenterY + 4, btCenterX, btCenterY + 9, COLOR_WHITE);

                // Status Text
                tft.setTextSize(1);
                tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);
                tft.drawString("BLUETOOTH AUDIO", 58, 28);

                tft.setTextSize(1);
                tft.setTextColor(statusColor != COLOR_DEEP_BG ? statusColor : COLOR_MUTED_GRAY, COLOR_DEEP_BG);
                tft.drawString(statusLabel, 58, 42);

                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                tft.drawString(g_deviceState.bluetoothDevice, 58, 56);

                tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);
                tft.drawString("A2DP Sink 44.1kHz", 58, 70);

                lastBTState = bt;
            }
            break;
        }

        // ============================================
        // C. SD CARD PLAYBACK (ID3 Tags & Progress)
        // ============================================
        case SOURCE_SD_CARD: {
            // ID3 Tags & Track Name
            if (strcmp(g_deviceState.trackTitle, lastTitle) != 0 || needFullRedraw) {
                tft.fillRect(4, 20, w - 8, 32, COLOR_DEEP_BG);

                tft.setTextSize(1);
                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);

                // Title Tag
                String title = (strlen(g_deviceState.trackTitle) > 0)
                               ? String(g_deviceState.trackTitle)
                               : (g_deviceState.sdCardMounted ? "Track Playback" : "No SD Card");
                if (title.length() > 22) title = title.substring(0, 20) + "..";
                tft.drawString(title, 8, 22);

                // Artist Tag
                tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
                String artist = (strlen(g_deviceState.trackArtist) > 0)
                                ? String(g_deviceState.trackArtist) : "ID3: Croenix SD";
                if (artist.length() > 24) artist = artist.substring(0, 22) + "..";
                tft.drawString(artist, 8, 33);

                // Album Tag & Folder Indicator
                tft.setTextColor(COLOR_AMBER, COLOR_DEEP_BG);
                String album = (strlen(g_deviceState.trackAlbum) > 0)
                               ? String(g_deviceState.trackAlbum) : g_deviceState.currentFolder;
                if (album.length() > 24) album = album.substring(0, 22) + "..";
                tft.drawString(album, 8, 44);

                strncpy(lastTitle, g_deviceState.trackTitle, MAX_METADATA_LENGTH - 1);
            }

            // Track Progress Bar & Time Elapsed / Duration
            if (g_deviceState.trackPosition != lastPos || g_deviceState.trackDuration > 0) {
                float progress = (g_deviceState.trackDuration > 0)
                                 ? (float)g_deviceState.trackPosition / (float)g_deviceState.trackDuration
                                 : 0.0f;

                drawModernProgressBar(8, 57, w - 16, 4, progress, COLOR_AMBER, COLOR_BORDER);

                // Time digits: MM:SS / MM:SS
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

                // Track browsing detent hint
                tft.setTextColor(COLOR_AMBER, COLOR_DEEP_BG);
                tft.drawString("[DETENT: NEXT/PREV]", w - 90, 64);

                lastPos = g_deviceState.trackPosition;
            }

            // Small animated visualizer
            drawAudioVisualizer(8, 75, w - 16, 12, g_deviceState.isPlaying);
            break;
        }

        // ============================================
        // D. MUSIC PLAYER (Web Stream / Network)
        // ============================================
        case SOURCE_WEB_STREAM:
        default: {
            if (strcmp(g_deviceState.trackTitle, lastTitle) != 0 || needFullRedraw) {
                tft.fillRect(4, 20, w - 8, 26, COLOR_DEEP_BG);

                tft.setTextSize(1);
                tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
                String title = (strlen(g_deviceState.trackTitle) > 0)
                               ? String(g_deviceState.trackTitle) : "Croenix Stream";
                if (title.length() > 22) title = title.substring(0, 20) + "..";
                tft.drawString(title, 8, 22);

                tft.setTextColor(COLOR_NEON_GREEN, COLOR_DEEP_BG);
                tft.drawString("Stream Player (44.1kHz / 16-bit)", 8, 34);

                strncpy(lastTitle, g_deviceState.trackTitle, MAX_METADATA_LENGTH - 1);
            }

            // Audio Visualizer
            drawAudioVisualizer(8, 50, w - 16, 32, g_deviceState.isPlaying);
            break;
        }
    }

    // --------------------------------------------
    // BOTTOM CONTROL & FOOTER (Volume & Device Info)
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

    // Live Footer Bar
    tft.fillRect(4, h - 15, w - 8, 11, COLOR_PANEL_BG);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_MUTED_GRAY, COLOR_PANEL_BG);

    if (g_deviceState.localIP[0] != '\0') {
        tft.drawString(g_deviceState.localIP, 6, h - 14);
    } else {
        tft.drawString("Croenix Audios", 6, h - 14);
    }

    // Live Uptime
    char upStr[16];
    int upM = (g_deviceState.uptime % 3600) / 60;
    int upS = g_deviceState.uptime % 60;
    snprintf(upStr, sizeof(upStr), "%02d:%02d", upM, upS);
    tft.drawString(upStr, w - 36, h - 14);

    xSemaphoreGive(g_stateMutex);
}

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
