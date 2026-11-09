#ifndef DISCOVERY_MANAGER_H
#define DISCOVERY_MANAGER_H

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>

class DiscoveryManager {
public:
  DiscoveryManager();
  void begin();
  void update();

  bool startMDNS(const String& hostname, const String& firmwareVersion);
  void updateMDNSTxtRecords(const String& firmwareVersion);

private:
  WiFiUDP _udp;
  bool _mdnsStarted;

  void _handleUDPDiscovery();
};

#endif // DISCOVERY_MANAGER_H
