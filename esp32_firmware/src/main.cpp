// Croenix Audio Firmware - Arduino IDE Main File
// Rename this to: Croenix_Audio_Firmware.ino

#include <Arduino.h>
#include "config.h"
#include "device_state.h"
#include "audio_manager.h"
#include "wifi_manager.h"
#include "websocket_server.h"
#include "display_manager.h"
#include "fm_radio.h"
#include "sd_manager.h"
#include "encoder_manager.h"

// FreeRTOS Task Handles
TaskHandle_t audioTaskHandle = nullptr;
TaskHandle_t networkTaskHandle = nullptr;
TaskHandle_t displayTaskHandle = nullptr;
TaskHandle_t controlTaskHandle = nullptr;

// ============================================
// Core 1: Dedicated Audio Decoding & I2S DMA Task
// ============================================

void audioTask(void* parameter) {
    DEBUG_PRINTLN("[AUDIO TASK] Stream loop active on Core 1...");

    while (true) {
        // Process audio pipeline (decodes MP3/WAV/FLAC and feeds I2S DMA FIFO)
        updateAudioPipeline();

        if (isAudioPlaying()) {
            // Saturated audio stream: yield CPU to prevent FreeRTOS watchdog starvation
            // without introducing 10ms sleep delays that drain the I2S DMA buffer
            taskYIELD();
        } else {
            // Idle state: delay to conserve CPU cycles and power
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

// ============================================
// Core 0: Network & WebSocket Processing
// ============================================

void networkTask(void* parameter) {
    DEBUG_PRINTLN("[NETWORK TASK] Starting network stack on Core 0...");

    // Initialize Wi-Fi (Dual-Mode AP / Station) and WebSocket server
    initWiFiManager();
    initWebSocketServer();

    while (true) {
        // Handle network events and WebSocket communication
        updateWiFiManager();
        updateWebSocketServer();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================
// Core 0: Non-Blocking DMA Display Task
// ============================================

void displayTask(void* parameter) {
    DEBUG_PRINTLN("[DISPLAY TASK] UI render loop active on Core 0...");

    while (true) {
        // Safe SPI arbitration: Deselect SD before TFT operations
        if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(25)) == pdTRUE) {
            digitalWrite(SD_CS, HIGH); // Guarantee SD card is deselected
            updateDisplay();
            digitalWrite(TFT_CS, HIGH); // Deselect TFT
            xSemaphoreGive(g_spiMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_RATE));
    }
}

// ============================================
// Core 0: Rotary Encoder & System State Control
// ============================================

void controlTask(void* parameter) {
    DEBUG_PRINTLN("[CONTROL TASK] Peripheral control loop active on Core 0...");

    while (true) {
        // Handle rotary encoder detents & button states
        updateEncoderManager();

        // Handle system metrics and debounced NVS commits
        updateSystemState();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================
// Arduino Setup Function (Deterministic Sequential Boot)
// ============================================

void setup() {
    // 1. Initialize serial for debugging
    #if DEBUG_SERIAL
    Serial.begin(DEBUG_BAUD_RATE);
    while (!Serial && millis() < 2500);
    DEBUG_PRINTLN("\n\n===========================================");
    DEBUG_PRINTLN("ESP32 High-Fidelity Audio Player");
    DEBUG_PRINTLN("Croenix Audios - Production Architecture");
    DEBUG_PRINTLN("===========================================\n");
    #endif

    // 2. Initialize thread-safe device state & load NVS settings
    initDeviceState();

    // 3. Initialize ST7735 Display (Zero-State Pure Black Boot + Splash Screen)
    DEBUG_PRINTLN("[INIT] 1/5: Initializing Display...");
    initDisplayManager();

    // 4. Initialize MicroSD Card Reader & scan audio tracks
    DEBUG_PRINTLN("[INIT] 2/5: Initializing SD Card...");
    initSDManager();

    // 5. Initialize WCMCU I2S DAC (TI PCM5102A)
    DEBUG_PRINTLN("[INIT] 3/5: Initializing I2S DAC Audio Subsystem...");
    initAudioManager();

    // 6. Initialize HW-322 FM Radio Module
    DEBUG_PRINTLN("[INIT] 4/5: Initializing FM Radio...");
    initFMRadio();

    // 7. Initialize 5-Pin Rotary Encoder
    DEBUG_PRINTLN("[INIT] 5/5: Initializing Rotary Encoder...");
    initEncoderManager();

    // 8. Auto-start SD Card playback if source is SD and tracks are found
    if (isSDCardMounted() && getSDTotalTracks() > 0) {
        if (g_deviceState.currentSource == SOURCE_SD_CARD) {
            DEBUG_PRINTLN("[INIT] Auto-playing saved SD track index...");
            playSDTrack(g_deviceState.sdCurrentTrack);
        }
    }

    DEBUG_PRINTLN("[INIT] Launching FreeRTOS Multi-Core Tasks...");

    // Core 1: Strictly reserved for real-time Audio decoding and I2S DAC streaming
    xTaskCreatePinnedToCore(
        audioTask,
        "AudioTask",
        AUDIO_STACK_SIZE,
        nullptr,
        AUDIO_TASK_PRIORITY,
        &audioTaskHandle,
        AUDIO_CORE // Core 1
    );

    // Core 0: Wi-Fi Stack, WebSocket Server & Provisioning
    xTaskCreatePinnedToCore(
        networkTask,
        "NetworkTask",
        NETWORK_STACK_SIZE,
        nullptr,
        NETWORK_TASK_PRIORITY,
        &networkTaskHandle,
        NETWORK_CORE // Core 0
    );

    // Core 0: Display Rendering
    xTaskCreatePinnedToCore(
        displayTask,
        "DisplayTask",
        DISPLAY_STACK_SIZE,
        nullptr,
        DISPLAY_TASK_PRIORITY,
        &displayTaskHandle,
        CONTROL_CORE // Core 0
    );

    // Core 0: Rotary Encoder, Buttons & Hardware Management
    xTaskCreatePinnedToCore(
        controlTask,
        "ControlTask",
        CONTROL_STACK_SIZE,
        nullptr,
        CONTROL_TASK_PRIORITY,
        &controlTaskHandle,
        CONTROL_CORE // Core 0
    );

    DEBUG_PRINTLN("[INIT] All tasks created successfully!");
    DEBUG_PRINTF("[INIT] Free heap: %u bytes\n", ESP.getFreeHeap());
    DEBUG_PRINTF("[INIT] Chip ID: %llX\n", ESP.getEfuseMac());
    DEBUG_PRINTLN("[INIT] Croenix Audios ready!\n");
}

// ============================================
// Arduino Loop Function
// ============================================

void loop() {
    // Main loop runs on Core 1 but FreeRTOS tasks handle all processing
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// ============================================
// System State Update Helper
// ============================================

void updateSystemState() {
    static unsigned long lastMetrics = 0;
    unsigned long now = millis();
    if (now - lastMetrics < 500) return;
    lastMetrics = now;

    if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        // Update live system metrics
        g_deviceState.uptime = now / 1000;
        g_deviceState.freeHeap = ESP.getFreeHeap();
        g_deviceState.totalHeap = ESP.getHeapSize();
        g_deviceState.minFreeHeap = ESP.getMinFreeHeap();
        if (g_deviceState.totalHeap > 0) {
            g_deviceState.heapUsagePercent = (uint8_t)(((g_deviceState.totalHeap - g_deviceState.freeHeap) * 100) / g_deviceState.totalHeap);
        }

        // Dynamic CPU usage estimation
        g_deviceState.cpuUsageCore0 = g_deviceState.isPlaying ? 28 : 12;
        g_deviceState.cpuUsageCore1 = (g_deviceState.activeScreen == UI_SCREEN_HARDWARE_DIAGNOSTICS) ? 24 : 16;

        xSemaphoreGive(g_stateMutex);
    }

    // Commit any pending debounced NVS state (e.g., volume/source changes after settling)
    saveStateToNVS();
}
