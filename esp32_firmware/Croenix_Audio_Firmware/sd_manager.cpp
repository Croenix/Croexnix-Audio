#include "sd_manager.h"
#include "device_state.h"
#include "audio_manager.h"
#include "Audio.h"

extern Audio audio;

static bool s_sdMounted = false;
static uint16_t s_sdTrackCount = 0;
static char s_sdTrackPaths[MAX_SD_TRACKS][64];
static char s_sdTrackNames[MAX_SD_TRACKS][48];

static void scanDirectory(const char* dirPath);

// -------------------------------------------------------------
// Robust Multi-Frequency SD Card Mount Engine
// -------------------------------------------------------------

bool mountSDCard() {
    DEBUG_PRINTLN("[SD] Initializing SPI bus and mounting SD card...");

    // 1. Deselect TFT and SD to avoid SPI bus conflict
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    delay(10);

    // 2. Initialize SPI without binding SD_CS to hardware SS
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1);

    // 3. Official SD SPI Standard: Send >= 74 dummy clock cycles with CS HIGH
    // This transitions the SD card internal state machine from SDIO mode to SPI mode
    digitalWrite(SD_CS, HIGH);
    for (int i = 0; i < 20; i++) {
        SPI.transfer(0xFF);
    }
    delay(10);

    bool mounted = false;

    // Attempt 1: 10 MHz (Standard High-Speed SPI)
    DEBUG_PRINTLN("[SD] Attempting SD.begin at 10 MHz...");
    mounted = SD.begin(SD_CS, SPI, SD_SPI_SPEED_FAST);

    // Attempt 2: 4 MHz (High-Reliability Fallback for jumper wires/level shifters)
    if (!mounted) {
        DEBUG_PRINTLN("[SD] 10 MHz failed. Retrying at 4 MHz fallback...");
        digitalWrite(SD_CS, HIGH);
        delay(25);
        mounted = SD.begin(SD_CS, SPI, SD_SPI_SPEED_NORMAL);
    }

    // Attempt 3: 1 MHz (Safe slow mode)
    if (!mounted) {
        DEBUG_PRINTLN("[SD] 4 MHz failed. Retrying at 1 MHz safe mode...");
        digitalWrite(SD_CS, HIGH);
        delay(25);
        mounted = SD.begin(SD_CS, SPI, SD_SPI_SPEED_FALLBACK);
    }

    if (!mounted) {
        DEBUG_PRINTLN("[SD] MOUNT FAILED: No SD card detected or card not formatted as FAT32.");
        digitalWrite(SD_CS, HIGH);
        s_sdMounted = false;
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
            g_deviceState.sdCardMounted = false;
            g_deviceState.needDisplayRedraw = true;
            xSemaphoreGive(g_stateMutex);
        }
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        DEBUG_PRINTLN("[SD] Card reported CARD_NONE!");
        digitalWrite(SD_CS, HIGH);
        s_sdMounted = false;
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
            g_deviceState.sdCardMounted = false;
            g_deviceState.needDisplayRedraw = true;
            xSemaphoreGive(g_stateMutex);
        }
        return false;
    }

    s_sdMounted = true;
    digitalWrite(SD_CS, HIGH);

    uint64_t cardSize = SD.cardSize();
    uint64_t usedBytes = SD.usedBytes();

    DEBUG_PRINTF("[SD] Success! Card Type: %s, Size: %llu MB, Used: %llu MB\n",
                 cardType == CARD_MMC ? "MMC" :
                 cardType == CARD_SD ? "SDSC" :
                 cardType == CARD_SDHC ? "SDHC" : "UNKNOWN",
                 cardSize / (1024 * 1024),
                 usedBytes / (1024 * 1024));

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_deviceState.sdCardMounted = true;
        g_deviceState.sdCardSize = cardSize;
        g_deviceState.sdCardUsed = usedBytes;
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }

    // Index all audio tracks
    scanSDAudioFiles();

    return true;
}

bool initSDManager() {
    return mountSDCard();
}

