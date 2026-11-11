// =========================================================================
// CROENIX AUDIO FIRMWARE v3.0 - OPTIMIZED REBUILD
// High-Performance, Low-Load Architecture for ESP32-WROOM-32
// Real-time Mobile App Integration with Smooth Performance
// =========================================================================

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

// -------------------------------------------------------------------------
// FreeRTOS Task Handles (Optimized Multi-Core Architecture)
// -------------------------------------------------------------------------

TaskHandle_t audioTaskHandle = nullptr;
TaskHandle_t networkTaskHandle = nullptr;
TaskHandle_t displayTaskHandle = nullptr;
TaskHandle_t controlTaskHandle = nullptr;

// Performance monitoring (disabled in production)
static uint32_t lastHeapReport = 0;

// =========================================================================
// CORE 1: DEDICATED AUDIO TASK (Highest Priority)
// Real-time audio decoding & I²S DMA streaming without interference
// =========================================================================

void audioTask(void* parameter) {
    DEBUG_PRINTLN("[TASK:AUDIO] Audio task started on Core 1");
    DEBUG_PRINTF("[TASK:AUDIO] Priority: %d, Stack: %d bytes\n",
                 AUDIO_TASK_PRIORITY, AUDIO_STACK_SIZE);

    // Small delay to allow other systems to initialize
    vTaskDelay(pdMS_TO_TICKS(100));

    while (true) {
        // Process audio pipeline with optimized loop iterations
        updateAudioPipeline();

        // Adaptive yielding based on playback state
        if (isAudioPlaying()) {
            // Active playback: Yield CPU but don't sleep (prevent buffer underruns)
            taskYIELD();
        } else {
            // Idle: Sleep to conserve CPU
            vTaskDelay(pdMS_TO_TICKS(AUDIO_UPDATE_INTERVAL));
        }
    }
}

// =========================================================================
// CORE 0: NETWORK TASK (High Priority)
// Wi-Fi, WebSocket, mDNS - Real-time mobile app communication
// =========================================================================

