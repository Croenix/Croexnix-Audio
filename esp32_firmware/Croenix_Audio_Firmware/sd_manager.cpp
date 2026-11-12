#include "sd_manager.h"
#include "device_state.h"
#include "audio_manager.h"
#include "Audio.h"

// =========================================================================
// OPTIMIZED SD MANAGER v3.0
// Fast, reliable SD card handling with efficient scanning
// =========================================================================

extern Audio audio;

// SD Card State
static bool s_sdMounted = false;
static uint8_t s_sdPhysicalType = CARD_NONE;
static bool s_sdFat32Error = false;

// Track Database (Optimized with longer path support)
static uint16_t s_sdTrackCount = 0;
static char s_sdTrackPaths[MAX_SD_TRACKS][80];  // Full paths
static char s_sdTrackNames[MAX_SD_TRACKS][MAX_TRACK_NAME_LENGTH];  // Display names

// Forward declarations
static void scanDirectory(const char* dirPath);
static bool attemptSDMount(uint32_t speed);

// -------------------------------------------------------------------------
// OPTIMIZED SD CARD MOUNTING
// -------------------------------------------------------------------------

bool mountSDCard() {
    DEBUG_SD_PRINTLN("[SD] ╔════════════════════════════════════════════════════╗");
    DEBUG_SD_PRINTLN("[SD] ║  SD CARD MOUNT - HIGH RELIABILITY v3.1           ║");
    DEBUG_SD_PRINTLN("[SD] ╚════════════════════════════════════════════════════╝");

    // 1. Power stabilization & clean shutdown of any prior session
    SD.end();
    delay(100);

    // 2. Isolate SPI lines: Disengage both TFT and SD chip selects
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    pinMode(SD_MOSI, OUTPUT);
    pinMode(SD_SCK, OUTPUT);
    pinMode(SD_MISO, INPUT_PULLUP);  // Critical internal pullup for SD MISO line

    delay(50);

    // 3. Initialize SPI bus cleanly
    SPI.end();
    delay(20);
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1);
    SPI.setFrequency(400000); // 400 kHz standard clock for SD SPI mode entry

    // 4. Send 20 dummy bytes (160 clock cycles) with CS HIGH to place SD in SPI mode
    digitalWrite(TFT_CS, HIGH);
    digitalWrite(SD_CS, HIGH);
    for (int i = 0; i < 20; i++) {
        SPI.transfer(0xFF);
    }
    delay(30);

    // 5. Progressive speed attempts (start at 400kHz, then 1MHz, 4MHz, 8MHz)
    bool mounted = false;
    const uint32_t speeds[] = {
        400000,               // 400 kHz - ultra safe
        SD_SPI_SPEED_SLOW,    // 1 MHz
        2000000,              // 2 MHz
        SD_SPI_SPEED_NORMAL   // 4 MHz
    };

    for (int i = 0; i < 4 && !mounted; i++) {
        digitalWrite(TFT_CS, HIGH);
        digitalWrite(SD_CS, HIGH);
        delay(20);

        DEBUG_SD_PRINTF("[SD] Probing bus at %u Hz...", speeds[i]);
        SPI.setFrequency(speeds[i]);
        mounted = SD.begin(SD_CS, SPI, speeds[i]);

        if (mounted) {
            DEBUG_SD_PRINTLN(" ✓ DETECTED!");
            break;
        } else {
            DEBUG_SD_PRINTLN(" ✗");
        }
    }

    s_sdPhysicalType = SD.cardType();

    if (!mounted) {
        digitalWrite(SD_CS, HIGH);
        s_sdMounted = false;

        if (s_sdPhysicalType != CARD_NONE) {
            s_sdFat32Error = true;
            DEBUG_SD_PRINTLN("[SD] ╔════════════════════════════════════════════════╗");
            DEBUG_SD_PRINTLN("[SD] ║  ✗ CARD DETECTED BUT MOUNT FAILED             ║");
            DEBUG_SD_PRINTLN("[SD] ╚════════════════════════════════════════════════╝");
            DEBUG_SD_PRINTLN("[SD] Card detected but filesystem mount failed.");
            DEBUG_SD_PRINTLN("[SD] SOLUTIONS:");
            DEBUG_SD_PRINTLN("[SD]   1. Reformat card as FAT32 (NOT exFAT/NTFS)");
            DEBUG_SD_PRINTLN("[SD]   2. Use card ≤ 32GB for best compatibility");
            DEBUG_SD_PRINTLN("[SD]   3. Try different SD card brand");
        } else {
            s_sdFat32Error = false;
            DEBUG_SD_PRINTLN("[SD] ╔════════════════════════════════════════════════╗");
            DEBUG_SD_PRINTLN("[SD] ║  ✗ NO CARD DETECTED                           ║");
            DEBUG_SD_PRINTLN("[SD] ╚════════════════════════════════════════════════╝");
            DEBUG_SD_PRINTLN("[SD] Check: CS=15, MISO=19 (pullup), MOSI=23, SCK=18");
        }

        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
            g_deviceState.sdCardMounted = false;
            g_deviceState.needDisplayRedraw = true;
            xSemaphoreGive(g_stateMutex);
        }
        return false;
    }

    // Mount successful
    s_sdMounted = true;
    s_sdFat32Error = false;
    digitalWrite(SD_CS, HIGH);

    uint64_t cardSize = SD.cardSize();
    uint64_t usedBytes = SD.usedBytes();

    const char* cardTypeStr =
        (s_sdPhysicalType == CARD_MMC) ? "MMC" :
        (s_sdPhysicalType == CARD_SD) ? "SDSC" :
        (s_sdPhysicalType == CARD_SDHC) ? "SDHC" : "UNKNOWN";

    DEBUG_SD_PRINTLN("[SD] ╔════════════════════════════════════════════════════╗");
    DEBUG_SD_PRINTLN("[SD] ║  ✓ SD CARD MOUNTED SUCCESSFULLY                   ║");
    DEBUG_SD_PRINTLN("[SD] ╚════════════════════════════════════════════════════╝");
    DEBUG_SD_PRINTF("[SD] Type: %s\n", cardTypeStr);
    DEBUG_SD_PRINTF("[SD] Size: %llu MB\n", cardSize / (1024 * 1024));
    DEBUG_SD_PRINTF("[SD] Used: %llu MB\n", usedBytes / (1024 * 1024));
    DEBUG_SD_PRINTF("[SD] Free: %llu MB\n", (cardSize - usedBytes) / (1024 * 1024));

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_deviceState.sdCardMounted = true;
        g_deviceState.sdCardSize = cardSize;
        g_deviceState.sdCardUsed = usedBytes;
        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }

    // Scan for audio files
    DEBUG_SD_PRINTLN("[SD] Scanning for audio files...");
    scanSDAudioFiles();
    DEBUG_SD_PRINTF("[SD] ═══════════════════════════════════════════════════\n");

    return true;
}

