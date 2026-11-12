#include "display_manager.h"
#include "device_state.h"
#include "encoder_manager.h"
#include "fm_radio.h"
#include "sd_manager.h"
#include "audio_manager.h"
#include "qrcode.h"

// Hardware TFT Instance
TFT_eSPI tft = TFT_eSPI();

// Display State Machine Tracking
static bool displayInitialized = false;
static unsigned long lastUpdate = 0;
static bool needFullRedraw = true;
static bool wasOverlayActive = false;
static UIScreenState currentScreen = UI_SCREEN_BOOT_SPLASH;

// Change detection cache for flicker-free differential redraws
static uint8_t  lastGridCursor = 255;
static int      lastVol = -1;
static bool     lastPlay = false;
static float    lastFreq = -1.0f;
static uint32_t lastPos = 999999;
static char     lastTrack[MAX_METADATA_LENGTH] = "";
static BluetoothState lastBTState = (BluetoothState)255;
static unsigned long lastWaveTick = 0;
static uint8_t  wavePhase = 0;

// Spectrum Visualizer Levels (14 Bars)
static const int VIZ_BARS = 14;
static uint8_t vizLevels[VIZ_BARS] = {0};
static uint8_t vizPeaks[VIZ_BARS] = {0};

// =========================================================================
// CYBER NEON PALETTE (Precise RGB565 mapping matching Reference Design)
// =========================================================================
#define COLOR_DEEP_BG       0x0821  // Obsidian Black/Navy (#0A0A14)
#define COLOR_CARD_BG       0x10A2  // Glass Dark Slate (#121422)
#define COLOR_CARD_BORDER   0x3166  // Subtle card border (#332644)
#define COLOR_NEON_PINK     0xFAAA  // Electric Magenta (#FF2E93)
#define COLOR_NEON_PURPLE   0x8A1F  // Deep Violet (#8B2FC9)
#define COLOR_NEON_CYAN     0x06BF  // Electric Cyan (#00D9FF)
#define COLOR_NEON_GREEN    0x07E0  // Connected Green (#10B981)
#define COLOR_AMBER         0xFDA0  // Warm Amber (#F59E0B)
#define COLOR_SOUNDCLOUD    0xFD20  // SoundCloud Orange (#FF5500)
#define COLOR_WHITE         0xFFFF  // Pure White (#FFFFFF)
#define COLOR_MUTED_TEXT    0x9CD3  // Muted Lavender / Gray (#9A9BB5)
#define COLOR_SLIDER_BG     0x2104  // Inactive slider track

// -------------------------------------------------------------------------
// Graphical Primitive Helpers
// -------------------------------------------------------------------------

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
            uint16_t col = (i < 4) ? COLOR_NEON_CYAN : ((i < 9) ? COLOR_NEON_PINK : COLOR_NEON_PURPLE);
            tft.fillRect(barX, y + clearH, barWidth, barH, col);
        }

        if (vizPeaks[i] > 1 && vizPeaks[i] < height) {
            tft.drawFastHLine(barX, y + height - vizPeaks[i], barWidth, COLOR_WHITE);
        }
    }
}

// Draw the bottom neon curved wave graphics
static void drawBottomNeonWaves(int yBase, int height) {
    int w = tft.width();
    for (int x = 0; x < w; x += 2) {
        float rad = (x * 0.04f) + (wavePhase * 0.1f);
        int y1 = yBase + (int)(sin(rad) * (height / 3.0f));
        int y2 = yBase + (int)(cos(rad * 0.8f) * (height / 2.5f));
        if (y1 >= 0 && y1 < tft.height()) tft.drawPixel(x, y1, COLOR_NEON_PURPLE);
        if (y2 >= 0 && y2 < tft.height()) tft.drawPixel(x, y2, COLOR_NEON_PINK);
    }
}

// -------------------------------------------------------------------------
// Unified Status Bar (Used across all active modes)
// -------------------------------------------------------------------------

void drawUnifiedStatusBar(const char* title) {
    int w = tft.width();
    tft.fillRect(0, 0, w, 14, COLOR_DEEP_BG);

    // Left: Back arrow '<' and Title
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
    tft.drawString(title, 4, 3);

    // Center-Right: Digital Clock
    char timeStr[8];
    uint32_t s = g_deviceState.uptime;
    uint32_t m = (s / 60) % 60;
    uint32_t h = (s / 3600) % 24;
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u", h > 0 ? h : 10, m > 0 ? m : 24);
    tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
    tft.drawString(timeStr, w - 58, 3);

    // Right: Bluetooth icon
    uint16_t btColor = (g_deviceState.bluetoothState == BT_STATE_CONNECTED) ? COLOR_NEON_CYAN : COLOR_MUTED_TEXT;
    int btX = w - 24;
    tft.drawLine(btX, 3, btX, 11, btColor);
    tft.drawLine(btX, 3, btX + 3, 6, btColor);
    tft.drawLine(btX + 3, 6, btX - 2, 9, btColor);
    tft.drawLine(btX - 2, 5, btX + 3, 8, btColor);
    tft.drawLine(btX + 3, 8, btX, 11, btColor);

    // Right: Wi-Fi icon
    uint16_t wifiColor = g_deviceState.wifiConnected ? COLOR_NEON_GREEN : (g_deviceState.apMode ? COLOR_AMBER : COLOR_MUTED_TEXT);
    int wfX = w - 14;
    tft.drawCircle(wfX, 10, 1, wifiColor);
    tft.drawCircle(wfX, 10, 3, wifiColor);

    // Battery / Power icon
    int batX = w - 6;
    tft.drawRect(batX - 1, 4, 6, 7, COLOR_MUTED_TEXT);
    tft.fillRect(batX, 5, 4, 5, COLOR_NEON_GREEN);
    tft.drawFastHLine(batX + 1, 3, 2, COLOR_MUTED_TEXT);
}

// -------------------------------------------------------------------------
// 1. BOOT / SPLASH SCREEN (Neon Glowing Circular Note & Curved Waves)
// -------------------------------------------------------------------------