void networkTask(void* parameter) {
    DEBUG_PRINTLN("[TASK:NETWORK] Network task started on Core 0");
    DEBUG_PRINTF("[TASK:NETWORK] Priority: %d, Stack: %d bytes\n",
                 NETWORK_TASK_PRIORITY, NETWORK_STACK_SIZE);

    // Initialize network components
    vTaskDelay(pdMS_TO_TICKS(200));

    initWiFiManager();
    vTaskDelay(pdMS_TO_TICKS(100));

    initWebSocketServer();

    DEBUG_PRINTLN("[TASK:NETWORK] Network stack ready");

    while (true) {
        // Update Wi-Fi state machine
        updateWiFiManager();

        // Handle WebSocket events and client messages
        updateWebSocketServer();

        // Moderate sleep for responsive app control
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// =========================================================================
// CORE 0: DISPLAY TASK (Medium Priority)
// Non-blocking SPI display updates with mutex protection
// =========================================================================

void displayTask(void* parameter) {
    DEBUG_PRINTLN("[TASK:DISPLAY] Display task started on Core 0");
    DEBUG_PRINTF("[TASK:DISPLAY] Priority: %d, Stack: %d bytes\n",
                 DISPLAY_TASK_PRIORITY, DISPLAY_STACK_SIZE);

    // Wait for display initialization
    vTaskDelay(pdMS_TO_TICKS(300));

    while (true) {
        // SPI bus arbitration with mutex
        if (g_spiMutex && xSemaphoreTake(g_spiMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            digitalWrite(SD_CS, HIGH);   // Ensure SD inactive

            updateDisplay();

            digitalWrite(TFT_CS, HIGH);  // Deselect display
            xSemaphoreGive(g_spiMutex);
        }

        // Display update rate (20 FPS for low CPU usage)
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_RATE));
    }
}

// =========================================================================
// CORE 0: CONTROL TASK (Medium Priority)
// Encoder input, button handling, system state management
// =========================================================================

void controlTask(void* parameter) {
    DEBUG_PRINTLN("[TASK:CONTROL] Control task started on Core 0");
    DEBUG_PRINTF("[TASK:CONTROL] Priority: %d, Stack: %d bytes\n",
                 CONTROL_TASK_PRIORITY, CONTROL_STACK_SIZE);

    vTaskDelay(pdMS_TO_TICKS(250));

    while (true) {
        // Handle rotary encoder events
        updateEncoderManager();

        // Update system state (FM tune window expiration, NVS commits, etc.)
        checkFMTuneExpiration();

        // Fast response for user input
        vTaskDelay(pdMS_TO_TICKS(10));

#if ENABLE_MEMORY_MONITOR
        // Periodic heap monitoring (debug only)
        uint32_t now = millis();
        if (now - lastHeapReport > 30000) {  // Every 30 seconds
            DEBUG_PRINTF("[SYSTEM] Free heap: %d bytes\n", ESP.getFreeHeap());
            lastHeapReport = now;
        }
#endif
    }
}

// =========================================================================
// ARDUINO SETUP - SYSTEM INITIALIZATION
// =========================================================================

void setup() {
    // Initialize serial debug output
#if DEBUG_SERIAL
    Serial.begin(DEBUG_BAUD_RATE);
    delay(500);
    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║           CROENIX AUDIO PLAYER v3.0 - OPTIMIZED          ║");
    Serial.println("║         High-Performance Low-Load Architecture            ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
    DEBUG_PRINTF("[SYSTEM] Firmware: %s\n", FIRMWARE_VERSION);
    DEBUG_PRINTF("[SYSTEM] Device: %s\n", DEVICE_NAME);
    DEBUG_PRINTF("[SYSTEM] Chip: ESP32, Cores: 2, CPU: %d MHz\n", getCpuFrequencyMhz());
    DEBUG_PRINTF("[SYSTEM] Flash: %d MB, Free heap: %d bytes\n",
                 ESP.getFlashChipSize() / (1024 * 1024), ESP.getFreeHeap());
    Serial.println("════════════════════════════════════════════════════════════");
#endif

    // Initialize device state and NVS
    DEBUG_PRINTLN("[INIT] Initializing device state...");
    initDeviceState();

    // Initialize audio subsystem (I²S DAC)
    DEBUG_PRINTLN("[INIT] Initializing audio manager...");
    initAudioManager();

    // Initialize SD card (non-blocking, will retry if failed)
    DEBUG_PRINTLN("[INIT] Initializing SD card manager...");
    if (!initSDManager()) {
        DEBUG_PRINTLN("[INIT] ⚠️  SD card mount failed - will retry in background");
    }

    // Initialize display (ST7735 TFT)
    DEBUG_PRINTLN("[INIT] Initializing display manager...");
    initDisplayManager();

    // Initialize encoder
    DEBUG_PRINTLN("[INIT] Initializing encoder...");
    initEncoderManager();

    // Initialize FM radio (optional)
#if ENABLE_FM_RADIO
    DEBUG_PRINTLN("[INIT] Initializing FM radio...");
    initFMRadio();
#endif

    // Show boot splash (display manager handles this)
    DEBUG_PRINTLN("[INIT] Displaying boot splash...");
    setUIScreen(UI_SCREEN_BOOT_SPLASH);
    delay(BOOT_DELAY_MS);
    setUIScreen(UI_SCREEN_HOME);

    DEBUG_PRINTLN("════════════════════════════════════════════════════════════");
    DEBUG_PRINTLN("[INIT] Creating FreeRTOS tasks...");

    // Create Core 1 task: Audio processing (highest priority)
    xTaskCreatePinnedToCore(
        audioTask,
        "AudioTask",
        AUDIO_STACK_SIZE,
        nullptr,
        AUDIO_TASK_PRIORITY,
        &audioTaskHandle,
        AUDIO_CORE
    );
    DEBUG_PRINTLN("[INIT] ✓ Audio task created on Core 1");

    // Create Core 0 tasks: Network, Display, Control
    xTaskCreatePinnedToCore(
        networkTask,
        "NetworkTask",
        NETWORK_STACK_SIZE,
        nullptr,
        NETWORK_TASK_PRIORITY,
        &networkTaskHandle,
        NETWORK_CORE
    );
    DEBUG_PRINTLN("[INIT] ✓ Network task created on Core 0");

    xTaskCreatePinnedToCore(
        displayTask,
        "DisplayTask",
        DISPLAY_STACK_SIZE,
        nullptr,
        DISPLAY_TASK_PRIORITY,
        &displayTaskHandle,
        CONTROL_CORE
    );
    DEBUG_PRINTLN("[INIT] ✓ Display task created on Core 0");

    xTaskCreatePinnedToCore(
        controlTask,
        "ControlTask",
        CONTROL_STACK_SIZE,
        nullptr,
        CONTROL_TASK_PRIORITY,
        &controlTaskHandle,
        CONTROL_CORE
    );
    DEBUG_PRINTLN("[INIT] ✓ Control task created on Core 0");

    DEBUG_PRINTLN("════════════════════════════════════════════════════════════");
    DEBUG_PRINTLN("[SYSTEM] Initialization complete - All tasks running");
    DEBUG_PRINTLN("[SYSTEM] Mobile app: Connect to Wi-Fi and open WebSocket");
    DEBUG_PRINTF("[SYSTEM] WebSocket URL: ws://%s:%d\n", AP_IP, WS_PORT);
    DEBUG_PRINTLN("════════════════════════════════════════════════════════════");
    DEBUG_PRINTLN();

    // Set initial source from NVS
    if (g_deviceState.currentSource == SOURCE_SD_CARD && isSDCardMounted()) {
        if (getSDTotalTracks() > 0) {
            DEBUG_PRINTLN("[SYSTEM] Auto-starting SD playback...");
            playSDTrack(g_deviceState.sdCurrentTrack);
        }
    }
}

// =========================================================================
// ARDUINO LOOP - MINIMAL OVERHEAD
// FreeRTOS scheduler handles all tasks, loop just yields
// =========================================================================

void loop() {
    // All work is done in FreeRTOS tasks
    // This loop just prevents watchdog timeout
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Optional: Handle critical system events here if needed
    // (Currently not used - all logic is in tasks)
}

// =========================================================================
// END OF FIRMWARE
// =========================================================================
