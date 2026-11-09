#include "CroenixBLEManager.h"
#include "../core/PlayerStateManager.h"
#include "../storage/SettingsManager.h"
#include "AuthManager.h"
#include "CroenixWiFiManager.h"
#include "../Config.h"
#include <ArduinoJson.h>

class ProvisioningCallbacks : public BLECharacteristicCallbacks {
public:
  ProvisioningCallbacks(CroenixBLEManager* manager) : _manager(manager) {}
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String value = String(pCharacteristic->getValue().c_str());
    if (value.length() > 0) {
      _manager->handleProvisioningWrite(value);
    }
  }
private:
  CroenixBLEManager* _manager;
};

class PairingCallbacks : public BLECharacteristicCallbacks {
public:
  PairingCallbacks(CroenixBLEManager* manager) : _manager(manager) {}
  void onRead(BLECharacteristic* pCharacteristic) override {
    String token = _manager->handlePairingRead();
    pCharacteristic->setValue(token.c_str());
  }
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String value = String(pCharacteristic->getValue().c_str());
    _manager->handlePairingWrite(value);
  }
private:
  CroenixBLEManager* _manager;
};

CroenixBLEManager::CroenixBLEManager(PlayerStateManager* stateManager,
                                     AuthManager* authManager,
                                     CroenixWiFiManager* wifiManager,
                                     SettingsManager* settingsManager)
  : _stateManager(stateManager),
    _authManager(authManager),
    _wifiManager(wifiManager),
    _settingsManager(settingsManager),
    _pServer(nullptr),
    _pService(nullptr),
    _pCharDeviceInfo(nullptr),
    _pCharProvisioning(nullptr),
    _pCharNetworkStatus(nullptr),
    _pCharPairing(nullptr),
    _pCharEvents(nullptr),
    _deviceConnected(false),
    _oldDeviceConnected(false),
    _initialized(false) {
}

void CroenixBLEManager::begin() {
  CROENIX_LOG("BLEManager", "Initializing BLE Setup & Recovery Engine as '%s'...", BLE_DEVICE_NAME);

  BLEDevice::init(BLE_DEVICE_NAME);
  _pServer = BLEDevice::createServer();
  _pServer->setCallbacks(this);

  _setupServiceAndCharacteristics();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  _initialized = true;
  if (_stateManager) {
    _stateManager->setBleStatus(true, false);
  }
  CROENIX_LOG("BLEManager", "BLE Advertising active. Service UUID: %s", BLE_SERVICE_UUID);
}

void CroenixBLEManager::_setupServiceAndCharacteristics() {
  _pService = _pServer->createService(BLE_SERVICE_UUID);

  // 1. Device Info (Read)
  _pCharDeviceInfo = _pService->createCharacteristic(
    BLE_CHAR_DEVICE_INFO,
    BLECharacteristic::PROPERTY_READ
  );
  if (_stateManager) {
    _pCharDeviceInfo->setValue(_stateManager->getDeviceApiJson().c_str());
  }

  // 2. Provisioning / WiFi Config (Write)
  _pCharProvisioning = _pService->createCharacteristic(
    BLE_CHAR_PROVISIONING,
    BLECharacteristic::PROPERTY_WRITE
  );
  _pCharProvisioning->setCallbacks(new ProvisioningCallbacks(this));

  // 3. Network Status (Read + Notify)
  _pCharNetworkStatus = _pService->createCharacteristic(
    BLE_CHAR_NETWORK_STATUS,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  _pCharNetworkStatus->addDescriptor(new BLE2902());
  if (_stateManager) {
    _pCharNetworkStatus->setValue(_stateManager->getNetworkApiJson().c_str());
  }

  // 4. Pairing / Token (Read + Write)
  _pCharPairing = _pService->createCharacteristic(
    BLE_CHAR_PAIRING,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  _pCharPairing->setCallbacks(new PairingCallbacks(this));

  // 5. Device Events (Notify)
  _pCharEvents = _pService->createCharacteristic(
    BLE_CHAR_EVENTS,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  _pCharEvents->addDescriptor(new BLE2902());

  _pService->start();
}

void CroenixBLEManager::onConnect(BLEServer* pServer) {
  _deviceConnected = true;
  CROENIX_LOG("BLEManager", "Client connected via BLE.");
  if (_stateManager) {
    _stateManager->setBleStatus(true, true);
  }
}

void CroenixBLEManager::onDisconnect(BLEServer* pServer) {
  _deviceConnected = false;
  CROENIX_LOG("BLEManager", "Client disconnected from BLE.");
  if (_stateManager) {
    _stateManager->setBleStatus(true, false);
  }
}

void CroenixBLEManager::update() {
  if (!_initialized) return;

  // Restart advertising if disconnected
  if (!_deviceConnected && _oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    _pServer->startAdvertising();
    CROENIX_LOG("BLEManager", "Restarted BLE advertising.");
    _oldDeviceConnected = _deviceConnected;
  }
  if (_deviceConnected && !_oldDeviceConnected) {
    _oldDeviceConnected = _deviceConnected;
  }
}

void CroenixBLEManager::stop() {
  if (_initialized) {
    BLEDevice::deinit(true);
    _initialized = false;
    if (_stateManager) {
      _stateManager->setBleStatus(false, false);
    }
    CROENIX_LOG("BLEManager", "BLE deinitialized.");
  }
}

bool CroenixBLEManager::isConnected() {
  return _deviceConnected;
}

void CroenixBLEManager::notifyNetworkStatus(const String& statusJson) {
  if (_deviceConnected && _pCharNetworkStatus) {
    _pCharNetworkStatus->setValue(statusJson.c_str());
    _pCharNetworkStatus->notify();
  }
}

void CroenixBLEManager::notifyEvent(const String& eventJson) {
  if (_deviceConnected && _pCharEvents) {
    _pCharEvents->setValue(eventJson.c_str());
    _pCharEvents->notify();
  }
}

void CroenixBLEManager::handleProvisioningWrite(const String& value) {
  CROENIX_LOG("BLEManager", "Received provisioning payload (size: %u bytes)", value.length());
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, value);
  if (err) {
    CROENIX_LOG("BLEManager", "Invalid JSON in BLE provisioning");
    return;
  }

  if (doc["ssid"].is<String>() && doc["password"].is<String>()) {
    String ssid = doc["ssid"].as<String>();
    String pass = doc["password"].as<String>();
    CROENIX_LOG("BLEManager", "Provisioning WiFi router: %s", ssid.c_str());
    if (_wifiManager) {
      _wifiManager->connectToRouterAsync(ssid, pass, true);
    }
  }
}

String CroenixBLEManager::handlePairingRead() {
  if (!_authManager) return "{\"error\":\"AUTH_UNAVAILABLE\"}";
  if (_authManager->isPairingModeActive()) {
    String token = _authManager->generateAndStoreToken();
    JsonDocument doc;
    doc["token"] = token;
    String out;
    serializeJson(doc, out);
    return out;
  }
  return "{\"error\":\"PAIRING_INACTIVE\"}";
}

void CroenixBLEManager::handlePairingWrite(const String& value) {
  JsonDocument doc;
  if (!deserializeJson(doc, value)) {
    if (doc["action"] == "enable_pairing") {
      if (_authManager) _authManager->enablePairingMode();
    }
  }
}

