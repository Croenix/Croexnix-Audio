#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

// Initialize display
void initDisplayManager();

// Update display with current state
void updateDisplay();

// Display screens
void showSplashScreen();
void showMainScreen();
void showSourceScreen();
void showVolumeScreen();
void showNetworkScreen();

// Helper functions
void drawProgressBar(int x, int y, int width, int height, float progress);
void drawVUMeter(int x, int y, int width, int height, uint8_t level);

#endif // DISPLAY_MANAGER_H
