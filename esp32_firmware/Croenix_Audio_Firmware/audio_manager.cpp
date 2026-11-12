#include "audio_manager.h"
#include "Audio.h"
#include "device_state.h"
#include "sd_manager.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <math.h>

// Forward declaration
void createBootToneFile();

// =========================================================================
// OPTIMIZED AUDIO MANAGER v3.0
// High-performance, low-CPU audio pipeline with efficient DMA streaming
// =========================================================================

// Audio library instance (ESP32-audioI2S)
Audio audio;

// Audio state
static bool audioInitialized = false;
static AudioSource activeSource = SOURCE_NONE;
static uint32_t lastMetadataUpdate = 0;
static bool audioPlaying = false;

// Performance optimization: Pre-allocated buffer
static uint8_t audioProcessCount = 0;

// -------------------------------------------------------------------------
// INITIALIZATION
// -------------------------------------------------------------------------

void initAudioManager() {
    DEBUG_PRINTLN("[AUDIO] ╔════════════════════════════════════════════════════╗");
    DEBUG_PRINTLN("[AUDIO] ║  CROENIX AUDIO MANAGER v3.0 - OPTIMIZED          ║");
    DEBUG_PRINTLN("[AUDIO] ╚════════════════════════════════════════════════════╝");
    DEBUG_PRINTLN("[AUDIO] Initializing I²S DAC subsystem...");
    DEBUG_PRINTLN("[AUDIO] Module: CJMCU-5102 (PCM5102A)");

    // Configure I²S pins
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

    DEBUG_PRINTF("[AUDIO] Pin Configuration: BCLK=GPIO%d, LRC=GPIO%d, DOUT=GPIO%d\n",
                 I2S_BCLK, I2S_LRC, I2S_DOUT);

    // Set initial volume (0-21 range for ESP32-audioI2S)
    uint8_t mappedVol = map(constrain(g_deviceState.volume, 0, 100), 0, 100, 0, 21);
    audio.setVolume(mappedVol);

    // Configure for high-fidelity stereo output
    audio.forceMono(false);
    audio.setTone(0, 0, 0);  // Flat EQ initially

    audioInitialized = true;
    activeSource = g_deviceState.currentSource;

    DEBUG_PRINTLN("[AUDIO] ✓ I²S initialized successfully");
    DEBUG_PRINTF("[AUDIO] Volume: %d%% (mapped to %d/21)\n",
                 g_deviceState.volume, mappedVol);
    DEBUG_PRINTLN("[AUDIO] ⚠️  CRITICAL HARDWARE CHECKS:");
    DEBUG_PRINTLN("[AUDIO]    → H1L jumper: MUST be at L position (SCK→GND)");
    DEBUG_PRINTLN("[AUDIO]    → H3 jumper: MUST be at H position (XSMT→VCC)");
    DEBUG_PRINTLN("[AUDIO]    → Without these: NO AUDIO or MUTED output!");
    DEBUG_PRINTLN("[AUDIO] ═══════════════════════════════════════════════════");
}

// -------------------------------------------------------------------------
// OPTIMIZED AUDIO PIPELINE - LOW CPU LOAD
// -------------------------------------------------------------------------

void updateAudioPipeline() {
    if (!audioInitialized) return;

    // Optimized audio processing based on source
    if (activeSource == SOURCE_SD_CARD) {
        // SD Card playback requires SPI mutex management
        if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            digitalWrite(TFT_CS, HIGH);  // Ensure TFT inactive

            // Process audio frames (optimized iteration count)
            for (int i = 0; i < AUDIO_LOOP_ITERATIONS; i++) {
                audio.loop();
                if (!audio.isRunning()) break;
            }

            digitalWrite(SD_CS, HIGH);  // Deselect SD
            xSemaphoreGive(g_spiMutex);
        } else {
            // Fallback without mutex (shouldn't happen often)
            for (int i = 0; i < 3; i++) {
                audio.loop();
                if (!audio.isRunning()) break;
            }
        }
    }
    else if (activeSource == SOURCE_WEB_STREAM || activeSource == SOURCE_BLUETOOTH) {
        // Network/Bluetooth sources - no SPI contention
        for (int i = 0; i < AUDIO_LOOP_ITERATIONS; i++) {
            audio.loop();
            if (!audio.isRunning()) break;
        }
    }
    else if (activeSource == SOURCE_FM_RADIO) {
        // FM radio - minimal processing
        for (int i = 0; i < 4; i++) {
            audio.loop();
            if (!audio.isRunning()) break;
        }
    }

    // Update playback state (throttled to reduce overhead)
    uint32_t now = millis();
    bool isRunning = audio.isRunning();

    if (isRunning != audioPlaying) {
        audioPlaying = isRunning;
        setPlaying(isRunning);
    }

    // Throttled metadata/position updates (every METADATA_UPDATE_INTERVAL ms)
    if (isRunning && (now - lastMetadataUpdate >= METADATA_UPDATE_INTERVAL)) {
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            g_deviceState.trackPosition = audio.getAudioCurrentTime();
            g_deviceState.trackDuration = audio.getAudioFileDuration();
            xSemaphoreGive(g_stateMutex);
        }
        lastMetadataUpdate = now;
    }
}