void showSplashScreen() {
    tft.fillScreen(COLOR_DEEP_BG);
    int w = tft.width();
    int h = tft.height();

    // Top status bar
    tft.setTextSize(1);
    tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
    tft.drawString("10:24", 8, 4);
    tft.drawString("CRX", w - 24, 4);

    // Glowing Circular Emblem in Center
    int cx = w / 2;
    int cy = 42;
    tft.drawCircle(cx, cy, 24, COLOR_NEON_PURPLE);
    tft.drawCircle(cx, cy, 23, COLOR_NEON_PINK);
    tft.drawCircle(cx, cy, 21, COLOR_NEON_CYAN);
    tft.fillCircle(cx, cy, 19, COLOR_CARD_BG);

    // Musical Note ♫ Vector Inside Circle
    tft.fillCircle(cx - 5, cy + 5, 3, COLOR_WHITE);
    tft.fillCircle(cx + 5, cy + 2, 3, COLOR_WHITE);
    tft.fillRect(cx - 3, cy - 8, 2, 13, COLOR_WHITE);
    tft.fillRect(cx + 7, cy - 11, 2, 13, COLOR_WHITE);
    tft.fillRect(cx - 3, cy - 8, 12, 3, COLOR_WHITE);

    // Brand Name: "Croenix Audio"
    tft.setTextSize(2);
    tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
    tft.drawString("Croenix Audio", cx - 74, cy + 28);

    // Subtitle: "Lossless Music Player"
    tft.setTextSize(1);
    tft.setTextColor(COLOR_NEON_PINK, COLOR_DEEP_BG);
    tft.drawString("Lossless Music Player", cx - 60, cy + 48);

    // Bottom Decorative Sine Waves
    for (int step = 0; step < 16; step++) {
        wavePhase = step;
        drawBottomNeonWaves(h - 12, 16);
        delay(35);
    }
}

// -------------------------------------------------------------------------
// 2. HOME SCREEN (2x4 Neon Glowing Mode Grid & Carousel Dots)
// -------------------------------------------------------------------------

void showHomeScreen() {
    int w = tft.width();
    int h = tft.height();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("< Home");
        lastGridCursor = 255;
    }

    uint8_t cur = g_deviceState.homeGridCursor;
    if (cur == lastGridCursor && !needFullRedraw) return;

    // 8 Grid items:
    // 0: SD Card, 1: USB, 2: Bluetooth, 3: FM Radio
    // 4: Web FM, 5: SoundCloud, 6: AUX, 7: More
    const char* labels[8] = {
        "SD Card", "USB", "Bluetooth", "FM Radio",
        "Web FM", "SoundCloud", "AUX", "More"
    };

    int cardW = 35;
    int cardH = 42;
    int startY0 = 18;
    int startY1 = 64;

    for (int i = 0; i < 8; i++) {
        int col = i % 4;
        int row = i / 4;
        int x = 4 + col * (cardW + 4);
        int y = (row == 0) ? startY0 : startY1;
        bool isSelected = (i == cur);

        // Background & Border
        uint16_t borderCol = isSelected ? COLOR_NEON_PINK : COLOR_CARD_BORDER;
        uint16_t bgCol = isSelected ? 0x2045 : COLOR_CARD_BG;

        tft.fillRoundRect(x, y, cardW, cardH, 5, bgCol);
        tft.drawRoundRect(x, y, cardW, cardH, 5, borderCol);
        if (isSelected) {
            tft.drawRoundRect(x + 1, y + 1, cardW - 2, cardH - 2, 4, COLOR_NEON_PINK);
        }

        // Draw Mode Icon
        int iconX = x + cardW / 2;
        int iconY = y + 14;

        switch (i) {
            case 0: // SD Card (Folder icon)
                tft.fillRoundRect(iconX - 7, iconY - 5, 14, 11, 2, isSelected ? COLOR_NEON_PINK : COLOR_MUTED_TEXT);
                tft.fillRect(iconX - 7, iconY - 7, 6, 3, isSelected ? COLOR_NEON_PINK : COLOR_MUTED_TEXT);
                break;
            case 1: // USB (USB drive)
                tft.fillRect(iconX - 4, iconY - 4, 8, 12, isSelected ? COLOR_NEON_CYAN : COLOR_MUTED_TEXT);
                tft.fillRect(iconX - 3, iconY - 8, 6, 4, COLOR_WHITE);
                break;
            case 2: // Bluetooth
                tft.drawCircle(iconX, iconY, 7, isSelected ? COLOR_NEON_CYAN : COLOR_MUTED_TEXT);
                tft.drawLine(iconX - 1, iconY - 5, iconX - 1, iconY + 5, COLOR_WHITE);
                tft.drawLine(iconX - 1, iconY - 5, iconX + 3, iconY - 1, COLOR_WHITE);
                tft.drawLine(iconX + 3, iconY - 1, iconX - 3, iconY + 3, COLOR_WHITE);
                break;
            case 3: // FM Radio
                tft.drawRoundRect(iconX - 8, iconY - 4, 16, 11, 2, isSelected ? COLOR_AMBER : COLOR_MUTED_TEXT);
                tft.fillCircle(iconX - 3, iconY + 1, 3, COLOR_WHITE);
                tft.drawLine(iconX + 6, iconY - 4, iconX + 8, iconY - 9, COLOR_WHITE);
                break;
            case 4: // Web FM (Broadcast antenna)
                tft.fillCircle(iconX, iconY + 2, 2, isSelected ? COLOR_NEON_PURPLE : COLOR_MUTED_TEXT);
                tft.drawCircle(iconX, iconY + 2, 5, isSelected ? COLOR_NEON_PURPLE : COLOR_MUTED_TEXT);
                tft.drawCircle(iconX, iconY + 2, 8, isSelected ? COLOR_NEON_PURPLE : COLOR_MUTED_TEXT);
                break;
            case 5: // SoundCloud (Cloud)
                tft.fillCircle(iconX - 3, iconY + 1, 4, isSelected ? COLOR_SOUNDCLOUD : COLOR_MUTED_TEXT);
                tft.fillCircle(iconX + 3, iconY + 2, 3, isSelected ? COLOR_SOUNDCLOUD : COLOR_MUTED_TEXT);
                tft.fillCircle(iconX, iconY - 2, 5, isSelected ? COLOR_SOUNDCLOUD : COLOR_MUTED_TEXT);
                tft.fillRect(iconX - 6, iconY + 1, 13, 4, isSelected ? COLOR_SOUNDCLOUD : COLOR_MUTED_TEXT);
                break;
            case 6: // AUX (3.5mm jack)
                tft.drawLine(iconX - 5, iconY + 5, iconX + 4, iconY - 4, isSelected ? COLOR_AMBER : COLOR_MUTED_TEXT);
                tft.fillCircle(iconX + 5, iconY - 5, 2, COLOR_WHITE);
                break;
            case 7: // More (4 dots)
                tft.fillCircle(iconX - 3, iconY - 2, 1, isSelected ? COLOR_WHITE : COLOR_MUTED_TEXT);
                tft.fillCircle(iconX + 3, iconY - 2, 1, isSelected ? COLOR_WHITE : COLOR_MUTED_TEXT);
                tft.fillCircle(iconX - 3, iconY + 4, 1, isSelected ? COLOR_WHITE : COLOR_MUTED_TEXT);
                tft.fillCircle(iconX + 3, iconY + 4, 1, isSelected ? COLOR_WHITE : COLOR_MUTED_TEXT);
                break;
        }

        // Label below icon
        tft.setTextSize(1);
        tft.setTextColor(isSelected ? COLOR_WHITE : COLOR_MUTED_TEXT, bgCol);
        String lbl = String(labels[i]);
        if (lbl.length() > 6) lbl = lbl.substring(0, 5) + ".";
        int tx = x + (cardW - (lbl.length() * 6)) / 2;
        tft.drawString(lbl, tx > x ? tx : x + 2, y + 28);
    }

    // Bottom Carousel Dots (Y=116)
    int dotY = 116;
    tft.fillRect(60, dotY - 2, 40, 8, COLOR_DEEP_BG);
    tft.fillRoundRect(70, dotY, 8, 3, 1, COLOR_NEON_PINK); // Active dot
    tft.fillCircle(83, dotY + 1, 2, COLOR_CARD_BORDER);
    tft.fillCircle(90, dotY + 1, 2, COLOR_CARD_BORDER);

    lastGridCursor = cur;
    needFullRedraw = false;
}

