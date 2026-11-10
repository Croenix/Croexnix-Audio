@echo off
REM Croenix Audio - Arduino IDE Conversion Script
REM This script reorganizes PlatformIO project for Arduino IDE

echo ================================================
echo Croenix Audio Firmware - Arduino IDE Converter
echo ================================================
echo.

REM Create Arduino sketch folder
set ARDUINO_FOLDER=Croenix_Audio_Firmware_Arduino
echo Creating Arduino sketch folder: %ARDUINO_FOLDER%
if not exist "%ARDUINO_FOLDER%" mkdir "%ARDUINO_FOLDER%"

REM Copy main file and rename to .ino
echo Copying main firmware file...
copy "src\main.cpp" "%ARDUINO_FOLDER%\Croenix_Audio_Firmware_Arduino.ino" >nul

REM Copy all header files from src folders
echo Copying header files...
copy "src\core\*.h" "%ARDUINO_FOLDER%\" >nul
copy "src\audio\*.h" "%ARDUINO_FOLDER%\" >nul
copy "src\network\*.h" "%ARDUINO_FOLDER%\" >nul
copy "src\display\*.h" "%ARDUINO_FOLDER%\" >nul
copy "src\peripherals\*.h" "%ARDUINO_FOLDER%\" >nul

REM Copy all cpp files from src folders
echo Copying implementation files...
copy "src\core\*.cpp" "%ARDUINO_FOLDER%\" >nul
copy "src\audio\*.cpp" "%ARDUINO_FOLDER%\" >nul
copy "src\network\*.cpp" "%ARDUINO_FOLDER%\" >nul
copy "src\display\*.cpp" "%ARDUINO_FOLDER%\" >nul
copy "src\peripherals\*.cpp" "%ARDUINO_FOLDER%\" >nul

echo.
echo ================================================
echo Conversion Complete!
echo ================================================
echo.
echo Arduino sketch folder created: %ARDUINO_FOLDER%
echo.
echo NEXT STEPS:
echo 1. Open Arduino IDE
echo 2. Install required libraries (see ARDUINO_IDE_FLASH_GUIDE.md)
echo 3. Configure TFT_eSPI library
echo 4. Open %ARDUINO_FOLDER%\Croenix_Audio_Firmware_Arduino.ino
echo 5. Select Board: ESP32 Dev Module
echo 6. Click Upload
echo.
echo For detailed instructions, see: ARDUINO_IDE_FLASH_GUIDE.md
echo.
pause
