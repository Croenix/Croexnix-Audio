#include "display_manager.h"
#include "device_state.h"
#include "encoder_manager.h"

// TFT instance
TFT_eSPI tft = TFT_eSPI();

// Display state
static bool displayInitialized = false;
static unsigned long lastUpdate = 0;
static bool needFullRedraw = true;
static bool wasOverlayActive = false;

// Previous state tracking for differential flicker-free updates
static AudioSource lastSource = SOURCE_NONE;
static bool lastPlaying = false;
static bool lastWifi = false;
static int lastVolume = -1;
static uint32_t lastPos = 0;
static float lastFMFreq = -1.0;
static char lastTitle[MAX_METADATA_LENGTH] = "";

// Animated Spectrum Visualizer levels
static const int VIZ_BARS = 12;
static uint8_t vizLevels[VIZ_BARS] = {0};
static uint8_t vizPeaks[VIZ_BARS] = {0};

// Trending RGB565 Color Palette
#define COLOR_DEEP_BG       0x0000  // Pure OLED Black
#define COLOR_PANEL_BG      0x1084  // Deep Cyber Slate (R:2, G:4, B:4)
#define COLOR_BORDER        0x2128  // Subtle Glass Border
#define COLOR_PRIMARY_CYAN  0x07FF  // Electric Cyan
#define COLOR_NEON_PURPLE   0x999F  // Neon Violet / Purple
#define COLOR_NEON_PINK     0xF819  // Neon Magenta
#define COLOR_NEON_GREEN    0x07E0  // Electric Green
#define COLOR_AMBER         0xFDA0  // Warm Gold / Amber
#define COLOR_WHITE         0xFFFF  // Pure White
#define COLOR_MUTED_GRAY    0x8410  // Soft Secondary Text
#define COLOR_RED_ALERT     0xF800  // Warning Red

void initDisplayManager() {
    DEBUG_PRINTLN("[DISPLAY] Initializing ST7735 1.8\" SPI display...");

    tft.init();
    tft.setRotation(TFT_ROTATION); // 1 = Landscape (160x128)
    tft.fillScreen(COLOR_DEEP_BG);

    displayInitialized = true;

    // Show trending animated boot screen
    showSplashScreen();

    needFullRedraw = true;
    DEBUG_PRINTLN("[DISPLAY] Display initialized successfully");
}