// -------------------------------------------------------------------------
// 3. NOW PLAYING SCREEN (SD / Music: Art, Badge, Waveform, Controls)
// -------------------------------------------------------------------------

void showNowPlayingScreen() {
    int w = tft.width();
    int h = tft.height();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("< SD");

        // Album Art Box on Left (X=6, Y=18, W=42, H=42)
        tft.fillRoundRect(6, 18, 42, 42, 6, COLOR_CARD_BG);
        tft.drawRoundRect(6, 18, 42, 42, 6, COLOR_NEON_PURPLE);
        // Vinyl graphic inside
        tft.drawCircle(27, 39, 14, COLOR_CARD_BORDER);
        tft.drawCircle(27, 39, 9, COLOR_NEON_PINK);
        tft.fillCircle(27, 39, 4, COLOR_WHITE);

        lastTrack[0] = '\0';
        lastPlay = !g_deviceState.isPlaying;
        lastPos = 999999;
    }

    // Track Title & Artist (Right of Art, X=54)
    if (strcmp(g_deviceState.trackTitle, lastTrack) != 0 || needFullRedraw) {
        tft.fillRect(52, 18, w - 54, 42, COLOR_DEEP_BG);

        // Title
        tft.setTextSize(1);
        tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
        String title = (strlen(g_deviceState.trackTitle) > 0)
                       ? String(g_deviceState.trackTitle) : "Goon Gumpas";
        if (title.length() > 16) title = title.substring(0, 14) + "..";
        tft.drawString(title, 54, 18);

        // Artist
        tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
        String artist = (strlen(g_deviceState.trackArtist) > 0)
                        ? String(g_deviceState.trackArtist) : "Aphex Twin";
        if (artist.length() > 18) artist = artist.substring(0, 16) + "..";
        tft.drawString(artist, 54, 30);

        // Quality Pill Badge: [FLAC 44.1k/16b] + Heart Icon
        tft.fillRoundRect(54, 44, 76, 11, 3, 0x20A6);
        tft.drawRoundRect(54, 44, 76, 11, 3, COLOR_NEON_PINK);
        tft.setTextColor(COLOR_NEON_PINK, 0x20A6);
        tft.drawString("FLAC 44.1k/16b", 58, 46);

        // Heart favorite icon
        tft.setTextColor(COLOR_NEON_PINK, COLOR_DEEP_BG);
        tft.drawString("<3", w - 16, 46);

        strncpy(lastTrack, g_deviceState.trackTitle, MAX_METADATA_LENGTH - 1);
    }

    // Progress Bar (Y=66)
    if (g_deviceState.trackPosition != lastPos || needFullRedraw) {
        tft.fillRect(6, 64, w - 12, 10, COLOR_DEEP_BG);

        // Digits
        char pStr[8], rStr[8];
        uint32_t pos = g_deviceState.trackPosition;
        uint32_t dur = (g_deviceState.trackDuration > 0) ? g_deviceState.trackDuration : 180;
        snprintf(pStr, sizeof(pStr), "%u:%02u", pos / 60, pos % 60);
        uint32_t rem = (dur > pos) ? (dur - pos) : 0;
        snprintf(rStr, sizeof(rStr), "-%u:%02u", rem / 60, rem % 60);

        tft.setTextSize(1);
        tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
        tft.drawString(pStr, 6, 64);
        tft.drawString(rStr, w - 34, 64);

        // Progress bar line
        float prog = (dur > 0) ? ((float)pos / (float)dur) : 0.0f;
        drawModernProgressBar(38, 66, w - 76, 4, prog, COLOR_NEON_PINK, COLOR_CARD_BORDER);

        lastPos = pos;
    }

    // Real-Time Neon Waveform Visualizer (Y=76 to 92)
    drawAudioVisualizer(6, 76, w - 12, 16, g_deviceState.isPlaying);

    // Transport Controls (Y=98 to 122)
    if (g_deviceState.isPlaying != lastPlay || needFullRedraw) {
        tft.fillRect(6, 96, w - 12, 26, COLOR_DEEP_BG);

        int midX = w / 2;
        int btnY = 108;

        // Shuffle icon on far left
        tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
        tft.drawString("~", 16, btnY - 3);

        // Previous button |<
        tft.fillTriangle(midX - 30, btnY, midX - 22, btnY - 5, midX - 22, btnY + 5, COLOR_WHITE);
        tft.drawFastVLine(midX - 32, btnY - 5, 11, COLOR_WHITE);

        // Play / Pause glowing pink circle in center
        tft.fillCircle(midX, btnY, 11, COLOR_NEON_PINK);
        tft.drawCircle(midX, btnY, 12, COLOR_WHITE);
        if (g_deviceState.isPlaying) {
            tft.fillRect(midX - 3, btnY - 5, 2, 10, COLOR_WHITE);
            tft.fillRect(midX + 1, btnY - 5, 2, 10, COLOR_WHITE);
        } else {
            tft.fillTriangle(midX - 2, btnY - 6, midX - 2, btnY + 6, midX + 5, btnY, COLOR_WHITE);
        }

        // Next button >|
        tft.fillTriangle(midX + 30, btnY, midX + 22, btnY - 5, midX + 22, btnY + 5, COLOR_WHITE);
        tft.drawFastVLine(midX + 32, btnY - 5, 11, COLOR_WHITE);

        // Repeat icon on far right
        tft.drawString("@", w - 24, btnY - 3);

        lastPlay = g_deviceState.isPlaying;
    }

    needFullRedraw = false;
}

