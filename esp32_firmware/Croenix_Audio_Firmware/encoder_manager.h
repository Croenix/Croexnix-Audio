#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include <Arduino.h>
#include "config.h"

// Initialize Rotary Encoder pins and interrupt-driven state machine
void initEncoderManager();

// Poll and process encoder state events (called from Control task)
void updateEncoderManager();

// Trigger / Toggle Hardware Diagnostics mode
void toggleHardwareDiagnostics();

// Check if a volume overlay is active
bool isVolumeOverlayActive();
uint8_t getVolumeOverlayValue();

#endif // ENCODER_MANAGER_H
