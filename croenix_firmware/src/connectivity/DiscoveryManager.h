#ifndef CROENIX_DISCOVERY_MANAGER_H
#define CROENIX_DISCOVERY_MANAGER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>

class DiscoveryManager {
public:
  DiscoveryManager();
  void begin();
  void update();

  bool startMDNS(const String& hostname, const String& firmwareVersion);
  void stopMDNS();

private:
  WiFiUDP _udp;
  bool _mdnsRunning;
  bool _udpRunning;

  void _processUDPPackets();
};

#endif // CROENIX_DISCOVERY_MANAGER_H

