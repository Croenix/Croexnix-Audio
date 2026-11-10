#!/bin/bash
# Croenix Audio - Arduino IDE Conversion Script (Mac/Linux)
# This script reorganizes PlatformIO project for Arduino IDE

echo "================================================"
echo "Croenix Audio Firmware - Arduino IDE Converter"
echo "================================================"
echo

# Create Arduino sketch folder
ARDUINO_FOLDER="Croenix_Audio_Firmware_Arduino"
echo "Creating Arduino sketch folder: $ARDUINO_FOLDER"
mkdir -p "$ARDUINO_FOLDER"

# Copy main file and rename to .ino
echo "Copying main firmware file..."
cp "src/main.cpp" "$ARDUINO_FOLDER/Croenix_Audio_Firmware_Arduino.ino"

# Copy all header files from src folders
echo "Copying header files..."
find src -name "*.h" -exec cp {} "$ARDUINO_FOLDER/" \;

# Copy all cpp files from src folders
echo "Copying implementation files..."
find src -name "*.cpp" -exec cp {} "$ARDUINO_FOLDER/" \;

echo
echo "================================================"
echo "Conversion Complete!"
echo "================================================"
echo
echo "Arduino sketch folder created: $ARDUINO_FOLDER"
echo
echo "NEXT STEPS:"
echo "1. Open Arduino IDE"
echo "2. Install required libraries (see ARDUINO_IDE_FLASH_GUIDE.md)"
echo "3. Configure TFT_eSPI library"
echo "4. Open $ARDUINO_FOLDER/Croenix_Audio_Firmware_Arduino.ino"
echo "5. Select Board: ESP32 Dev Module"
echo "6. Click Upload"
echo
echo "For detailed instructions, see: ARDUINO_IDE_FLASH_GUIDE.md"
echo