bool checkSDIntegrity() {
    if (!s_sdMounted) {
        // Hot-plug retry: attempt to mount if previously unmounted
        return mountSDCard();
    }

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

bool isSDCardMounted() {
    return s_sdMounted;
}

uint64_t getSDCardSize() {
    return s_sdMounted ? SD.cardSize() : 0;
}

uint64_t getSDCardUsed() {
    return s_sdMounted ? SD.usedBytes() : 0;
}

uint16_t getSDTotalTracks() {
    return s_sdTrackCount;
}

const char* getSDTrackPath(int index) {
    if (index < 0 || index >= s_sdTrackCount) return "";
    return s_sdTrackPaths[index];
}

const char* getSDTrackName(int index) {
    if (index < 0 || index >= s_sdTrackCount) return "Unknown Track";
    return s_sdTrackNames[index];
}

const char* getSDCurrentTrackName() {
    return getSDTrackName(g_deviceState.sdCurrentTrack);
}

// -------------------------------------------------------------
// Audio File Scanner & Playlist Builder
// -------------------------------------------------------------

static void scanDirectory(const char* dirPath) {
    if (s_sdTrackCount >= MAX_SD_TRACKS) return;

    File root = SD.open(dirPath);
    if (!root || !root.isDirectory()) {
        if (root) root.close();
        return;
    }

    File file = root.openNextFile();
    while (file && s_sdTrackCount < MAX_SD_TRACKS) {
        if (!file.isDirectory()) {
            const char* name = file.name();
            String fname = String(name);
            if (fname.endsWith(".mp3") || fname.endsWith(".MP3") ||
                fname.endsWith(".wav") || fname.endsWith(".WAV") ||
                fname.endsWith(".flac") || fname.endsWith(".FLAC") ||
                fname.endsWith(".aac") || fname.endsWith(".m4a")) {

                // Build full path
                String fullPath;
                if (String(dirPath) == "/") {
                    fullPath = "/" + fname;
                } else {
                    fullPath = String(dirPath) + "/" + fname;
                }

                strncpy(s_sdTrackPaths[s_sdTrackCount], fullPath.c_str(), sizeof(s_sdTrackPaths[0]) - 1);
                s_sdTrackPaths[s_sdTrackCount][sizeof(s_sdTrackPaths[0]) - 1] = '\0';

                // Format display title (strip path and extension)
                String title = fname;
                int dotIdx = title.lastIndexOf('.');
                if (dotIdx > 0) title = title.substring(0, dotIdx);
                if (title.startsWith("/")) title = title.substring(1);

                strncpy(s_sdTrackNames[s_sdTrackCount], title.c_str(), sizeof(s_sdTrackNames[0]) - 1);
                s_sdTrackNames[s_sdTrackCount][sizeof(s_sdTrackNames[0]) - 1] = '\0';

                s_sdTrackCount++;
            }
        }
        file = root.openNextFile();
    }
    root.close();
}

void scanSDAudioFiles() {
    if (!s_sdMounted) return;

    DEBUG_PRINTLN("[SD] Scanning SD card for audio tracks...");
    s_sdTrackCount = 0;

    // Scan root directory
    scanDirectory("/");

    // Scan standard audio directories if present
    if (SD.exists("/MUSIC")) scanDirectory("/MUSIC");
    else if (SD.exists("/music")) scanDirectory("/music");
    else if (SD.exists("/MP3")) scanDirectory("/MP3");
    else if (SD.exists("/AUDIO")) scanDirectory("/AUDIO");

    DEBUG_PRINTF("[SD] Found %d tracks on SD card\n", s_sdTrackCount);

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_deviceState.sdTotalTracks = s_sdTrackCount;
        if (s_sdTrackCount > 0) {
            if (g_deviceState.sdCurrentTrack >= s_sdTrackCount) {
                g_deviceState.sdCurrentTrack = 0;
            }
            strncpy(g_deviceState.trackTitle, s_sdTrackNames[g_deviceState.sdCurrentTrack], MAX_METADATA_LENGTH - 1);
            g_deviceState.trackTitle[MAX_METADATA_LENGTH - 1] = '\0';
        }
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
}

// -------------------------------------------------------------
// Track Playback & Navigation Logic
// -------------------------------------------------------------

bool playSDTrack(int index) {
    if (!s_sdMounted) {
        if (!mountSDCard()) return false;
    }
    if (s_sdTrackCount == 0) {
        DEBUG_PRINTLN("[SD] No tracks found on SD card");
        return false;
    }

    // Wrap around index
    if (index < 0) index = s_sdTrackCount - 1;
    if (index >= s_sdTrackCount) index = 0;

    const char* path = s_sdTrackPaths[index];
    DEBUG_PRINTF("[SD] Playing track [%d/%d]: %s\n", index + 1, s_sdTrackCount, path);

    bool ok = false;
    if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(150)) == pdTRUE) {
        digitalWrite(TFT_CS, HIGH); // Guarantee TFT unselected
        ok = audio.connecttoFS(SD, path);
        digitalWrite(SD_CS, HIGH);
        xSemaphoreGive(g_spiMutex);
    } else {
        ok = audio.connecttoFS(SD, path);
    }

    if (ok) {
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            g_deviceState.sdCurrentTrack = index;
            strncpy(g_deviceState.trackTitle, s_sdTrackNames[index], MAX_METADATA_LENGTH - 1);
            g_deviceState.trackTitle[MAX_METADATA_LENGTH - 1] = '\0';
            g_deviceState.trackArtist[0] = '\0';
            g_deviceState.trackAlbum[0] = '\0';
            g_deviceState.trackPosition = 0;
            g_deviceState.trackDuration = 0;
            g_deviceState.isPlaying = true;
            g_deviceState.currentSource = SOURCE_SD_CARD;
            g_deviceState.needDisplayRedraw = true;
            xSemaphoreGive(g_stateMutex);
        }
        commitSDTrackIndexToNVS(index);
        DEBUG_PRINTLN("[SD] Track playback initiated successfully");
    } else {
        DEBUG_PRINTF("[SD] Failed to start playback of track: %s\n", path);
    }

    return ok;
}

