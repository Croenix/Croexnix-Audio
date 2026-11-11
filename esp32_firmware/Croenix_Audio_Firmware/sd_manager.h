#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "config.h"

// SD Card Lifecycle & Bus Arbitration
bool initSDManager();
bool mountSDCard();
bool checkSDIntegrity();

// SD Card Status & Capacity
bool isSDCardMounted();
uint64_t getSDCardSize();
uint64_t getSDCardUsed();
uint16_t getSDTotalTracks();

// Track Playback & Navigation
bool playSDTrack(int index);
bool playNextSDTrack();
bool playPrevSDTrack();
const char* getSDCurrentTrackName();
const char* getSDTrackPath(int index);
const char* getSDTrackName(int index);

// File & Playlist Management
void scanSDAudioFiles();
void listSDFiles(const char* path, JsonArray& filesArray);
bool fileExists(const char* path);
String readTextFile(const char* path);
void createPlaylist(const char* name);
void addToPlaylist(const char* playlistName, const char* filepath);
void loadPlaylist(const char* name);

#endif // SD_MANAGER_H
