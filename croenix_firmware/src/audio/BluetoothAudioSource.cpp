#include "BluetoothAudioSource.h"
#include "../core/PlayerStateManager.h"
#include "../Config.h"
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include <esp_a2dp_api.h>
#include <esp_avrc_api.h>

static BluetoothAudioSource* s_btAudioSourceInstance = nullptr;

static void bt_a2d_data_callback(const uint8_t *data, uint32_t len) {
  if (s_btAudioSourceInstance) {
    s_btAudioSourceInstance->handleAudioData(data, len);
  }
}

static void bt_a2d_connection_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
  if (s_btAudioSourceInstance) {
    s_btAudioSourceInstance->handleConnectionEvent(event, param);
  }
}

BluetoothAudioSource::BluetoothAudioSource(PlayerStateManager* stateManager)
  : _stateManager(stateManager), _connected(false), _isStreaming(false) {
  s_btAudioSourceInstance = this;
}

bool BluetoothAudioSource::begin() {
  CROENIX_LOG("BTAudioSource", "Initializing Always-On Bluetooth Classic A2DP Audio Sink...");

  // Set device name for Bluetooth Classic pairing
  esp_bt_dev_set_device_name("Croenix Audio");

  // Initialize A2DP sink
  esp_err_t ret = esp_a2d_sink_init();
  if (ret != ESP_OK) {
    CROENIX_LOG("BTAudioSource", "esp_a2d_sink_init failed: %d", ret);
    return false;
  }

  esp_a2d_register_callback(bt_a2d_connection_callback);
  esp_a2d_sink_register_data_callback(bt_a2d_data_callback);

  // Set discoverable & connectable mode so phones can connect anytime
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

  CROENIX_LOG("BTAudioSource", "Bluetooth A2DP Sink active: phone can pair with 'Croenix Audio' for music.");
  return true;
}

void BluetoothAudioSource::stop() {
  _connected = false;
  _isStreaming = false;
  if (_stateManager) _stateManager->setPlaybackStatus("stopped", true);
}

bool BluetoothAudioSource::play() {
  if (!isAvailable()) return false;
  if (_stateManager) _stateManager->setPlaybackStatus("playing", true);
  return true;
}

bool BluetoothAudioSource::pause() {
  if (_stateManager) _stateManager->setPlaybackStatus("paused", true);
  return true;
}

bool BluetoothAudioSource::isAvailable() {
  return _connected;
}

void BluetoothAudioSource::handleAudioData(const uint8_t *data, uint32_t len) {
  // PCM audio data received from phone (16-bit stereo 44.1kHz)
  if (!_isStreaming) {
    _isStreaming = true;
    if (_stateManager) {
      _stateManager->setSource("bluetooth", true);
      _stateManager->setPlaybackStatus("playing", true);
    }
  }
}

void BluetoothAudioSource::handleConnectionEvent(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
  switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
      if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
        _connected = true;
        CROENIX_LOG("BTAudioSource", "[EVENT] Phone connected to Bluetooth A2DP audio!");
        if (_stateManager) {
          _stateManager->setSource("bluetooth", true);
          _stateManager->setBleStatus(true, true);
        }
      } else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
        _connected = false;
        _isStreaming = false;
        CROENIX_LOG("BTAudioSource", "[EVENT] Bluetooth audio disconnected.");
        if (_stateManager) {
          _stateManager->setPlaybackStatus("stopped", true);
        }
      }
      break;

    case ESP_A2D_AUDIO_STATE_EVT:
      if (param->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED) {
        _isStreaming = true;
        if (_stateManager) _stateManager->setPlaybackStatus("playing", true);
      } else if (param->audio_stat.state == ESP_A2D_AUDIO_STATE_STOPPED) {
        _isStreaming = false;
        if (_stateManager) _stateManager->setPlaybackStatus("stopped", true);
      }
      break;

    default:
      break;
  }
}