// -------------------------------------------------------------------------
// PLAYBACK CONTROL
// -------------------------------------------------------------------------

void playAudio() {
    if (!audioInitialized) return;

    DEBUG_AUDIO_PRINTLN("[AUDIO] Play requested");

    if (activeSource == SOURCE_SD_CARD) {
        if (!audio.isRunning() && getSDTotalTracks() > 0) {
            playSDTrack(g_deviceState.sdCurrentTrack);
        } else if (audio.isRunning()) {
            audio.pauseResume();
        }
    } else {
        audio.pauseResume();
        setPlaying(true);
    }
}

void pauseAudio() {
    if (!audioInitialized) return;

    DEBUG_AUDIO_PRINTLN("[AUDIO] Pause requested");

    if (audio.isRunning()) {
        audio.pauseResume();
        setPlaying(false);
    }
}

void stopAudio() {
    if (!audioInitialized) return;

    DEBUG_AUDIO_PRINTLN("[AUDIO] Stop requested");

    audio.stopSong();
    setPlaying(false);
    audioPlaying = false;
}

void nextTrack() {
    if (activeSource == SOURCE_SD_CARD) {
        DEBUG_AUDIO_PRINTLN("[AUDIO] Next track");
        playNextSDTrack();
    }
}

void previousTrack() {
    if (activeSource == SOURCE_SD_CARD) {
        DEBUG_AUDIO_PRINTLN("[AUDIO] Previous track");
        playPrevSDTrack();
    }
}

// -------------------------------------------------------------------------
// VOLUME CONTROL
// -------------------------------------------------------------------------

void setMasterVolume(uint8_t volume) {
    volume = constrain(volume, 0, 100);

    // Map 0-100 to 0-21 (ESP32-audioI2S range)
    uint8_t mappedVolume = map(volume, 0, 100, 0, 21);

    if (audioInitialized) {
        audio.setVolume(mappedVolume);
    }

    setVolume(volume);
    DEBUG_PRINTF("[AUDIO] Volume: %d%% (I²S gain: %d/21)\n", volume, mappedVolume);
}

uint8_t getMasterVolume() {
    return g_deviceState.volume;
}

// -------------------------------------------------------------------------
// SOURCE SWITCHING
// -------------------------------------------------------------------------

bool switchAudioSource(AudioSource source) {
    DEBUG_PRINTF("[AUDIO] Switching source: %d → %d\n", activeSource, source);

    // Stop current playback
    stopAudio();
    delay(50);  // Brief delay for clean transition

    // Update source
    activeSource = source;
    setAudioSource(source);

    // Initialize new source
    switch (source) {
        case SOURCE_SD_CARD:
            if (!isSDCardMounted()) {
                DEBUG_PRINTLN("[AUDIO] Mounting SD card...");
                if (!mountSDCard()) {
                    DEBUG_PRINTLN("[AUDIO] ✗ SD card mount failed");
                    return false;
                }
            }
            if (getSDTotalTracks() > 0) {
                playSDTrack(g_deviceState.sdCurrentTrack);
            }
            break;

        case SOURCE_BLUETOOTH:
            DEBUG_PRINTLN("[AUDIO] Bluetooth source activated");
            break;

        case SOURCE_FM_RADIO:
            DEBUG_PRINTLN("[AUDIO] FM radio source activated");
            break;

        case SOURCE_WEB_STREAM:
            DEBUG_PRINTLN("[AUDIO] Web stream source activated");
            if (WiFi.status() == WL_CONNECTED) {
                DEBUG_PRINTLN("[AUDIO] Connected to Wi-Fi: Auto-streaming SoundCloud channel...");
                connectToWebStream("http://stream.zeno.fm/f3wvbbqmdg8uv");
            } else {
                DEBUG_PRINTLN("[AUDIO] Wi-Fi offline - playing verification chime to test DAC hardware...");
                playBootChime();
            }
            break;

        default:
            break;
    }

    return true;
}

// -------------------------------------------------------------------------
// EQUALIZER PRESETS
// -------------------------------------------------------------------------

