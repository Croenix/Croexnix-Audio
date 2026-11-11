#include "audio_manager.h"
#include "Audio.h"
#include "device_state.h"
#include "sd_manager.h"

// Audio library instance (ESP32-audioI2S)
Audio audio;

// Current playback state
static bool audioInitialized = false;
static AudioSource activeSource = SOURCE_NONE;

void initAudioManager() {
    DEBUG_PRINTLN("[AUDIO] Initializing I2S audio subsystem (WCMCU / TI PCM5102A)...");

    // Configure I2S pins: BCK=GPIO26, LRC=GPIO25, DOUT=GPIO27
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

    // Set initial volume (0-21 mapped from 0-100)
    uint8_t mappedVol = map(g_deviceState.volume, 0, 100, 0, 21);
    audio.setVolume(mappedVol);

    // Configure audio quality
    audio.forceMono(false);  // High-Fidelity Stereo
    audio.setTone(0, 0, 0);  // Flat EQ by default

    audioInitialized = true;
    activeSource = g_deviceState.currentSource;

    DEBUG_PRINTLN("[AUDIO] I2S initialized successfully");
    DEBUG_PRINTF("[AUDIO] BCLK=%d, LRC=%d, DOUT=%d, Initial Volume=%d/21\n",
                 I2S_BCLK, I2S_LRC, I2S_DOUT, mappedVol);
}

void updateAudioPipeline() {
    if (!audioInitialized) return;

    // Pump audio decoding & I2S DMA streaming
    if (activeSource == SOURCE_SD_CARD) {
        if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(15)) == pdTRUE) {
            digitalWrite(TFT_CS, HIGH); // Ensure TFT is not listening to SPI
            for (int i = 0; i < 4; i++) {
                audio.loop();
                if (!audio.isRunning()) break;
            }
            digitalWrite(SD_CS, HIGH); // Deselect SD
            xSemaphoreGive(g_spiMutex);
        } else {
            for (int i = 0; i < 2; i++) {
                audio.loop();
                if (!audio.isRunning()) break;
            }
        }
    } else {
        for (int i = 0; i < 4; i++) {
            audio.loop();
            if (!audio.isRunning()) break;
        }
    }

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
    if (!audioInitialized) return;

    if (activeSource == SOURCE_SD_CARD) {
        if (!audio.isRunning()) {
            if (getSDTotalTracks() > 0) {
                playSDTrack(g_deviceState.sdCurrentTrack);
            } else {
                audio.pauseResume();
                setPlaying(true);
            }
        }
    } else {
        if (!audio.isRunning()) {
            audio.pauseResume();
            setPlaying(true);
            DEBUG_PRINTLN("[AUDIO] Playback started");
        }
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
    DEBUG_PRINTLN("[AUDIO] Next track requested");
    if (activeSource == SOURCE_SD_CARD) {
        playNextSDTrack();
    }
}

void previousTrack() {
    DEBUG_PRINTLN("[AUDIO] Previous track requested");
    if (activeSource == SOURCE_SD_CARD) {
        playPrevSDTrack();
    }
}

void setMasterVolume(uint8_t volume) {
    if (volume > 100) volume = 100;

    // Convert 0-100 to 0-21 (ESP32-audioI2S range)
    uint8_t mappedVolume = map(volume, 0, 100, 0, 21);

    if (audioInitialized) {
        audio.setVolume(mappedVolume);
    }

    setVolume(volume);
    DEBUG_PRINTF("[AUDIO] Volume set to %d%% (I2S Gain: %d/21)\n", volume, mappedVolume);
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

    if (source == SOURCE_SD_CARD) {
        if (!isSDCardMounted()) {
            mountSDCard();
        }
        if (isSDCardMounted() && getSDTotalTracks() > 0) {
            playSDTrack(g_deviceState.sdCurrentTrack);
        }
    }

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

    bool success = false;
    if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(150)) == pdTRUE) {
        digitalWrite(TFT_CS, HIGH);
        success = audio.connecttoFS(SD, filepath);
        digitalWrite(SD_CS, HIGH);
        xSemaphoreGive(g_spiMutex);
    } else {
        success = audio.connecttoFS(SD, filepath);
    }

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

    bool success = false;
    if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(150)) == pdTRUE) {
        digitalWrite(TFT_CS, HIGH);
        success = audio.connecttoFS(SD, filepath);
        digitalWrite(SD_CS, HIGH);
        xSemaphoreGive(g_spiMutex);
    } else {
        success = audio.connecttoFS(SD, filepath);
    }

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

    bool success = false;
    if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(150)) == pdTRUE) {
        digitalWrite(TFT_CS, HIGH);
        success = audio.connecttoFS(SD, filepath);
        digitalWrite(SD_CS, HIGH);
        xSemaphoreGive(g_spiMutex);
    } else {
        success = audio.connecttoFS(SD, filepath);
    }

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
    DEBUG_PRINTLN("[AUDIO] End of track reached - advancing to next track");
    if (activeSource == SOURCE_SD_CARD) {
        playNextSDTrack();
    } else {
        setPlaying(false);
    }
}

void audio_bitrate(const char *info) {
    uint16_t bitrate = atoi(info);

    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        g_deviceState.trackBitrate = bitrate;
        xSemaphoreGive(g_stateMutex);
    }

    DEBUG_PRINTF("[AUDIO] Bitrate: %d kbps\n", bitrate);
}
