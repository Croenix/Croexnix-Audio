#ifndef CROENIX_BLE_MANAGER_H
#define CROENIX_BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

class PlayerStateManager;
class AuthManager;
class CroenixWiFiManager;
class SettingsManager;

class CroenixBLEManager : public BLEServerCallbacks {
public:
  CroenixBLEManager(PlayerStateManager* stateManager,
                    AuthManager* authManager,
                    CroenixWiFiManager* wifiManager,
                    SettingsManager* settingsManager);

  void begin();
  void update();
  void stop();

  bool isConnected();
  void notifyNetworkStatus(const String& statusJson);
  void notifyEvent(const String& eventJson);

  // BLEServerCallbacks
  void onConnect(BLEServer* pServer) override;
  void onDisconnect(BLEServer* pServer) override;

  void handleProvisioningWrite(const String& value);
  String handlePairingRead();
  void handlePairingWrite(const String& value);

private:
  PlayerStateManager* _stateManager;
  AuthManager* _authManager;
  CroenixWiFiManager* _wifiManager;
  SettingsManager* _settingsManager;

  BLEServer* _pServer;
  BLEService* _pService;
  BLECharacteristic* _pCharDeviceInfo;
  BLECharacteristic* _pCharProvisioning;
  BLECharacteristic* _pCharNetworkStatus;
  BLECharacteristic* _pCharPairing;
  BLECharacteristic* _pCharEvents;

  bool _deviceConnected;
  bool _oldDeviceConnected;
  bool _initialized;

  void _setupServiceAndCharacteristics();
};

#endif // CROENIX_BLE_MANAGER_H

