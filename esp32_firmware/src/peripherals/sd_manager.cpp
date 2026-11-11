#include "sd_manager.h"
#include "device_state.h"

static bool sdMounted = false;

bool checkSDIntegrity() {
    if (!sdMounted) return false;
    uint8_t cType = SD.cardType();
    bool ok = (cType != CARD_NONE);
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
        g_deviceState.sdCardMounted = ok;
        if (ok) {
            g_deviceState.sdCardSize = SD.cardSize();
            g_deviceState.sdCardUsed = SD.usedBytes();
        }
        xSemaphoreGive(g_stateMutex);
    }
    return ok;
}

void initSDManager() {
    DEBUG_PRINTLN("[SD] Initializing SD card reader...");

    // Deselect TFT to avoid SPI bus conflict during SD init
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    // Initialize SPI SD Card with shared bus
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    if (!SD.begin(SD_CS)) {
        DEBUG_PRINTLN("[SD] Card mount failed or no card inserted");
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
            g_deviceState.sdCardMounted = false;
            xSemaphoreGive(g_stateMutex);
        }
        return;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        DEBUG_PRINTLN("[SD] No SD card attached!");
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
            g_deviceState.sdCardMounted = false;
            xSemaphoreGive(g_stateMutex);
        }
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

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
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
        DEBUG_PRINTF("[SD] Not a directory: %s\n", path);
        root.close();
        return;
    }

    File file = root.openNextFile();
    int count = 0;

    while (file && count < 100) {
        if (!file.isDirectory()) {
            String filename = String(file.name());

            // Filter audio files
            if (filename.endsWith(".mp3") || filename.endsWith(".MP3") ||
                filename.endsWith(".flac") || filename.endsWith(".FLAC") ||
                filename.endsWith(".wav") || filename.endsWith(".WAV")) {

                JsonObject fileObj = filesArray.createNestedObject();
                fileObj["name"] = filename;
                fileObj["size"] = file.size();
                fileObj["path"] = String(path) + "/" + filename;
                count++;
            }
        }
        file = root.openNextFile();
    }

    root.close();
    DEBUG_PRINTF("[SD] Found %d audio files in %s\n", count, path);
}

bool fileExists(const char* path) {
    if (!sdMounted) return false;
    return SD.exists(path);
}

String readTextFile(const char* path) {
    if (!sdMounted) return "";

    File file = SD.open(path, FILE_READ);
    if (!file) {
        DEBUG_PRINTF("[SD] Failed to open file: %s\n", path);
        return "";
    }

    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }

    file.close();
    return content;
}

void createPlaylist(const char* name) {
    DEBUG_PRINTF("[SD] Creating playlist: %s\n", name);
}

void addToPlaylist(const char* playlistName, const char* filepath) {
    DEBUG_PRINTF("[SD] Adding %s to %s\n", filepath, playlistName);
}

void loadPlaylist(const char* name) {
    DEBUG_PRINTF("[SD] Loading playlist: %s\n", name);
}