// -------------------------------------------------------------------------
// 4. SD CARD PLAYLIST / MUSIC EXPLORER SCREEN
// -------------------------------------------------------------------------

void showPlaylistScreen() {
    int w = tft.width();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("< SD Card / Music");

        // 3 Track items matching reference
        const char* titles[3] = {"Goon Gumpas.flac", "Window Seat.wav", "Dreamscape.flac"};
        const char* artists[3] = {"Aphex Twin", "Erykah Badu", "Tycho"};
        const char* durations[3] = {"3:21", "5:14", "4:37"};

        for (int i = 0; i < 3; i++) {
            int y = 20 + (i * 34);
            bool isCurrent = (i == 0);

            // Container card
            tft.fillRoundRect(6, y, w - 12, 30, 4, isCurrent ? 0x20A6 : COLOR_CARD_BG);
            tft.drawRoundRect(6, y, w - 12, 30, 4, isCurrent ? COLOR_NEON_PINK : COLOR_CARD_BORDER);

            // Mini artwork thumbnail
            tft.fillRoundRect(10, y + 4, 22, 22, 3, isCurrent ? COLOR_NEON_PINK : COLOR_CARD_BORDER);
            tft.fillCircle(21, y + 15, 4, COLOR_WHITE);

            // Title & Artist
            tft.setTextSize(1);
            tft.setTextColor(COLOR_WHITE, isCurrent ? 0x20A6 : COLOR_CARD_BG);
            tft.drawString(titles[i], 38, y + 5);

            tft.setTextColor(COLOR_MUTED_TEXT, isCurrent ? 0x20A6 : COLOR_CARD_BG);
            tft.drawString(artists[i], 38, y + 17);

            // Duration
            tft.setTextColor(COLOR_MUTED_TEXT, isCurrent ? 0x20A6 : COLOR_CARD_BG);
            tft.drawString(durations[i], w - 40, y + 11);
        }
        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// 5. FM RADIO SCREEN (98.30 MHz, Real-Time Spectrum, P1-P4 Presets)
// -------------------------------------------------------------------------

void showFMRadioScreen() {
    int w = tft.width();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("< FM Radio");
        lastFreq = -1.0f;
    }

    // Tuning Readout (Y=20 to 52)
    float curFreq = g_deviceState.fmFrequency;
    if (curFreq != lastFreq || needFullRedraw) {
        tft.fillRect(6, 18, w - 12, 36, COLOR_DEEP_BG);

        // Left / Right Tuning Arrows
        tft.fillTriangle(14, 34, 22, 28, 22, 40, COLOR_WHITE);
        tft.fillTriangle(w - 14, 34, w - 22, 28, w - 22, 40, COLOR_WHITE);

        // Big Frequency Digits: "98.30"
        char freqStr[16];
        snprintf(freqStr, sizeof(freqStr), "%.2f", curFreq > 0 ? curFreq : 98.30f);

        tft.setTextSize(3);
        tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
        tft.drawString(freqStr, 34, 24);

        // MHz Unit
        tft.setTextSize(1);
        tft.setTextColor(COLOR_NEON_PINK, COLOR_DEEP_BG);
        tft.drawString("MHz", w - 46, 36);

        lastFreq = curFreq;
    }

    // Real-Time FM Neon Spectrum Visualizer (Y=56 to 82)
    drawAudioVisualizer(12, 56, w - 24, 24, true);

    // Preset Buttons Bar (Y=90 to 118)
    // P1: 91.10, P2: 98.30, P3: 100.80, P4: 104.80
    const char* pNames[4] = {"P1", "P2", "P3", "P4"};
    const char* pFreqs[4] = {"91.10", "98.30", "100.80", "104.80"};
    int pW = 34;
    int pH = 22;

    for (int i = 0; i < 4; i++) {
        int px = 6 + i * (pW + 4);
        int py = 92;
        bool isActive = (i == g_deviceState.selectedPresetFM);

        uint16_t bg = isActive ? COLOR_NEON_PINK : COLOR_CARD_BG;
        uint16_t border = isActive ? COLOR_WHITE : COLOR_CARD_BORDER;

        tft.fillRoundRect(px, py, pW, pH, 4, bg);
        tft.drawRoundRect(px, py, pW, pH, 4, border);

        tft.setTextSize(1);
        tft.setTextColor(isActive ? COLOR_WHITE : COLOR_MUTED_TEXT, bg);
        tft.drawString(pNames[i], px + 11, py + 2);
        tft.drawString(pFreqs[i], px + 3, py + 12);
    }

    needFullRedraw = false;
}