bool playNextSDTrack() {
    if (s_sdTrackCount == 0) return false;
    return playSDTrack(g_deviceState.sdCurrentTrack + 1);
}

bool playPrevSDTrack() {
    if (s_sdTrackCount == 0) return false;
    return playSDTrack(g_deviceState.sdCurrentTrack - 1);
}

// -------------------------------------------------------------
// File & Playlist Helpers
// -------------------------------------------------------------

void listSDFiles(const char* path, JsonArray& filesArray) {
    if (!s_sdMounted) return;

    File root = SD.open(path);
    if (!root || !root.isDirectory()) {
        if (root) root.close();
        return;
    }

    File file = root.openNextFile();
    int count = 0;

    while (file && count < 100) {
        if (!file.isDirectory()) {
            String filename = String(file.name());
            if (filename.endsWith(".mp3") || filename.endsWith(".MP3") ||
                filename.endsWith(".flac") || filename.endsWith(".FLAC") ||
                filename.endsWith(".wav") || filename.endsWith(".WAV")) {

                JsonObject fileObj = filesArray.createNestedObject();
                fileObj["name"] = filename;
                fileObj["size"] = file.size();
                fileObj["path"] = (String(path) == "/") ? ("/" + filename) : (String(path) + "/" + filename);
                count++;
            }
        }
        file = root.openNextFile();
    }
    root.close();
}

bool fileExists(const char* path) {
    if (!s_sdMounted) return false;
    return SD.exists(path);
}

String readTextFile(const char* path) {
    if (!s_sdMounted) return "";

    File file = SD.open(path, FILE_READ);
    if (!file) return "";

    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }
    file.close();
    return content;
}

void createPlaylist(const char* name) {
    if (!s_sdMounted) return;
    String p = "/playlists/" + String(name) + ".m3u";
    if (!SD.exists("/playlists")) SD.mkdir("/playlists");
    File f = SD.open(p.c_str(), FILE_WRITE);
    if (f) f.close();
}

void addToPlaylist(const char* playlistName, const char* filepath) {
    if (!s_sdMounted) return;
    String p = "/playlists/" + String(playlistName) + ".m3u";
    File f = SD.open(p.c_str(), FILE_APPEND);
    if (f) {
        f.println(filepath);
        f.close();
    }
}

void loadPlaylist(const char* name) {
    DEBUG_PRINTF("[SD] Loading playlist: %s\n", name);
}
