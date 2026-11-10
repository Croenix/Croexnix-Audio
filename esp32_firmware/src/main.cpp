#include <Arduino.h>
#include "core/config.h"
#include "core/device_state.h"
#include "audio/audio_manager.h"
#include "network/wifi_manager.h"
#include "network/websocket_server.h"
#include "display/display_manager.h"
#include "peripherals/fm_radio.h"
#include "peripherals/sd_manager.h"

// FreeRTOS Task Handles
TaskHandle_t audioTaskHandle = nullptr;
TaskHandle_t networkTaskHandle = nullptr;
TaskHandle_t displayTaskHandle = nullptr;
TaskHandle_t controlTaskHandle = nullptr;

// ============================================
// Core 0: Audio & Network Processing
// ============================================

void audioTask(void* parameter) {
    DEBUG_PRINTLN("[AUDIO TASK] Starting on Core 0...");

    // Initialize audio subsystem
    initAudioManager();

    while (true) {
        // Process audio pipeline
        updateAudioPipeline();

        // Allow other tasks to run
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void networkTask(void* parameter) {
    DEBUG_PRINTLN("[NETWORK TASK] Starting on Core 0...");

    // Initialize Wi-Fi and WebSocket server
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
// Core 1: Display & Control
// ============================================

void displayTask(void* parameter) {
    DEBUG_PRINTLN("[DISPLAY TASK] Starting on Core 1...");

    // Initialize TFT display
    initDisplayManager();

    while (true) {
        // Update display with current state
        updateDisplay();

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_RATE));
    }
}

void controlTask(void* parameter) {
    DEBUG_PRINTLN("[CONTROL TASK] Starting on Core 1...");

    // Initialize peripherals
    initFMRadio();
    initSDManager();

    while (true) {
        // Handle peripheral control and state updates
        updateSystemState();

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================
// System Initialization
// ============================================

void setup() {
    // Initialize serial for debugging
    #if DEBUG_SERIAL
    Serial.begin(DEBUG_BAUD_RATE);
    while (!Serial && millis() < 3000);
    DEBUG_PRINTLN("\n\n===========================================");
    DEBUG_PRINTLN("ESP32 High-Fidelity Audio Player");
    DEBUG_PRINTLN("===========================================\n");
    #endif

    // Initialize global device state
    initDeviceState();

    DEBUG_PRINTLN("[INIT] Creating FreeRTOS tasks...");

    // Create Core 0 tasks (Audio & Network)
    xTaskCreatePinnedToCore(
        audioTask,
        "AudioTask",
        AUDIO_STACK_SIZE,
        nullptr,
        AUDIO_TASK_PRIORITY,
        &audioTaskHandle,
        AUDIO_CORE
    );

    xTaskCreatePinnedToCore(
        networkTask,
        "NetworkTask",
        NETWORK_STACK_SIZE,
        nullptr,
        NETWORK_TASK_PRIORITY,
        &networkTaskHandle,
        AUDIO_CORE
    );

    // Create Core 1 tasks (Display & Control)
    xTaskCreatePinnedToCore(
        displayTask,
        "DisplayTask",
        DISPLAY_STACK_SIZE,
        nullptr,
        DISPLAY_TASK_PRIORITY,
        &displayTaskHandle,
        CONTROL_CORE
    );

    xTaskCreatePinnedToCore(
        controlTask,
        "ControlTask",
        CONTROL_STACK_SIZE,
        nullptr,
        CONTROL_TASK_PRIORITY,
        &controlTaskHandle,
        CONTROL_CORE
    );

    DEBUG_PRINTLN("[INIT] All tasks created successfully");
    DEBUG_PRINTF("[INIT] Free heap: %d bytes\n", ESP.getFreeHeap());
    DEBUG_PRINTF("[INIT] Chip ID: %llX\n", ESP.getEfuseMac());
}

void loop() {
    // Main loop runs on Core 1 but tasks handle everything
    // Keep this minimal to avoid blocking
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// ============================================
// System State Update Helper
// ============================================

void updateSystemState() {
    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY) == pdTRUE) {
        // Update system metrics
        g_deviceState.uptime = millis() / 1000;
        g_deviceState.freeHeap = ESP.getFreeHeap();

        // Get CPU usage for both cores (approximate)
        g_deviceState.cpuUsageCore0 = 100 - (uxTaskGetSystemState(nullptr, 0, nullptr) / 10);
        g_deviceState.cpuUsageCore1 = 100 - (uxTaskGetSystemState(nullptr, 0, nullptr) / 10);

        xSemaphoreGive(g_stateMutex);
    }
}