bool initSDManager() {
    delay(200); // Power stabilization delay on cold boot
    return mountSDCard();
}

static unsigned long s_lastSDProbeTime = 0;

void updateSDManager() {
    if (!s_sdMounted) {
        unsigned long now = millis();
        // Periodically retry mounting if card is inserted later
        if (now - s_lastSDProbeTime >= 3000) {
            s_lastSDProbeTime = now;
            mountSDCard();
        }
    }
}

// -------------------------------------------------------------------------
// INTEGRITY CHECK (Hot-plug detection)
// -------------------------------------------------------------------------

bool checkSDIntegrity() {
    if (!s_sdMounted) {
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

// -------------------------------------------------------------------------
// STATUS QUERIES
// -------------------------------------------------------------------------

bool isSDCardMounted() {
    return s_sdMounted;
}

uint8_t getSDCardPhysicalType() {
    return s_sdPhysicalType;
}

bool isSDFat32FormatError() {
    return s_sdFat32Error;
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

// -------------------------------------------------------------------------
// OPTIMIZED AUDIO FILE SCANNER
// -------------------------------------------------------------------------

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

            // Check for supported audio formats
            if (fname.endsWith(".mp3") || fname.endsWith(".MP3") ||
                fname.endsWith(".wav") || fname.endsWith(".WAV") ||
                fname.endsWith(".flac") || fname.endsWith(".FLAC") ||
                fname.endsWith(".aac") || fname.endsWith(".AAC") ||
                fname.endsWith(".m4a") || fname.endsWith(".M4A")) {

                // Build full path
                String fullPath;
                if (String(dirPath) == "/") {
                    fullPath = "/" + fname;
                } else {
                    fullPath = String(dirPath) + "/" + fname;
                }

                // Store full path
                strncpy(s_sdTrackPaths[s_sdTrackCount], fullPath.c_str(),
                        sizeof(s_sdTrackPaths[0]) - 1);
                s_sdTrackPaths[s_sdTrackCount][sizeof(s_sdTrackPaths[0]) - 1] = '\0';

                // Create display name (strip path and extension)
                String title = fname;
                int dotIdx = title.lastIndexOf('.');
                if (dotIdx > 0) title = title.substring(0, dotIdx);
                if (title.startsWith("/")) title = title.substring(1);

                // Replace underscores with spaces for better display
                title.replace("_", " ");

                strncpy(s_sdTrackNames[s_sdTrackCount], title.c_str(),
                        sizeof(s_sdTrackNames[0]) - 1);
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

    s_sdTrackCount = 0;

    // Scan root directory first
    scanDirectory("/");

    // Scan common music directories
    const char* musicDirs[] = {
        "/MUSIC", "/music", "/Music",
        "/MP3", "/mp3",
        "/AUDIO", "/audio", "/Audio",
        "/SONGS", "/songs", "/Songs"
    };

    for (int i = 0; i < 9; i++) {
        if (SD.exists(musicDirs[i])) {
            scanDirectory(musicDirs[i]);
        }
    }

    DEBUG_SD_PRINTF("[SD] Found %d audio files\n", s_sdTrackCount);

    // Update device state
    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_deviceState.sdTotalTracks = s_sdTrackCount;

        if (s_sdTrackCount > 0) {
            // Ensure current track index is valid
            if (g_deviceState.sdCurrentTrack >= s_sdTrackCount) {
                g_deviceState.sdCurrentTrack = 0;
            }

            // Set initial track name
            strncpy(g_deviceState.trackTitle,
                    s_sdTrackNames[g_deviceState.sdCurrentTrack],
                    MAX_METADATA_LENGTH - 1);
            g_deviceState.trackTitle[MAX_METADATA_LENGTH - 1] = '\0';
        }

        g_deviceState.needDisplayRedraw = true;
        xSemaphoreGive(g_stateMutex);
    }
}

// -------------------------------------------------------------------------
// TRACK PLAYBACK & NAVIGATION
// -------------------------------------------------------------------------

bool playSDTrack(int index) {
    if (!s_sdMounted) {
        if (!mountSDCard()) return false;
    }

    if (s_sdTrackCount == 0) {
        DEBUG_SD_PRINTLN("[SD] No tracks available");
        return false;
    }

    // Wrap index
    if (index < 0) index = s_sdTrackCount - 1;
    if (index >= s_sdTrackCount) index = 0;

    const char* path = s_sdTrackPaths[index];
    DEBUG_SD_PRINTF("[SD] Playing [%d/%d]: %s\n", index + 1, s_sdTrackCount,
                    s_sdTrackNames[index]);

    // Load track with SPI mutex protection
    bool ok = false;
    if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        digitalWrite(TFT_CS, HIGH);
        ok = audio.connecttoFS(SD, path);
        digitalWrite(SD_CS, HIGH);
        xSemaphoreGive(g_spiMutex);
    } else {
        ok = audio.connecttoFS(SD, path);
    }

    if (ok) {
        // Update device state
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            g_deviceState.sdCurrentTrack = index;
            strncpy(g_deviceState.trackTitle, s_sdTrackNames[index],
                    MAX_METADATA_LENGTH - 1);
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
        DEBUG_SD_PRINTLN("[SD] ✓ Playback started");
    } else {
        DEBUG_SD_PRINTF("[SD] ✗ Failed to load: %s\n", path);
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

// -------------------------------------------------------------------------
// FILE UTILITIES
// -------------------------------------------------------------------------

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
                filename.endsWith(".wav") || filename.endsWith(".WAV") ||
                filename.endsWith(".aac") || filename.endsWith(".AAC") ||
                filename.endsWith(".m4a") || filename.endsWith(".M4A")) {

                JsonObject fileObj = filesArray.createNestedObject();
                fileObj["name"] = filename;
                fileObj["size"] = file.size();
                fileObj["path"] = (String(path) == "/") ?
                                  ("/" + filename) :
                                  (String(path) + "/" + filename);
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

// -------------------------------------------------------------------------
// PLAYLIST MANAGEMENT
// -------------------------------------------------------------------------

void createPlaylist(const char* name) {
    if (!s_sdMounted) return;

    String p = "/playlists/" + String(name) + ".m3u";
    if (!SD.exists("/playlists")) {
        SD.mkdir("/playlists");
    }

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
    DEBUG_SD_PRINTF("[SD] Loading playlist: %s\n", name);
    // Playlist loading implementation can be added here
}
