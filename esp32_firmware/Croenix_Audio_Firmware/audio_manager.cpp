#include "audio_manager.h"
#include "Audio.h"
#include "device_state.h"

// Audio library instance (ESP32-audioI2S)
Audio audio;

// Current playback state
static bool audioInitialized = false;
static AudioSource activeSource = SOURCE_NONE;

void initAudioManager() {
    DEBUG_PRINTLN("[AUDIO] Initializing I2S audio subsystem...");

    // Configure I2S pins
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

    // Set initial volume (0-21)
    audio.setVolume(12);

    // Configure audio quality
    audio.forceMono(false);  // Keep stereo
    audio.setTone(0, 0, 0);  // Flat EQ by default

    audioInitialized = true;

    DEBUG_PRINTLN("[AUDIO] I2S initialized successfully");
    DEBUG_PRINTF("[AUDIO] Sample Rate: %d Hz, Bits: %d, Channels: %d\n",
                 AUDIO_SAMPLE_RATE, AUDIO_BITS_PER_SAMPLE, AUDIO_CHANNELS);
}

void updateAudioPipeline() {
    if (!audioInitialized) return;

    // Process audio loop (handles buffering, decoding, DMA)
    audio.loop();

    // Update state if playing
    if (audio.isRunning()) {
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            g_deviceState.isPlaying = true;
            g_deviceState.trackPosition = audio.getAudioCurrentTime();
            g_deviceState.trackDuration = audio.getAudioFileDuration();

            xSemaphoreGive(g_stateMutex);
        }
    }
}

void playAudio() {
    if (audioInitialized && !audio.isRunning()) {
        audio.pauseResume();
        setPlaying(true);
        DEBUG_PRINTLN("[AUDIO] Playback started");
    }
}

void pauseAudio() {
    if (audioInitialized && audio.isRunning()) {
        audio.pauseResume();
        setPlaying(false);
        DEBUG_PRINTLN("[AUDIO] Playback paused");
    }
}

void stopAudio() {
    if (audioInitialized) {
        audio.stopSong();
        setPlaying(false);
        DEBUG_PRINTLN("[AUDIO] Playback stopped");
    }
}

void nextTrack() {
    // Handled by SD manager or playlist logic
    DEBUG_PRINTLN("[AUDIO] Next track requested");
}

void previousTrack() {
    // Handled by SD manager or playlist logic
    DEBUG_PRINTLN("[AUDIO] Previous track requested");
}

void setMasterVolume(uint8_t volume) {
    if (volume > 100) volume = 100;

    // Convert 0-100 to 0-21 (ESP32-audioI2S range)
    uint8_t mappedVolume = map(volume, 0, 100, 0, 21);

    if (audioInitialized) {
        audio.setVolume(mappedVolume);
    }

    setVolume(volume);
    DEBUG_PRINTF("[AUDIO] Volume set to %d%%\n", volume);
}

uint8_t getMasterVolume() {
    return g_deviceState.volume;
}

bool switchAudioSource(AudioSource source) {
    DEBUG_PRINTF("[AUDIO] Switching source to: %d\n", source);

    // Stop current playback
    stopAudio();

    // Update active source
    activeSource = source;
    setAudioSource(source);

    return true;
}

void setEQPreset(uint8_t preset) {
    int8_t bass = 0, treble = 0;

    switch (preset) {
        case 0: // Flat
            bass = 0; treble = 0;
            break;
        case 1: // Rock
            bass = 5; treble = 3;
            break;
        case 2: // Pop
            bass = 3; treble = 5;
            break;
        case 3: // Classical
            bass = -2; treble = 4;
            break;
        case 4: // Jazz
            bass = 2; treble = 2;
            break;
        case 5: // Bass Boost
            bass = 8; treble = 0;
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

    DEBUG_PRINTF("[AUDIO] EQ Preset set to: %d (Bass: %d, Treble: %d)\n", preset, bass, treble);
}

uint8_t getEQPreset() {
    return g_deviceState.eqPreset;
}

bool loadMP3File(const char* filepath) {
    if (!audioInitialized) return false;

    DEBUG_PRINTF("[AUDIO] Loading MP3: %s\n", filepath);

    bool success = audio.connecttoFS(SD, filepath);

    if (success) {
        switchAudioSource(SOURCE_SD_CARD);
        DEBUG_PRINTLN("[AUDIO] MP3 loaded successfully");
    } else {
        DEBUG_PRINTLN("[AUDIO] Failed to load MP3");
    }

    return success;
}

bool loadFLACFile(const char* filepath) {
    if (!audioInitialized) return false;

    DEBUG_PRINTF("[AUDIO] Loading FLAC: %s\n", filepath);

    bool success = audio.connecttoFS(SD, filepath);

    if (success) {
        switchAudioSource(SOURCE_SD_CARD);
        DEBUG_PRINTLN("[AUDIO] FLAC loaded successfully");
    } else {
        DEBUG_PRINTLN("[AUDIO] Failed to load FLAC");
    }

    return success;
}

bool loadWAVFile(const char* filepath) {
    if (!audioInitialized) return false;

    DEBUG_PRINTF("[AUDIO] Loading WAV: %s\n", filepath);

    bool success = audio.connecttoFS(SD, filepath);

    if (success) {
        switchAudioSource(SOURCE_SD_CARD);
        DEBUG_PRINTLN("[AUDIO] WAV loaded successfully");
    } else {
        DEBUG_PRINTLN("[AUDIO] Failed to load WAV");
    }

    return success;
}

bool connectToWebStream(const char* url) {
    if (!audioInitialized) return false;

    DEBUG_PRINTF("[AUDIO] Connecting to stream: %s\n", url);

    bool success = audio.connecttohost(url);

    if (success) {
        switchAudioSource(SOURCE_WEB_STREAM);
        DEBUG_PRINTLN("[AUDIO] Stream connected successfully");
    } else {
        DEBUG_PRINTLN("[AUDIO] Failed to connect to stream");
    }

    return success;
}

void disconnectWebStream() {
    if (audioInitialized) {
        audio.stopSong();
        DEBUG_PRINTLN("[AUDIO] Stream disconnected");
    }
}

bool isAudioPlaying() {
    return audioInitialized && audio.isRunning();
}

uint32_t getCurrentPosition() {
    return audioInitialized ? audio.getAudioCurrentTime() : 0;
}

uint32_t getCurrentDuration() {
    return audioInitialized ? audio.getAudioFileDuration() : 0;
}

// ============================================
// Audio Event Callbacks (ESP32-audioI2S)
// ============================================

void audio_info(const char *info) {
    DEBUG_PRINTF("[AUDIO INFO] %s\n", info);
}

void audio_id3data(const char *info) {
    DEBUG_PRINTF("[AUDIO ID3] %s\n", info);

    // Parse ID3 tags and update metadata
    // Format: "Title=Song Name" or "Artist=Artist Name"
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

void audio_eof_mp3(const char *info) {
    DEBUG_PRINTLN("[AUDIO] End of file");
    setPlaying(false);
}

void audio_bitrate(const char *info) {
    uint16_t bitrate = atoi(info);

    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.trackBitrate = bitrate;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTF("[AUDIO] Bitrate: %d kbps\n", bitrate);
}
