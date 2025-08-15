#include "wifi_manager.h"
#include <WiFi.h>

void WifiMgr::beginAP(const char* ssid, const char* pwd) {
  WiFi.mode(WIFI_AP);
  IPAddress ip(10,0,0,1), gw(10,0,0,1), mask(255,255,255,0);
  WiFi.softAP("AeroTrace32", "12345678");
}