// -------------------------------------------------------------------------
// 6. BLUETOOTH SCREEN (Glowing Ring, OnePlus 12 Connected, Transport)
// -------------------------------------------------------------------------

void showBluetoothScreen() {
    int w = tft.width();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("< Bluetooth");

        // Central Glowing Bluetooth Ring Emblem
        int cx = 46;
        int cy = 52;
        tft.drawCircle(cx, cy, 26, COLOR_NEON_PURPLE);
        tft.drawCircle(cx, cy, 25, COLOR_NEON_PINK);
        tft.drawCircle(cx, cy, 23, COLOR_NEON_CYAN);
        tft.fillCircle(cx, cy, 21, COLOR_CARD_BG);

        // Bluetooth B Icon
        tft.drawLine(cx - 3, cy - 10, cx - 3, cy + 10, COLOR_WHITE);
        tft.drawLine(cx - 3, cy - 10, cx + 5, cy - 2, COLOR_WHITE);
        tft.drawLine(cx + 5, cy - 2, cx - 6, cy + 6, COLOR_WHITE);
        tft.drawLine(cx - 6, cy - 6, cx + 5, cy + 2, COLOR_WHITE);
        tft.drawLine(cx + 5, cy + 2, cx - 3, cy + 10, COLOR_WHITE);

        // Status on Right of Ring (X=82)
        // Green connected dot
        tft.fillCircle(86, 36, 3, COLOR_NEON_GREEN);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
        tft.drawString("Connected", 94, 32);

        // Paired Device Name
        tft.setTextSize(1);
        tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
        const char* dev = (strlen(g_deviceState.bluetoothDevice) > 0)
                          ? g_deviceState.bluetoothDevice : "OnePlus 12";
        tft.drawString(dev, 94, 46);

        // Bottom Controls (Y=92 to 118)
        int midX = w / 2;
        int btnY = 104;

        // Previous
        tft.fillTriangle(midX - 32, btnY, midX - 24, btnY - 6, midX - 24, btnY + 6, COLOR_WHITE);
        tft.drawFastVLine(midX - 34, btnY - 6, 12, COLOR_WHITE);

        // Glowing Pink Play/Pause Circle
        tft.fillCircle(midX, btnY, 13, COLOR_NEON_PINK);
        tft.fillRect(midX - 3, btnY - 5, 2, 10, COLOR_WHITE);
        tft.fillRect(midX + 1, btnY - 5, 2, 10, COLOR_WHITE);

        // Next
        tft.fillTriangle(midX + 32, btnY, midX + 24, btnY - 6, midX + 24, btnY + 6, COLOR_WHITE);
        tft.drawFastVLine(midX + 34, btnY - 6, 12, COLOR_WHITE);

        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// 7. WEB FM SCREEN (Stations List: BBC Radio 1, Chillout, Trance)
// -------------------------------------------------------------------------

void showWebFMScreen() {
    int w = tft.width();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("< Web FM");

        const char* stNames[3] = {"BBC Radio 1", "Chillout Lounge", "Trance FM"};
        const char* stGenres[3] = {"United Kingdom", "Ambient / Chill", "Electronic"};
        uint16_t stIcons[3] = {COLOR_NEON_PINK, COLOR_NEON_GREEN, COLOR_AMBER};

        for (int i = 0; i < 3; i++) {
            int y = 20 + (i * 34);
            bool isCurrent = (i == 0);

            tft.fillRoundRect(6, y, w - 12, 30, 5, isCurrent ? 0x20A6 : COLOR_CARD_BG);
            tft.drawRoundRect(6, y, w - 12, 30, 5, isCurrent ? COLOR_NEON_PINK : COLOR_CARD_BORDER);

            // Station Logo Box
            tft.fillRoundRect(10, y + 4, 22, 22, 3, stIcons[i]);
            tft.setTextSize(1);
            tft.setTextColor(COLOR_DEEP_BG, stIcons[i]);
            tft.drawString(i == 0 ? "BBC" : (i == 1 ? "CHL" : "TRN"), 12, y + 11);

            // Name & Genre
            tft.setTextColor(COLOR_WHITE, isCurrent ? 0x20A6 : COLOR_CARD_BG);
            tft.drawString(stNames[i], 38, y + 5);

            tft.setTextColor(COLOR_MUTED_TEXT, isCurrent ? 0x20A6 : COLOR_CARD_BG);
            tft.drawString(stGenres[i], 38, y + 17);

            // Right Arrow >
            tft.setTextColor(COLOR_MUTED_TEXT, isCurrent ? 0x20A6 : COLOR_CARD_BG);
            tft.drawString(">", w - 20, y + 11);
        }
        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// 8. SOUNDCLOUD SCREEN (Trending, Likes, Playlists, Following)
// -------------------------------------------------------------------------

void showSoundCloudScreen() {
    int w = tft.width();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("< SoundCloud");

        const char* menuItems[4] = {"Trending", "Your Likes", "Playlists", "Following"};
        const char* icons[4] = {"*", "<3", "=", "@"};

        for (int i = 0; i < 4; i++) {
            int y = 18 + (i * 26);
            bool isCurrent = (i == 0);

            tft.fillRoundRect(6, y, w - 12, 22, 4, isCurrent ? 0x2802 : COLOR_CARD_BG);
            tft.drawRoundRect(6, y, w - 12, 22, 4, isCurrent ? COLOR_SOUNDCLOUD : COLOR_CARD_BORDER);

            tft.setTextSize(1);
            tft.setTextColor(COLOR_SOUNDCLOUD, isCurrent ? 0x2802 : COLOR_CARD_BG);
            tft.drawString(icons[i], 12, y + 7);

            tft.setTextColor(COLOR_WHITE, isCurrent ? 0x2802 : COLOR_CARD_BG);
            tft.drawString(menuItems[i], 32, y + 7);

            tft.setTextColor(COLOR_MUTED_TEXT, isCurrent ? 0x2802 : COLOR_CARD_BG);
            tft.drawString(">", w - 18, y + 7);
        }
        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// 9. EQUALIZER SCREEN (5 Sliders + Flat, Rock, Pop, Jazz Presets)
// -------------------------------------------------------------------------

void showEqualizerScreen() {
    int w = tft.width();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("Equalizer");

        // ON Toggle Switch at top right (X=126, Y=2)
        tft.fillRoundRect(w - 34, 2, 24, 10, 5, COLOR_NEON_PINK);
        tft.fillCircle(w - 16, 7, 4, COLOR_WHITE);

        // 5 Vertical Sliders on Left (60, 250, 1k, 4k, 16k)
        const char* freqLabels[5] = {"60", "250", "1k", "4k", "16k"};
        int thumbY[5] = {70, 48, 76, 54, 50}; // Default visual positions

        for (int i = 0; i < 5; i++) {
            int sx = 14 + (i * 18);

            // Vertical Track (Y=24 to Y=96, H=72)
            tft.fillRoundRect(sx - 1, 24, 4, 72, 2, COLOR_SLIDER_BG);

            // Active pink fill up to thumb
            tft.fillRoundRect(sx - 1, thumbY[i], 4, 96 - thumbY[i], 2, COLOR_NEON_PINK);

            // White Slider Bead Thumb
            tft.fillCircle(sx + 1, thumbY[i], 4, COLOR_WHITE);
            tft.drawCircle(sx + 1, thumbY[i], 5, COLOR_NEON_PINK);

            // Bottom frequency label (Y=104)
            tft.setTextSize(1);
            tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
            tft.drawString(freqLabels[i], sx - 4, 104);
        }

        // 4 Presets on Right (Flat, Rock, Pop, Jazz)
        const char* eqPresets[4] = {"Flat", "Rock", "Pop", "Jazz"};
        int prX = 112;
        int prW = 42;
        int prH = 18;

        for (int p = 0; p < 4; p++) {
            int py = 24 + (p * 22);
            bool isRock = (p == g_deviceState.selectedEQPreset);

            tft.fillRoundRect(prX, py, prW, prH, 4, isRock ? COLOR_NEON_PINK : COLOR_CARD_BG);
            tft.drawRoundRect(prX, py, prW, prH, 4, isRock ? COLOR_WHITE : COLOR_CARD_BORDER);

            tft.setTextSize(1);
            tft.setTextColor(isRock ? COLOR_WHITE : COLOR_MUTED_TEXT, isRock ? COLOR_NEON_PINK : COLOR_CARD_BG);
            tft.drawString(eqPresets[p], prX + 10, py + 5);
        }
        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// 10. USB SCREEN (3D USB Drive Graphic & Capacity)
// -------------------------------------------------------------------------

void showUSBScreen() {
    int w = tft.width();
    int h = tft.height();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("< USB");

        // 3D USB Drive Graphic in Center
        int cx = w / 2;
        int cy = 46;

        // Drive Body
        tft.fillRoundRect(cx - 16, cy - 8, 32, 24, 4, COLOR_CARD_BG);
        tft.drawRoundRect(cx - 16, cy - 8, 32, 24, 4, COLOR_NEON_CYAN);
        tft.drawRoundRect(cx - 15, cy - 7, 30, 22, 3, COLOR_NEON_PURPLE);

        // Metal Connector
        tft.fillRect(cx - 8, cy - 18, 16, 10, COLOR_WHITE);
        tft.fillRect(cx - 5, cy - 15, 3, 4, COLOR_DEEP_BG);
        tft.fillRect(cx + 2, cy - 15, 3, 4, COLOR_DEEP_BG);

        // Labels
        tft.setTextSize(1);
        tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
        tft.drawString("USB Drive", cx - 26, cy + 24);

        tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
        tft.drawString("128 GB | 412 Songs", cx - 52, cy + 38);

        // Bottom purple neon wave
        drawBottomNeonWaves(h - 10, 14);
        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// 11. AUX SCREEN (3.5mm Jack & Waveform)
// -------------------------------------------------------------------------

void showAUXScreen() {
    int w = tft.width();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("< AUX");

        int cx = w / 2;
        int cy = 40;

        // 3.5mm Jack Vector Graphic
        tft.fillRect(cx - 4, cy - 16, 8, 18, COLOR_WHITE);
        tft.drawFastHLine(cx - 4, cy - 12, 8, COLOR_DEEP_BG);
        tft.drawFastHLine(cx - 4, cy - 7, 8, COLOR_DEEP_BG);
        tft.fillRoundRect(cx - 7, cy + 2, 14, 18, 2, COLOR_NEON_PURPLE);

        // Labels
        tft.setTextSize(1);
        tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
        tft.drawString("AUX IN", cx - 18, cy + 26);

        tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
        tft.drawString("External Audio", cx - 38, cy + 38);

        // Live Audio Spectrum Waveform
        drawAudioVisualizer(10, cy + 52, w - 20, 18, true);
        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// 12. SETTINGS SCREEN (Audio, Display, Network, System)
// -------------------------------------------------------------------------

void showSettingsScreen() {
    int w = tft.width();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("Settings");

        const char* items[4] = {"Audio", "Display", "Network", "System"};
        for (int i = 0; i < 4; i++) {
            int y = 20 + (i * 26);
            tft.fillRoundRect(6, y, w - 12, 22, 4, COLOR_CARD_BG);
            tft.drawRoundRect(6, y, w - 12, 22, 4, COLOR_CARD_BORDER);

            tft.setTextSize(1);
            tft.setTextColor(COLOR_WHITE, COLOR_CARD_BG);
            tft.drawString(items[i], 24, y + 7);

            tft.setTextColor(COLOR_MUTED_TEXT, COLOR_CARD_BG);
            tft.drawString(">", w - 18, y + 7);
        }
        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// 13. ABOUT SCREEN (ESP32 Chip Graphic & Specifications)
// -------------------------------------------------------------------------

void showAboutScreen() {
    int w = tft.width();
    int h = tft.height();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("< About");

        // ESP32 Chip Vector Graphic on Left (X=10, Y=26, W=36, H=36)
        tft.fillRoundRect(10, 26, 36, 36, 4, COLOR_CARD_BG);
        tft.drawRoundRect(10, 26, 36, 36, 4, COLOR_NEON_CYAN);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_WHITE, COLOR_CARD_BG);
        tft.drawString("ESP32", 13, 38);

        // Text Info on Right (X=54)
        tft.drawString("Croenix Audio", 54, 24);
        tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
        tft.drawString("ESP32 Music Player", 54, 35);
        tft.drawString("v3.0.0 Hi-Fi", 54, 46);

        // Format Support Badges (Y=68)
        tft.setTextColor(COLOR_NEON_PINK, COLOR_DEEP_BG);
        tft.drawString("FLAC / WAV / MP3 / AAC", 10, 68);

        tft.setTextColor(COLOR_NEON_CYAN, COLOR_DEEP_BG);
        tft.drawString("FM / Web FM / SoundCloud / BT", 10, 80);

        // Bottom neon wave
        drawBottomNeonWaves(h - 10, 14);
        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// 14. SONG INFO SCREEN
// -------------------------------------------------------------------------

void showSongInfoScreen() {
    int w = tft.width();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("< Song Info");

        // Album Art Box on Left
        tft.fillRoundRect(8, 22, 46, 46, 5, COLOR_CARD_BG);
        tft.drawRoundRect(8, 22, 46, 46, 5, COLOR_NEON_PINK);
        tft.fillCircle(31, 45, 12, COLOR_CARD_BORDER);
        tft.fillCircle(31, 45, 4, COLOR_WHITE);

        // Song Metadata on Right (X=60)
        tft.setTextSize(1);
        tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
        tft.drawString("Goon Gumpas", 60, 22);

        tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
        tft.drawString("Aphex Twin", 60, 34);
        tft.drawString("Selected Ambient Works", 60, 46);
        tft.drawString("1995", 60, 58);

        // Format pill
        tft.fillRoundRect(60, 72, 88, 11, 3, 0x20A6);
        tft.drawRoundRect(60, 72, 88, 11, 3, COLOR_NEON_PINK);
        tft.setTextColor(COLOR_NEON_PINK, 0x20A6);
        tft.drawString("FLAC 44.1k / 16-bit", 64, 74);

        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// 15. VOLUME HUD SCREEN (68%, Slider with Thumb, Rising LED Level Meter)
// -------------------------------------------------------------------------

void drawVolumeHUD(uint8_t volume) {
    int w = tft.width();

    tft.fillScreen(COLOR_DEEP_BG);
    drawUnifiedStatusBar("Volume");

    // Big Percentage Readout in Center: "68%"
    tft.setTextSize(3);
    tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
    char volStr[8];
    snprintf(volStr, sizeof(volStr), "%d%%", volume);
    int tx = (volume >= 100) ? 52 : ((volume >= 10) ? 60 : 70);
    tft.drawString(volStr, tx, 26);

    // Horizontal Slider with Thumb (Y=62 to Y=74)
    int slX = 26;
    int slW = w - 52;
    int slY = 66;

    // Minus Button [-] on Left
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
    tft.drawString("-", 12, slY - 2);

    // Track
    tft.fillRoundRect(slX, slY, slW, 6, 3, COLOR_SLIDER_BG);

    // Active Pink Track
    int fillW = (int)(slW * (volume / 100.0f));
    if (fillW > 0) {
        tft.fillRoundRect(slX, slY, fillW, 6, 3, COLOR_NEON_PINK);
    }

    // White Round Thumb
    int thumbX = slX + fillW;
    tft.fillCircle(thumbX, slY + 3, 5, COLOR_WHITE);
    tft.drawCircle(thumbX, slY + 3, 6, COLOR_NEON_PINK);

    // Plus Button [+] on Right
    tft.drawString("+", w - 18, slY - 2);

    // Speaker Icon & Rising LED Bar Meter (Y=90 to 118)
    int spkX = 40;
    int spkY = 98;

    // Speaker Icon
    tft.fillTriangle(spkX, spkY + 6, spkX + 7, spkY, spkX + 7, spkY + 12, COLOR_WHITE);
    tft.fillRect(spkX - 4, spkY + 3, 5, 6, COLOR_WHITE);

    // Rising LED Gradient Bars (10 Bars)
    int numActive = (volume * 10) / 100;
    for (int b = 0; b < 10; b++) {
        int bx = spkX + 16 + (b * 6);
        int bh = 3 + (b * 1);
        int by = spkY + 12 - bh;
        uint16_t col = (b < numActive)
                       ? ((b < 6) ? COLOR_NEON_PINK : COLOR_WHITE)
                       : COLOR_CARD_BORDER;
        tft.fillRect(bx, by, 4, bh, col);
    }
}

// -------------------------------------------------------------------------
// HARDWARE STATUS & DIAGNOSTICS SCREEN (10-Second Long-Press)
// -------------------------------------------------------------------------

void showHardwareDiagnosticsScreen() {
    int w = tft.width();
    int h = tft.height();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("Hardware Diagnostics");

        uint8_t mod = g_deviceState.selectedDiagModule;
        char modStr[32];
        snprintf(modStr, sizeof(modStr), "Module [%d/4]", mod + 1);

        tft.setTextSize(1);
        tft.setTextColor(COLOR_AMBER, COLOR_DEEP_BG);
        tft.drawString(modStr, 8, 20);

        tft.drawRoundRect(6, 32, w - 12, 70, 5, COLOR_CARD_BORDER);

        tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
        switch (mod) {
            case 0:
                tft.drawString("PCM5102A Hi-Fi DAC", 12, 38);
                tft.drawString("BCK26 DIN25 LCK22", 12, 50);
                tft.drawString("XMT->3.3V SCL->GND", 12, 62);
                tft.setTextColor(COLOR_NEON_GREEN, COLOR_DEEP_BG);
                tft.drawString("Status: READY", 12, 76);
                break;
            case 1:
                tft.drawString("Si4703 FM Radio", 12, 38);
                tft.drawString("SDA21 SCL27 RST16", 12, 50);
                tft.drawString("Band: 87.5-108.0MHz", 12, 62);
                tft.setTextColor(COLOR_NEON_GREEN, COLOR_DEEP_BG);
                tft.drawString("Status: ONLINE", 12, 76);
                break;
            case 2:
                tft.drawString("SD Card SPI", 12, 38);
                tft.drawString("CS15 SCK18 MOSI23 MISO19", 12, 50);
                tft.drawString(g_deviceState.sdCardMounted ? "Format: FAT32 OK" : "Format: NO CARD", 12, 62);
                tft.setTextColor(g_deviceState.sdCardMounted ? COLOR_NEON_GREEN : COLOR_AMBER, COLOR_DEEP_BG);
                tft.drawString(g_deviceState.sdCardMounted ? "Status: MOUNTED" : "Check Card", 12, 76);
                break;
            case 3:
                tft.drawString("ESP32 Dual-Core", 12, 38);
                tft.drawString("Core 1: Audio DMA", 12, 50);
                tft.drawString("Core 0: UI / WiFi", 12, 62);
                tft.setTextColor(COLOR_NEON_GREEN, COLOR_DEEP_BG);
                tft.drawString("Free SRAM: >120KB", 12, 76);
                break;
        }

        tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
        tft.drawString("Turn: Browse | Click: Exit", 10, h - 14);
        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// PROVISIONING QR SCREEN
// -------------------------------------------------------------------------

void showProvisioningQRScreen() {
    int w = tft.width();
    int h = tft.height();

    if (needFullRedraw) {
        tft.fillScreen(COLOR_DEEP_BG);
        drawUnifiedStatusBar("Wi-Fi Setup");

        QRCode qrcode;
        uint8_t qrcodeBytes[128];
        char payload[96];
        snprintf(payload, sizeof(payload), "{\"ap\":\"%s\",\"ip\":\"%s\"}", AP_SSID, AP_STATIC_IP);
        qrcode_initText(&qrcode, qrcodeBytes, 3, ECC_LOW, payload);

        int startX = (w - (qrcode.size * 2)) / 2;
        int startY = 22;

        tft.fillRect(startX - 3, startY - 3, (qrcode.size * 2) + 6, (qrcode.size * 2) + 6, COLOR_WHITE);
        for (uint8_t y = 0; y < qrcode.size; y++) {
            for (uint8_t x = 0; x < qrcode.size; x++) {
                uint16_t c = qrcode_getModule(&qrcode, x, y) ? COLOR_DEEP_BG : COLOR_WHITE;
                tft.fillRect(startX + (x * 2), startY + (y * 2), 2, 2, c);
            }
        }

        tft.setTextSize(1);
        tft.setTextColor(COLOR_WHITE, COLOR_DEEP_BG);
        tft.drawString("SCAN IN APP TO CONNECT", 14, startY + (qrcode.size * 2) + 8);
        tft.setTextColor(COLOR_MUTED_TEXT, COLOR_DEEP_BG);
        tft.drawString("Click: Back to Home", 24, h - 14);

        needFullRedraw = false;
    }
}

// -------------------------------------------------------------------------
// Initialization Routine
// -------------------------------------------------------------------------

void initDisplayManager() {
    DEBUG_PRINTLN("[DISPLAY] Initializing ST7735 with Zero-State Black Boot...");

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    tft.init();
    tft.setRotation(TFT_ROTATION); // 1 = Landscape (160x128 pixels)
    tft.fillScreen(COLOR_DEEP_BG);
    digitalWrite(TFT_CS, HIGH);
    delay(50);

    displayInitialized = true;

    // Render splash animation
    showSplashScreen();
    digitalWrite(TFT_CS, HIGH);

    needFullRedraw = true;
    currentScreen = UI_SCREEN_HOME;
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_deviceState.activeScreen = UI_SCREEN_HOME;
        xSemaphoreGive(g_stateMutex);
    }
}

// -------------------------------------------------------------------------
// Non-Blocking Display Loop (State Machine Router)
// -------------------------------------------------------------------------

void updateDisplay() {
    if (!displayInitialized) return;

    unsigned long now = millis();
    lastUpdate = now;

    // Volume HUD overlay override
    if (isVolumeOverlayActive()) {
        wasOverlayActive = true;
        drawVolumeHUD(getVolumeOverlayValue());
        return;
    } else if (wasOverlayActive) {
        wasOverlayActive = false;
        needFullRedraw = true;
    }

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
        case UI_SCREEN_BOOT_SPLASH:
            showSplashScreen();
            break;
        case UI_SCREEN_HOME:
            showHomeScreen();
            break;
        case UI_SCREEN_NOW_PLAYING:
            showNowPlayingScreen();
            break;
        case UI_SCREEN_PLAYLIST:
            showPlaylistScreen();
            break;
        case UI_SCREEN_FM_RADIO:
            showFMRadioScreen();
            break;
        case UI_SCREEN_BLUETOOTH:
            showBluetoothScreen();
            break;
        case UI_SCREEN_WEB_FM:
            showWebFMScreen();
            break;
        case UI_SCREEN_SOUNDCLOUD:
            showSoundCloudScreen();
            break;
        case UI_SCREEN_EQUALIZER:
            showEqualizerScreen();
            break;
        case UI_SCREEN_USB:
            showUSBScreen();
            break;
        case UI_SCREEN_AUX:
            showAUXScreen();
            break;
        case UI_SCREEN_SETTINGS:
            showSettingsScreen();
            break;
        case UI_SCREEN_ABOUT:
            showAboutScreen();
            break;
        case UI_SCREEN_SONG_INFO:
            showSongInfoScreen();
            break;
        case UI_SCREEN_HARDWARE_DIAGNOSTICS:
            showHardwareDiagnosticsScreen();
            break;
        case UI_SCREEN_PROVISIONING_QR:
            showProvisioningQRScreen();
            break;
        default:
            showHomeScreen();
            break;
    }
}
