#ifndef FM_RADIO_H
#define FM_RADIO_H

#include <Arduino.h>
#include "config.h"

// Initialize FM radio module (HW-322 / Si4703 / TEA5767)
void initFMRadio();

// Hardware Diagnostics Integrity Check
bool checkFMIntegrity();

// FM control functions
void setFMFrequency(float frequency);
float getFMFrequency();

void seekFMUp();
void seekFMDown();
void scanFMStations();

// FM status
uint8_t getFMSignalStrength();
String getFMStationName();

// Presets
void saveFMPreset(uint8_t slot, float frequency);
float loadFMPreset(uint8_t slot);

#endif // FM_RADIO_H
