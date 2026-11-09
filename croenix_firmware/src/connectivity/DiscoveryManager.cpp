#include "DiscoveryManager.h"
#include "../Config.h"

DiscoveryManager::DiscoveryManager()
  : _mdnsRunning(false), _udpRunning(false) {
}

void DiscoveryManager::begin() {
  if (_udp.begin(UDP_DISCOVERY_PORT)) {
    _udpRunning = true;
    CROENIX_LOG("DiscoveryManager", "UDP broadcast discovery listening on port %d", UDP_DISCOVERY_PORT);
  } else {
    CROENIX_LOG("DiscoveryManager", "Failed to start UDP discovery socket!");
  }
}

void DiscoveryManager::update() {
  if (_udpRunning) {
    _processUDPPackets();
  }
}

bool DiscoveryManager::startMDNS(const String& hostname, const String& firmwareVersion) {
  if (_mdnsRunning) return true;

  CROENIX_LOG("DiscoveryManager", "Starting mDNS responder for %s.local", hostname.c_str());
  if (!MDNS.begin(hostname.c_str())) {
    CROENIX_LOG("DiscoveryManager", "mDNS startup failed!");
    return false;
  }

  MDNS.addService("http", "tcp", HTTP_PORT);
  MDNS.addService(MDNS_SERVICE_NAME, MDNS_PROTOCOL, HTTP_PORT);
  MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "version", firmwareVersion);
  MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "name", DEVICE_NAME);
  MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, "model", HARDWARE_IDENTIFIER);

  _mdnsRunning = true;
  CROENIX_LOG("DiscoveryManager", "mDNS services registered successfully");
  return true;
}

void DiscoveryManager::stopMDNS() {
  if (_mdnsRunning) {
    MDNS.end();
    _mdnsRunning = false;
    CROENIX_LOG("DiscoveryManager", "mDNS service stopped.");
  }
}

void DiscoveryManager::_processUDPPackets() {
  int packetSize = _udp.parsePacket();
  if (packetSize > 0) {
    char packetBuffer[128];
    int len = _udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = '\0';
      if (strcmp(packetBuffer, DISCOVERY_MAGIC) == 0) {
        IPAddress remoteIp = _udp.remoteIP();
        uint16_t remotePort = _udp.remotePort();
        CROENIX_LOG("DiscoveryManager", "Discovery magic received from %s:%u", remoteIp.toString().c_str(), remotePort);

        _udp.beginPacket(remoteIp, remotePort);
        _udp.print(DISCOVERY_RESPONSE);
        _udp.endPacket();
      }
    }
  }
}