void setEQPreset(uint8_t preset) {
    int8_t bass = 0, treble = 0;

    switch (preset) {
        case 0: // Flat
            bass = 0; treble = 0;
            break;
        case 1: // Rock
            bass = 6; treble = 4;
            break;
        case 2: // Pop
            bass = 4; treble = 6;
            break;
        case 3: // Classical
            bass = -2; treble = 5;
            break;
        case 4: // Jazz
            bass = 3; treble = 3;
            break;
        case 5: // Bass Boost
            bass = 10; treble = 0;
            break;
        case 6: // Treble Boost
            bass = 0; treble = 10;
            break;
        default:
            bass = 0; treble = 0;
    }

    if (audioInitialized) {
        audio.setTone(bass, treble, 0);
    }

    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.eqPreset = preset;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTF("[AUDIO] EQ preset: %d (Bass: %+d, Treble: %+d)\n", preset, bass, treble);
}

uint8_t getEQPreset() {
    return g_deviceState.eqPreset;
}

// -------------------------------------------------------------------------
// FILE LOADING (SD CARD)
// -------------------------------------------------------------------------

bool loadMP3File(const char* filepath) {
    if (!audioInitialized) return false;

    DEBUG_PRINTF("[AUDIO] Loading MP3: %s\n", filepath);

    bool success = false;
    if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        digitalWrite(TFT_CS, HIGH);
        success = audio.connecttoFS(SD, filepath);
        digitalWrite(SD_CS, HIGH);
        xSemaphoreGive(g_spiMutex);
    } else {
        success = audio.connecttoFS(SD, filepath);
    }

    if (success) {
        switchAudioSource(SOURCE_SD_CARD);
        DEBUG_AUDIO_PRINTLN("[AUDIO] ✓ MP3 loaded");
    } else {
        DEBUG_PRINTLN("[AUDIO] ✗ MP3 load failed");
    }

    return success;
}

bool loadFLACFile(const char* filepath) {
    if (!audioInitialized) return false;

    DEBUG_PRINTF("[AUDIO] Loading FLAC: %s\n", filepath);

    bool success = false;
    if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        digitalWrite(TFT_CS, HIGH);
        success = audio.connecttoFS(SD, filepath);
        digitalWrite(SD_CS, HIGH);
        xSemaphoreGive(g_spiMutex);
    } else {
        success = audio.connecttoFS(SD, filepath);
    }

    if (success) {
        switchAudioSource(SOURCE_SD_CARD);
        DEBUG_AUDIO_PRINTLN("[AUDIO] ✓ FLAC loaded");
    } else {
        DEBUG_PRINTLN("[AUDIO] ✗ FLAC load failed");
    }

    return success;
}

bool loadWAVFile(const char* filepath) {
    if (!audioInitialized) return false;

    DEBUG_PRINTF("[AUDIO] Loading WAV: %s\n", filepath);

    bool success = false;
    if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        digitalWrite(TFT_CS, HIGH);
        success = audio.connecttoFS(SD, filepath);
        digitalWrite(SD_CS, HIGH);
        xSemaphoreGive(g_spiMutex);
    } else {
        success = audio.connecttoFS(SD, filepath);
    }

    if (success) {
        switchAudioSource(SOURCE_SD_CARD);
        DEBUG_AUDIO_PRINTLN("[AUDIO] ✓ WAV loaded");
    } else {
        DEBUG_PRINTLN("[AUDIO] ✗ WAV load failed");
    }

    return success;
}

// -------------------------------------------------------------------------
// WEB STREAMING
// -------------------------------------------------------------------------

bool connectToWebStream(const char* url) {
    if (!audioInitialized) return false;

    DEBUG_PRINTF("[AUDIO] Connecting to stream: %s\n", url);

    bool success = audio.connecttohost(url);

    if (success) {
        switchAudioSource(SOURCE_WEB_STREAM);
        DEBUG_PRINTLN("[AUDIO] ✓ Stream connected");
    } else {
        DEBUG_PRINTLN("[AUDIO] ✗ Stream connection failed");
    }

    return success;
}

void disconnectWebStream() {
    if (audioInitialized) {
        audio.stopSong();
        DEBUG_PRINTLN("[AUDIO] Stream disconnected");
    }
}

// -------------------------------------------------------------------------
// STATUS QUERIES
// -------------------------------------------------------------------------

bool isAudioPlaying() {
    return audioInitialized && audio.isRunning();
}

uint32_t getCurrentPosition() {
    return audioInitialized ? audio.getAudioCurrentTime() : 0;
}

uint32_t getCurrentDuration() {
    return audioInitialized ? audio.getAudioFileDuration() : 0;
}

AudioSource getActiveSource() {
    return activeSource;
}

// =========================================================================
// AUDIO EVENT CALLBACKS (ESP32-audioI2S Library)
// =========================================================================

void audio_info(const char *info) {
    DEBUG_AUDIO_PRINTF("[AUDIO INFO] %s\n", info);
}

