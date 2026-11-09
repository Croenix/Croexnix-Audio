#include "DisplayManager.h"
#include "../core/PlayerStateManager.h"
#include "../PinConfig.h"
#include "../Config.h"

DisplayManager::DisplayManager(PlayerStateManager* stateManager)
  : _stateManager(stateManager),
    _currentPage(ScreenPage::DASHBOARD),
    _lastRenderMs(0),
    _hardwareEnabled(false),
    _alertExpireMs(0) {
}

void DisplayManager::begin() {
  // Check if physical pins for SPI TFT are assigned
  if (PIN_TFT_CS >= 0 && PIN_TFT_DC >= 0 && PIN_TFT_RST >= 0) {
    _hardwareEnabled = true;
    CROENIX_LOG("DisplayManager", "Hardware display configured on CS=%d, DC=%d, RST=%d (1.8\" 128x160)",
                PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
  } else {
    _hardwareEnabled = false;
    CROENIX_LOG("DisplayManager", "TFT SPI display pins not assigned. Running in virtual/standby display mode.");
  }
}

void DisplayManager::update() {
  unsigned long now = millis();
  if (now - _lastRenderMs < 500) { // Controlled 2 Hz refresh rate to avoid starving audio tasks
    return;
  }
  _lastRenderMs = now;

  _renderCurrentPage();
}

void DisplayManager::setPage(ScreenPage page) {
  _currentPage = page;
  CROENIX_LOG("DisplayManager", "Switched to screen page %d", (int)page);
}

ScreenPage DisplayManager::getPage() {
  return _currentPage;
}

void DisplayManager::showTemporaryAlert(const String& alertText, unsigned long durationMs) {
  _alertMessage = alertText;
  _alertExpireMs = millis() + durationMs;
  _currentPage = ScreenPage::ERROR_ALERT;
}

void DisplayManager::_renderCurrentPage() {
  if (millis() < _alertExpireMs) {
    _renderAlert();
    return;
  }

  switch (_currentPage) {
    case ScreenPage::DASHBOARD:
      _renderDashboard();
      break;
    case ScreenPage::NOW_PLAYING:
      _renderNowPlaying();
      break;
    case ScreenPage::NETWORK_STATUS:
      _renderNetworkStatus();
      break;
    case ScreenPage::SYSTEM_DIAGNOSTICS:
      _renderSystemDiagnostics();
      break;
    default:
      _renderDashboard();
      break;
  }
}

void DisplayManager::_renderDashboard() {
  if (!_stateManager) return;
  // State is read cleanly from PlayerStateManager
  PlayerState st = _stateManager->getState();
  // In hardware implementation with TFT library, draws header, network status, volume bar, and source
}

void DisplayManager::_renderNowPlaying() {
  if (!_stateManager) return;
  PlayerState st = _stateManager->getState();
  // Draws track title, artist, elapsed/total time bar
}

void DisplayManager::_renderNetworkStatus() {
  if (!_stateManager) return;
  PlayerState st = _stateManager->getState();
  // Draws Hotspot IP/clients, Router IP/SSID/RSSI, and BLE connection status
}

void DisplayManager::_renderSystemDiagnostics() {
  if (!_stateManager) return;
  PlayerState st = _stateManager->getState();
  // Draws free heap, uptime, reset reason
}

void DisplayManager::_renderAlert() {
  // Draws modal alert box with _alertMessage
}

