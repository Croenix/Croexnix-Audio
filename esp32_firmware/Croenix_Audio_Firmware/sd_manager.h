#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "config.h"

// Initialize SD card
void initSDManager();

// Hardware Diagnostics Integrity Check
bool checkSDIntegrity();

// SD card status
bool isSDCardMounted();
uint64_t getSDCardSize();
uint64_t getSDCardUsed();

// File operations
void listSDFiles(const char* path, JsonArray& filesArray);
bool fileExists(const char* path);
String readTextFile(const char* path);

// Playlist management
void createPlaylist(const char* name);
void addToPlaylist(const char* playlistName, const char* filepath);
void loadPlaylist(const char* name);

#endif // SD_MANAGER_H
