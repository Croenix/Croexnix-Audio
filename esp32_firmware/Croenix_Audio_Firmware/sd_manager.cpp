#include "sd_manager.h"
#include "device_state.h"

static bool sdMounted = false;

void initSDManager() {
    DEBUG_PRINTLN("[SD] Initializing SD card...");

    // Deselect TFT to avoid SPI bus conflict during SD init
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    // Initialize SPI SD Card with shared bus
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    if (!SD.begin(SD_CS)) {
        DEBUG_PRINTLN("[SD] Card mount failed!");
        return;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        DEBUG_PRINTLN("[SD] No SD card attached!");
        return;
    }

    sdMounted = true;

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    uint64_t usedSize = SD.usedBytes() / (1024 * 1024);

    DEBUG_PRINTF("[SD] Card Type: %s\n",
                 cardType == CARD_MMC ? "MMC" :
                 cardType == CARD_SD ? "SDSC" :
                 cardType == CARD_SDHC ? "SDHC" : "UNKNOWN");
    DEBUG_PRINTF("[SD] Card Size: %llu MB\n", cardSize);
    DEBUG_PRINTF("[SD] Used Space: %llu MB\n", usedSize);

    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.sdCardMounted = true;
        g_deviceState.sdCardSize = SD.cardSize();
        g_deviceState.sdCardUsed = SD.usedBytes();
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTLN("[SD] SD card initialized successfully");
}

bool isSDCardMounted() {
    return sdMounted;
}

uint64_t getSDCardSize() {
    return sdMounted ? SD.cardSize() : 0;
}

uint64_t getSDCardUsed() {
    return sdMounted ? SD.usedBytes() : 0;
}

void listSDFiles(const char* path, JsonArray& filesArray) {
    if (!sdMounted) return;

    File root = SD.open(path);
    if (!root) {
        DEBUG_PRINTF("[SD] Failed to open directory: %s\n", path);
        return;
    }

    if (!root.isDirectory()) {
        DEBUG_PRINTLN("[SD] Not a directory!");
        root.close();
        return;
    }

    File file = root.openNextFile();
    while (file) {
        JsonObject fileObj = filesArray.createNestedObject();

        fileObj["name"] = String(file.name());
        fileObj["size"] = file.size();
        fileObj["is_dir"] = file.isDirectory();

        if (!file.isDirectory()) {
            String filename = String(file.name());
            filename.toLowerCase();

            // Determine file type
            if (filename.endsWith(".mp3")) fileObj["type"] = "mp3";
            else if (filename.endsWith(".flac")) fileObj["type"] = "flac";
            else if (filename.endsWith(".wav")) fileObj["type"] = "wav";
            else fileObj["type"] = "other";
        }

        file = root.openNextFile();
    }

    root.close();
}

bool fileExists(const char* path) {
    if (!sdMounted) return false;
    return SD.exists(path);
}

String readTextFile(const char* path) {
    if (!sdMounted) return "";

    File file = SD.open(path);
    if (!file) return "";

    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }

    file.close();
    return content;
}

void createPlaylist(const char* name) {
    if (!sdMounted) return;

    String filename = String("/playlists/") + name + ".m3u";

    File file = SD.open(filename.c_str(), FILE_WRITE);
    if (file) {
        file.println("#EXTM3U");
        file.close();
        DEBUG_PRINTF("[SD] Playlist created: %s\n", filename.c_str());
    } else {
        DEBUG_PRINTF("[SD] Failed to create playlist: %s\n", filename.c_str());
    }
}

void addToPlaylist(const char* playlistName, const char* filepath) {
    if (!sdMounted) return;

    String filename = String("/playlists/") + playlistName + ".m3u";

    File file = SD.open(filename.c_str(), FILE_APPEND);
    if (file) {
        file.println(filepath);
        file.close();
        DEBUG_PRINTF("[SD] Added to playlist: %s\n", filepath);
    } else {
        DEBUG_PRINTF("[SD] Failed to open playlist: %s\n", filename.c_str());
    }
}

void loadPlaylist(const char* name) {
    if (!sdMounted) return;

    String filename = String("/playlists/") + name + ".m3u";

    File file = SD.open(filename.c_str());
    if (!file) {
        DEBUG_PRINTF("[SD] Failed to open playlist: %s\n", filename.c_str());
        return;
    }

    DEBUG_PRINTF("[SD] Loading playlist: %s\n", name);

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.length() > 0 && !line.startsWith("#")) {
            DEBUG_PRINTF("[SD] Track: %s\n", line.c_str());
            // Add track to queue...
        }
    }

    file.close();
}