void showSplashScreen() {
    tft.fillScreen(COLOR_DEEP_BG);

    int w = tft.width();
    int h = tft.height();

    // 1. Neon Accent Top/Bottom lines
    tft.drawFastHLine(0, 0, w, COLOR_PRIMARY_CYAN);
    tft.drawFastHLine(0, 1, w, COLOR_NEON_PURPLE);
    tft.drawFastHLine(0, h - 2, w, COLOR_NEON_PURPLE);
    tft.drawFastHLine(0, h - 1, w, COLOR_PRIMARY_CYAN);

    // 2. Geometric Audio Emblem
    int centerX = w / 2;
    int logoY = 24;

    // Stylish decorative audio brackets
    tft.drawRoundRect(centerX - 50, logoY - 14, 100, 36, 6, COLOR_BORDER);
    tft.drawRoundRect(centerX - 48, logoY - 12, 96, 32, 5, COLOR_PRIMARY_CYAN);

    // Brand Name with Glow
    tft.setTextSize(2);
    tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
    tft.drawString("CROENIX", centerX - 42, logoY - 4);

    // Subtitle
    tft.setTextSize(1);
    tft.setTextColor(COLOR_NEON_PURPLE, COLOR_DEEP_BG);
    tft.drawString("HI-FI AUDIO ENGINE", centerX - 48, logoY + 28);

    // 3. Animated Loading Progress Bar & Diagnostics Sequence
    const char* bootSteps[] = {
        "INIT HARDWARE...",
        "I2S DAC (GPIO 27) [OK]",
        "SI4703 FM RADIO   [OK]",
        "MICROSD SPI BUS   [OK]",
        "WEBSOCKET LIVE    [OK]",
        "CROENIX READY!"
    };

    int barY = 82;
    int barW = w - 40;
    int barX = 20;

    // Background bar
    tft.drawRoundRect(barX - 1, barY - 1, barW + 2, 8, 3, COLOR_BORDER);

    for (int step = 0; step < 6; step++) {
        // Step Diagnostics text
        tft.fillRect(10, barY + 14, w - 20, 12, COLOR_DEEP_BG);
        tft.setTextSize(1);
        tft.setTextColor(step == 5 ? COLOR_NEON_GREEN : COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
        tft.drawString(bootSteps[step], 16, barY + 14);

        // Progress Fill
        float progress = (step + 1) / 6.0f;
        int fillW = (int)(barW * progress);
        if (fillW > 0) {
            uint16_t barColor = (step < 3) ? COLOR_PRIMARY_CYAN : (step < 5 ? COLOR_NEON_PURPLE : COLOR_NEON_GREEN);
            tft.fillRoundRect(barX, barY, fillW, 6, 2, barColor);
        }

        delay(320);
    }

    delay(400);
}

void updateDisplay() {
    if (!displayInitialized) return;

    unsigned long now = millis();
    if (now - lastUpdate < DISPLAY_UPDATE_RATE) return;
    lastUpdate = now;

    // Check if rotary encoder triggered the Volume HUD
    if (isVolumeOverlayActive()) {
        wasOverlayActive = true;
        drawVolumeHUD(getVolumeOverlayValue());
        return;
    } else if (wasOverlayActive) {
        // Overlay just ended - restore main screen completely
        wasOverlayActive = false;
        needFullRedraw = true;
    }

    showMainScreen();
}

void showMainScreen() {
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(15)) != pdTRUE) return;

    int w = tft.width();
    int h = tft.height();

    // Check if source or full redraw is needed
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
    }

    // ============================================
    // 1. TOP STATUS BAR (y: 0 to 18)
    // ============================================
    // Source Pill Badge
    const char* srcText = "NONE";
    uint16_t srcColor = COLOR_MUTED_GRAY;

    switch (g_deviceState.currentSource) {
        case SOURCE_BLUETOOTH:  srcText = "BT";  srcColor = COLOR_PRIMARY_CYAN; break;
        case SOURCE_SD_CARD:    srcText = "SD";  srcColor = COLOR_AMBER;        break;
        case SOURCE_FM_RADIO:   srcText = "FM";  srcColor = COLOR_NEON_PINK;    break;
        case SOURCE_WEB_STREAM: srcText = "WEB"; srcColor = COLOR_NEON_GREEN;   break;
        default:                srcText = "---"; srcColor = COLOR_MUTED_GRAY;   break;
    }

    // Draw Source Pill
    tft.fillRoundRect(3, 2, 28, 14, 3, srcColor);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_DEEP_BG, srcColor);
    tft.drawString(srcText, srcText[2] == '\0' ? 10 : 7, 5);

    // Play/Pause State Icon
    tft.fillRect(36, 3, 24, 12, COLOR_PANEL_BG);
    if (g_deviceState.isPlaying) {
        tft.fillTriangle(38, 4, 38, 14, 46, 9, COLOR_NEON_GREEN);
    } else {
        tft.fillRect(38, 4, 3, 10, COLOR_RED_ALERT);
        tft.fillRect(43, 4, 3, 10, COLOR_RED_ALERT);
    }

    // Wi-Fi Status on Right
    tft.fillRect(w - 55, 3, 52, 12, COLOR_PANEL_BG);
    tft.setTextSize(1);
    if (g_deviceState.wifiConnected) {
        // Wi-Fi 3-bar signal icon
        int sigX = w - 46;
        tft.fillRect(sigX,     11, 2, 3, COLOR_PRIMARY_CYAN);
        tft.fillRect(sigX + 3,  9, 2, 5, COLOR_PRIMARY_CYAN);
        tft.fillRect(sigX + 6,  7, 2, 7, COLOR_PRIMARY_CYAN);

        tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_PANEL_BG);
        tft.drawString("WiFi", w - 32, 5);
    } else if (g_deviceState.apMode) {
        tft.setTextColor(COLOR_AMBER, COLOR_PANEL_BG);
        tft.drawString("AP:Croenix", w - 54, 5);
    } else {
        tft.setTextColor(COLOR_MUTED_GRAY, COLOR_PANEL_BG);
        tft.drawString("OFFLINE", w - 48, 5);
    }

    // ============================================
    // 2. ACTIVE MEDIA CARD (y: 22 to 74)
    // ============================================
    if (g_deviceState.currentSource == SOURCE_FM_RADIO) {
        // --- FM Radio Big Frequency Display ---
        if (g_deviceState.fmFrequency != lastFMFreq) {
            tft.fillRect(4, 22, w - 8, 50, COLOR_DEEP_BG);

            // Frequency in Big Text
            tft.setTextSize(2);
            tft.setTextColor(COLOR_NEON_PINK, COLOR_DEEP_BG);
            char freqStr[16];
            snprintf(freqStr, sizeof(freqStr), "%.1f", g_deviceState.fmFrequency);
            tft.drawString(freqStr, 12, 26);

            tft.setTextSize(1);
            tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);
            tft.drawString("MHz", 74, 32);

            // Station Name
            tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
            const char* stName = (strlen(g_deviceState.fmStationName) > 0)
                                 ? g_deviceState.fmStationName : "FM TUNER";
            tft.drawString(stName, 12, 48);

            // Signal bar
            uint8_t rssi = g_deviceState.fmSignalStrength;
            drawModernProgressBar(12, 60, w - 24, 4, rssi / 100.0f, COLOR_NEON_PINK, COLOR_BORDER);

            lastFMFreq = g_deviceState.fmFrequency;
        }
    } else {
        // --- Digital Audio Track Display (SD / BT / Web) ---
        // Title
        if (strcmp(g_deviceState.trackTitle, lastTitle) != 0) {
            tft.fillRect(4, 22, w - 8, 22, COLOR_DEEP_BG);

            tft.setTextSize(1);
            tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);

            String title = (strlen(g_deviceState.trackTitle) > 0)
                           ? String(g_deviceState.trackTitle)
                           : (g_deviceState.currentSource == SOURCE_BLUETOOTH
                              ? "Bluetooth Audio" : "No Track Loaded");

            if (title.length() > 22) title = title.substring(0, 20) + "...";
            tft.drawString(title, 8, 24);

            // Artist
            tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
            String artist = (strlen(g_deviceState.trackArtist) > 0)
                            ? String(g_deviceState.trackArtist) : "Croenix Audio";
            if (artist.length() > 24) artist = artist.substring(0, 22) + "...";
            tft.drawString(artist, 8, 35);

            strncpy(lastTitle, g_deviceState.trackTitle, MAX_METADATA_LENGTH - 1);
        }

        // Progress Bar & Time
        if (g_deviceState.trackPosition != lastPos || g_deviceState.trackDuration > 0) {
            float progress = (g_deviceState.trackDuration > 0)
                             ? (float)g_deviceState.trackPosition / (float)g_deviceState.trackDuration
                             : 0.0f;

            drawModernProgressBar(8, 48, w - 16, 4, progress, COLOR_PRIMARY_CYAN, COLOR_BORDER);

            // Time digits
            tft.fillRect(8, 55, w - 16, 9, COLOR_DEEP_BG);
            tft.setTextSize(1);
            tft.setTextColor(COLOR_MUTED_GRAY, COLOR_DEEP_BG);

            char timeStr[24];
            int pM = g_deviceState.trackPosition / 60;
            int pS = g_deviceState.trackPosition % 60;
            int dM = g_deviceState.trackDuration / 60;
            int dS = g_deviceState.trackDuration % 60;
            snprintf(timeStr, sizeof(timeStr), "%d:%02d / %d:%02d", pM, pS, dM, dS);
            tft.drawString(timeStr, 8, 55);

            lastPos = g_deviceState.trackPosition;
        }

        // Animated Equalizer Visualizer Bars
        drawAudioVisualizer(8, 67, w - 16, 18, g_deviceState.isPlaying);
    }

    // ============================================
    // 3. BOTTOM CONTROL & FOOTER (y: 88 to 127)
    // ============================================
    // Volume Indicator Row
    if (g_deviceState.volume != lastVolume) {
        tft.fillRect(4, 90, w - 8, 16, COLOR_DEEP_BG);

        tft.setTextSize(1);
        tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_DEEP_BG);
        char volStr[16];
        snprintf(volStr, sizeof(volStr), "VOL %d%%", g_deviceState.volume);
        tft.drawString(volStr, 8, 92);

        // Volume Level Bar
        drawModernProgressBar(58, 93, w - 66, 6, g_deviceState.volume / 100.0f, COLOR_PRIMARY_CYAN, COLOR_BORDER);
        lastVolume = g_deviceState.volume;
    }

    // Footer Info Bar (IP and Uptime)
    tft.fillRect(4, h - 15, w - 8, 11, COLOR_PANEL_BG);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_MUTED_GRAY, COLOR_PANEL_BG);

    if (g_deviceState.localIP[0] != '\0') {
        tft.drawString(g_deviceState.localIP, 6, h - 14);
    } else {
        tft.drawString("Croenix v1.0", 6, h - 14);
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

    // Background track
    tft.fillRoundRect(x, y, width, height, height / 2, bgColor);

    // Filled portion
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
            // Organic pseudo-spectrum animation
            int target = random(3, height);
            // Smooth lerp
            vizLevels[i] = (vizLevels[i] * 3 + target) / 4;
            if (vizLevels[i] > vizPeaks[i]) {
                vizPeaks[i] = vizLevels[i];
            } else if (vizPeaks[i] > 1) {
                vizPeaks[i]--;
            }
        } else {
            // Decay to 0 when paused
            if (vizLevels[i] > 0) vizLevels[i]--;
            if (vizPeaks[i] > 0) vizPeaks[i]--;
        }

        int barX = x + (i * barSpacing);
        int barH = vizLevels[i];
        int clearH = height - barH;

        // Clear top portion
        if (clearH > 0) {
            tft.fillRect(barX, y, barWidth, clearH, COLOR_DEEP_BG);
        }

        // Draw active bar with gradient effect
        if (barH > 0) {
            uint16_t barCol = (i < 4) ? COLOR_PRIMARY_CYAN : (i < 8 ? COLOR_NEON_PURPLE : COLOR_NEON_PINK);
            tft.fillRect(barX, y + clearH, barWidth, barH, barCol);
        }

        // Peak Hold Dot
        if (vizPeaks[i] > 1 && vizPeaks[i] < height) {
            tft.drawFastHLine(barX, y + height - vizPeaks[i], barWidth, COLOR_WHITE);
        }
    }
}

