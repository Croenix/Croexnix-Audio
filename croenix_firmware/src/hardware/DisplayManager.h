#ifndef CROENIX_DISPLAY_MANAGER_H
#define CROENIX_DISPLAY_MANAGER_H

#include <Arduino.h>

class PlayerStateManager;

enum class ScreenPage {
  SPLASH,
  DASHBOARD,
  NOW_PLAYING,
  NETWORK_STATUS,
  SYSTEM_DIAGNOSTICS,
  ERROR_ALERT
};

class DisplayManager {
public:
  DisplayManager(PlayerStateManager* stateManager);
  void begin();
  void update();

  void setPage(ScreenPage page);
  ScreenPage getPage();
  void showTemporaryAlert(const String& alertText, unsigned long durationMs = 3000);

private:
  PlayerStateManager* _stateManager;
  ScreenPage _currentPage;
  unsigned long _lastRenderMs;
  bool _hardwareEnabled;

  String _alertMessage;
  unsigned long _alertExpireMs;

  void _renderCurrentPage();
  void _renderDashboard();
  void _renderNowPlaying();
  void _renderNetworkStatus();
  void _renderSystemDiagnostics();
  void _renderAlert();
};

#endif // CROENIX_DISPLAY_MANAGER_H

