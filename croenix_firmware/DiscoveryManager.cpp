#include "DiscoveryManager.h"
#include "Config.h"

DiscoveryManager::DiscoveryManager() : _mdnsStarted(false) {
}

void DiscoveryManager::begin() {
  // Start UDP listener for discovery broadcast
  if (_udp.begin(UDP_DISCOVERY_PORT)) {
    DEBUG_PRINT("[DiscoveryManager] UDP discovery listening on port ");
    DEBUG_PRINTLN(UDP_DISCOVERY_PORT);
  } else {
    DEBUG_PRINTLN("[DiscoveryManager] Failed to start UDP discovery");
  }
}

void DiscoveryManager::update() {
  _handleUDPDiscovery();
}

bool DiscoveryManager::startMDNS(const String& hostname, const String& firmwareVersion) {
  if (_mdnsStarted) {
    return true;
  }

  DEBUG_PRINT("[DiscoveryManager] Starting mDNS as: ");
  DEBUG_PRINT(hostname);
  DEBUG_PRINTLN(".local");

  if (!MDNS.begin(hostname.c_str())) {
    DEBUG_PRINTLN("[DiscoveryManager] mDNS start failed!");
    return false;
  }

  // Add HTTP service
  MDNS.addService("http", "tcp", HTTP_PORT);

  // Add Croenix service with TXT records
  MDNS.addService(MDNS_SERVICE_NAME, MDNS_PROTOCOL, HTTP_PORT);
  MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "version", firmwareVersion);
  MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "name", DEVICE_NAME);

  _mdnsStarted = true;
  DEBUG_PRINTLN("[DiscoveryManager] mDNS started successfully");

  return true;
}

void DiscoveryManager::updateMDNSTxtRecords(const String& firmwareVersion) {
  if (!_mdnsStarted) {
    return;
  }

  // Update TXT records
  MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "version", firmwareVersion);
}

void DiscoveryManager::_handleUDPDiscovery() {
  int packetSize = _udp.parsePacket();

  if (packetSize > 0) {
    char incomingPacket[255];
    int len = _udp.read(incomingPacket, 255);

    if (len > 0) {
      incomingPacket[len] = 0;

      // Check if it's a discovery request
      if (strcmp(incomingPacket, DISCOVERY_MAGIC) == 0) {
        DEBUG_PRINTLN("[DiscoveryManager] Discovery request received");

        // Send response
        IPAddress remoteIP = _udp.remoteIP();
        uint16_t remotePort = _udp.remotePort();

        _udp.beginPacket(remoteIP, remotePort);
        _udp.print(DISCOVERY_RESPONSE);
        _udp.endPacket();

        DEBUG_PRINT("[DiscoveryManager] Sent discovery response to ");
        DEBUG_PRINTLN(remoteIP);
      }
    }
  }
}
