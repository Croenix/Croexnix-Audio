#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Initialize Rotary Encoder pins and interrupts
void initEncoderManager();

// Poll and process encoder state
void updateEncoderManager();

// Check if volume HUD overlay is currently active on screen
bool isVolumeOverlayActive();
uint8_t getVolumeOverlayValue();

#endif // ENCODER_MANAGER_H