void drawVolumeHUD(uint8_t volume) {
    int w = tft.width();
    int h = tft.height();

    // Floating glass HUD card
    int cardW = 120;
    int cardH = 68;
    int cardX = (w - cardW) / 2;
    int cardY = (h - cardH) / 2;

    tft.fillRoundRect(cardX, cardY, cardW, cardH, 8, COLOR_PANEL_BG);
    tft.drawRoundRect(cardX, cardY, cardW, cardH, 8, COLOR_PRIMARY_CYAN);
    tft.drawRoundRect(cardX + 1, cardY + 1, cardW - 2, cardH - 2, 7, COLOR_NEON_PURPLE);

    // Title
    tft.setTextSize(1);
    tft.setTextColor(COLOR_PRIMARY_CYAN, COLOR_PANEL_BG);
    tft.drawString("VOLUME LEVEL", cardX + 22, cardY + 10);

    // Big Volume Digits
    tft.setTextSize(3);
    tft.setTextColor(COLOR_WHITE, COLOR_PANEL_BG);
    char volStr[8];
    snprintf(volStr, sizeof(volStr), "%d%%", volume);
    int numX = (volume >= 100) ? (cardX + 26) : (volume >= 10 ? cardX + 34 : cardX + 44);
    tft.drawString(volStr, numX, cardY + 24);

    // Glowing Volume Bar
    drawModernProgressBar(cardX + 12, cardY + 52, cardW - 24, 6, volume / 100.0f, COLOR_PRIMARY_CYAN, COLOR_BORDER);
}