void audio_id3data(const char *info) {
    if (!info || strlen(info) == 0) return;

    DEBUG_AUDIO_PRINTF("[AUDIO ID3] %s\n", info);

    String data = String(info);

    if (data.startsWith("Title=")) {
        String title = data.substring(6);
        updateMetadata(title.c_str(), nullptr, nullptr);
    }
    else if (data.startsWith("Artist=")) {
        String artist = data.substring(7);
        updateMetadata(nullptr, artist.c_str(), nullptr);
    }
    else if (data.startsWith("Album=")) {
        String album = data.substring(6);
        updateMetadata(nullptr, nullptr, album.c_str());
    }
}

void audio_showstreamtitle(const char *info) {
    if (!info || strlen(info) == 0) return;

    DEBUG_AUDIO_PRINTF("[AUDIO STREAM] %s\n", info);

    String s = String(info);
    int sep = s.indexOf(" - ");

    if (sep > 0) {
        String artist = s.substring(0, sep);
        String title = s.substring(sep + 3);
        updateMetadata(title.c_str(), artist.c_str(), "Live Stream");
    } else {
        updateMetadata(s.c_str(), "Live", "Stream");
    }
}

void audio_eof_mp3(const char *info) {
    DEBUG_AUDIO_PRINTLN("[AUDIO] End of track");

    if (activeSource == SOURCE_SD_CARD) {
        playNextSDTrack();
    } else {
        setPlaying(false);
        audioPlaying = false;
    }
}

void audio_bitrate(const char *info) {
    if (!info) return;

    uint16_t bitrate = atoi(info);

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        g_deviceState.trackBitrate = bitrate;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_AUDIO_PRINTF("[AUDIO] Bitrate: %d kbps\n", bitrate);
}

void audio_eof_stream(const char *info) {
    DEBUG_AUDIO_PRINTLN("[AUDIO] Stream ended");
    setPlaying(false);
    audioPlaying = false;
}

// -------------------------------------------------------------------------
// HARDWARE AUDIO VERIFICATION CHIME (Runs on SPIFFS without internet/SD)
// -------------------------------------------------------------------------

void createBootToneFile() {
    if (!SPIFFS.begin(true)) {
        DEBUG_PRINTLN("[AUDIO] SPIFFS mount failed for boot tone");
        return;
    }
    if (SPIFFS.exists("/boot_tone.wav")) {
        return;
    }

    File f = SPIFFS.open("/boot_tone.wav", "w");
    if (!f) return;

    // Standard 44-byte WAV header for 44100 Hz, 16-bit, Stereo, 1.2s duration
    uint32_t sampleRate = 44100;
    uint16_t numChannels = 2;
    uint16_t bitsPerSample = 16;
    uint32_t numSamples = (uint32_t)(sampleRate * 1.2f);
    uint32_t subchunk2Size = numSamples * numChannels * (bitsPerSample / 8);
    uint32_t chunkSize = 36 + subchunk2Size;

    f.write((const uint8_t*)"RIFF", 4);
    f.write((const uint8_t*)&chunkSize, 4);
    f.write((const uint8_t*)"WAVEfmt ", 8);
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1; // PCM
    f.write((const uint8_t*)&subchunk1Size, 4);
    f.write((const uint8_t*)&audioFormat, 2);
    f.write((const uint8_t*)&numChannels, 2);
    f.write((const uint8_t*)&sampleRate, 4);
    uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    f.write((const uint8_t*)&byteRate, 4);
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);
    f.write((const uint8_t*)&blockAlign, 2);
    f.write((const uint8_t*)&bitsPerSample, 2);
    f.write((const uint8_t*)"data", 4);
    f.write((const uint8_t*)&subchunk2Size, 4);

    // Two-tone chime: 523Hz (C5) for 0.5s, then 659Hz (E5) for 0.7s with gentle decay
    int16_t buf[256];
    for (uint32_t i = 0; i < numSamples; i += 128) {
        for (int s = 0; s < 128; s++) {
            uint32_t n = i + s;
            float t = (float)n / (float)sampleRate;
            float freq = (t < 0.45f) ? 523.25f : 659.25f;
            float env = 1.0f - (t / 1.3f);
            if (env < 0.0f) env = 0.0f;
            int16_t val = (int16_t)(sinf(2.0f * 3.14159265f * freq * t) * 14000.0f * env);
            buf[s * 2] = val;     // Left
            buf[s * 2 + 1] = val; // Right
        }
        f.write((const uint8_t*)buf, 128 * 4);
    }
    f.close();
    DEBUG_PRINTLN("[AUDIO] ✓ Generated Croenix boot chime in SPIFFS (/boot_tone.wav)");
}

bool playBootChime() {
    if (!audioInitialized) return false;
    createBootToneFile();
    DEBUG_PRINTLN("[AUDIO] ▶ Playing Croenix audio verification chime to DAC...");
    return audio.connecttoFS(SPIFFS, "/boot_tone.wav");
}
