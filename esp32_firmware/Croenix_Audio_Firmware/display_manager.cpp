#include "display_manager.h"
#include "device_state.h"

// TFT instance
TFT_eSPI tft = TFT_eSPI();

// Display state
static bool displayInitialized = false;
static uint8_t currentScreen = 0;
static unsigned long lastUpdate = 0;

// Colors (RGB565)
#define COLOR_BG        0x0000  // Black
#define COLOR_PRIMARY   0x07FF  // Cyan
#define COLOR_TEXT      0xFFFF  // White
#define COLOR_SECONDARY 0x7BEF  // Light Gray
#define COLOR_ACCENT    0xF800  // Red

void initDisplayManager() {
    DEBUG_PRINTLN("[DISPLAY] Initializing TFT display...");

    tft.init();
    tft.setRotation(TFT_ROTATION);
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);

    displayInitialized = true;

    // Show splash screen
    showSplashScreen();

    DEBUG_PRINTLN("[DISPLAY] TFT initialized successfully");
}

void updateDisplay() {
    if (!displayInitialized) return;

    unsigned long now = millis();
    if (now - lastUpdate < DISPLAY_UPDATE_RATE) return;
    lastUpdate = now;

    // Show main screen
    showMainScreen();
}

void showSplashScreen() {
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_PRIMARY);

    int centerX = TFT_WIDTH / 2;
    int centerY = TFT_HEIGHT / 2;

    tft.setCursor(centerX - 60, centerY - 10);
    tft.println("CROENIX");

    tft.setTextSize(1);
    tft.setTextColor(COLOR_SECONDARY);
    tft.setCursor(centerX - 50, centerY + 20);
    tft.println("Audio Player");

    delay(2000);
}

void showMainScreen() {
    tft.fillScreen(COLOR_BG);

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        // --- Header: Source & Wi-Fi Status ---
        tft.setTextSize(1);
        tft.setTextColor(COLOR_SECONDARY);

        const char* sourceName = "";
        switch (g_deviceState.currentSource) {
            case SOURCE_BLUETOOTH:  sourceName = "BT"; break;
            case SOURCE_SD_CARD:    sourceName = "SD"; break;
            case SOURCE_FM_RADIO:   sourceName = "FM"; break;
            case SOURCE_WEB_STREAM: sourceName = "WEB"; break;
            default:                sourceName = "---"; break;
        }

        tft.setCursor(2, 2);
        tft.printf("[%s]", sourceName);

        // Wi-Fi indicator
        if (g_deviceState.wifiConnected) {
            tft.setTextColor(COLOR_PRIMARY);
            tft.setCursor(TFT_WIDTH - 30, 2);
            tft.printf("WiFi");
        } else if (g_deviceState.apMode) {
            tft.setTextColor(COLOR_ACCENT);
            tft.setCursor(TFT_WIDTH - 20, 2);
            tft.printf("AP");
        }

        // --- Track Metadata ---
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT);

        // Title
        tft.setCursor(2, 20);
        if (strlen(g_deviceState.trackTitle) > 0) {
            String title = String(g_deviceState.trackTitle);
            if (title.length() > 26) title = title.substring(0, 23) + "...";
            tft.println(title);
        } else {
            tft.println("No Track");
        }

        // Artist
        tft.setTextColor(COLOR_SECONDARY);
        tft.setCursor(2, 32);
        if (strlen(g_deviceState.trackArtist) > 0) {
            String artist = String(g_deviceState.trackArtist);
            if (artist.length() > 26) artist = artist.substring(0, 23) + "...";
            tft.println(artist);
        } else {
            tft.println("---");
        }

        // --- Progress Bar ---
        if (g_deviceState.trackDuration > 0) {
            float progress = (float)g_deviceState.trackPosition / (float)g_deviceState.trackDuration;
            drawProgressBar(2, 50, TFT_WIDTH - 4, 6, progress);

            // Time display
            tft.setTextSize(1);
            tft.setTextColor(COLOR_SECONDARY);

            int posMin = g_deviceState.trackPosition / 60;
            int posSec = g_deviceState.trackPosition % 60;
            int durMin = g_deviceState.trackDuration / 60;
            int durSec = g_deviceState.trackDuration % 60;

            tft.setCursor(2, 60);
            tft.printf("%d:%02d / %d:%02d", posMin, posSec, durMin, durSec);
        }

        // --- Bitrate & EQ ---
        tft.setTextSize(1);
        tft.setTextColor(COLOR_SECONDARY);
        tft.setCursor(2, 75);
        if (g_deviceState.trackBitrate > 0) {
            tft.printf("Bitrate: %d kbps", g_deviceState.trackBitrate);
        }

        const char* eqNames[] = {"Flat", "Rock", "Pop", "Classical", "Jazz", "Bass"};
        tft.setCursor(2, 87);
        tft.printf("EQ: %s", eqNames[g_deviceState.eqPreset]);

        // --- Volume Bar ---
        tft.setTextColor(COLOR_PRIMARY);
        tft.setCursor(2, 105);
        tft.printf("Volume: %d%%", g_deviceState.volume);

        drawProgressBar(2, 118, TFT_WIDTH - 4, 8, g_deviceState.volume / 100.0);

        // --- Network Info ---
        tft.setTextSize(1);
        tft.setTextColor(COLOR_SECONDARY);
        tft.setCursor(2, 133);

        if (g_deviceState.wifiConnected) {
            tft.printf("IP: %s", g_deviceState.localIP);
            tft.setCursor(2, 145);
            tft.printf("RSSI: %d dBm", g_deviceState.wifiRSSI);
        } else if (g_deviceState.apMode) {
            tft.println("AP Mode: Setup Required");
        } else {
            tft.println("No Network Connection");
        }

        xSemaphoreGive(g_stateMutex);
    }
}

void drawProgressBar(int x, int y, int width, int height, float progress) {
    if (progress < 0.0) progress = 0.0;
    if (progress > 1.0) progress = 1.0;

    int fillWidth = (int)(width * progress);

    // Border
    tft.drawRect(x, y, width, height, COLOR_SECONDARY);

    // Fill
    if (fillWidth > 2) {
        tft.fillRect(x + 1, y + 1, fillWidth - 2, height - 2, COLOR_PRIMARY);
    }
}

void drawVUMeter(int x, int y, int width, int height, uint8_t level) {
    // Simple VU meter visualization
    int barCount = 10;
    int barWidth = (width / barCount) - 2;

    int activeBars = map(level, 0, 100, 0, barCount);

    for (int i = 0; i < barCount; i++) {
        int barX = x + (i * (barWidth + 2));
        uint16_t color = (i < activeBars) ? COLOR_PRIMARY : COLOR_BG;

        if (i >= 7 && i < activeBars) color = COLOR_ACCENT; // Red for high levels

        tft.fillRect(barX, y, barWidth, height, color);
    }
}

void showSourceScreen() {
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_PRIMARY);
    tft.setCursor(10, 20);
    tft.println("Select Source");

    // Source list...
}

void showVolumeScreen() {
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(3);
    tft.setTextColor(COLOR_PRIMARY);
    tft.setCursor(20, 50);
    tft.printf("Vol: %d%%", g_deviceState.volume);
}

void showNetworkScreen() {
    tft.fillScreen(COLOR_BG);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_PRIMARY);
    tft.setCursor(2, 10);
    tft.println("Network Status");

    // Network details...
}
