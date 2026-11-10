#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "../core/config.h"

// Initialize Wi-Fi manager
void initWiFiManager();

// Update Wi-Fi state (called from network task)
void updateWiFiManager();

// Wi-Fi connection management
bool connectToWiFi(const char* ssid, const char* password);
void disconnectWiFi();
bool isWiFiConnected();

// Access Point mode
void startAccessPoint();
void stopAccessPoint();
bool isAPMode();

// Wi-Fi scanning
int scanNetworks();
String getScannedNetwork(int index);
int8_t getScannedRSSI(int index);

// Network information
String getLocalIP();
String getSSID();
int8_t getRSSI();
String getMACAddress();

// Save/Load credentials
void saveWiFiCredentials(const char* ssid, const char* password);
bool loadWiFiCredentials(String& ssid, String& password);
void clearWiFiCredentials();

#endif // WIFI_MANAGER_H
