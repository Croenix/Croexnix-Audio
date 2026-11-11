#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

// Initialize display and run animated boot sequence
void initDisplayManager();

// Update display with non-blocking state machine
void updateDisplay();

// Screen views
void showSplashScreen();
void showMainScreen();
void showHardwareDiagnosticsScreen();

// Custom graphical draw helpers
void drawModernProgressBar(int x, int y, int width, int height, float progress, uint16_t color, uint16_t bgColor);
void drawAudioVisualizer(int x, int y, int width, int height, bool isPlaying);
void drawVolumeHUD(uint8_t volume);

#endif // DISPLAY_MANAGER_H
