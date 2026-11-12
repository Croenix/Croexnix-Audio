#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Initialize audio subsystem
void initAudioManager();

// Update audio pipeline (called from audio task)
void updateAudioPipeline();

// Audio control functions
void playAudio();
void pauseAudio();
void stopAudio();
void nextTrack();
void previousTrack();

// Volume control
void setMasterVolume(uint8_t volume);
uint8_t getMasterVolume();

// Source switching
bool switchAudioSource(AudioSource source);

// Equalizer presets
void setEQPreset(uint8_t preset);
uint8_t getEQPreset();
void setHardwareTone(int8_t bass, int8_t mid, int8_t treble);

// Codec-specific functions
bool loadMP3File(const char* filepath);
bool loadFLACFile(const char* filepath);
bool loadWAVFile(const char* filepath);

// Streaming
bool connectToWebStream(const char* url);
void disconnectWebStream();

// Status queries
bool isAudioPlaying();
uint32_t getCurrentPosition();
uint32_t getCurrentDuration();

// Hardware Audio Verification Chime
bool playBootChime();

#endif // AUDIO_MANAGER_H
