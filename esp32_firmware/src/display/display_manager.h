#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

// Initialize display with Zero-State pure black boot
void initDisplayManager();

// Non-blocking state machine display update
void updateDisplay();

// Screen rendering views
void showSplashScreen();
void showMainScreen();
void showHardwareDiagnosticsScreen();
void showProvisioningQRScreen();

// Custom graphical draw helpers
void drawModernProgressBar(int x, int y, int width, int height, float progress, uint16_t color, uint16_t bgColor);
void drawAudioVisualizer(int x, int y, int width, int height, bool isPlaying);
void drawVolumeHUD(uint8_t volume);

#endif // DISPLAY_MANAGER_H
