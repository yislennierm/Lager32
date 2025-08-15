#include "wifi_manager.h"

IPAddress WifiMgr::s_apIP;

void WifiMgr::beginAP(const char* ssid, const char* pwd, uint8_t channel, bool hidden) {
  // Don’t burn flash with WiFi credentials writes
  WiFi.persistent(false);

  // Clean mode switch (prevents odd states)
  WiFi.mode(WIFI_OFF);
  delay(50);
  WiFi.mode(WIFI_AP);

  // Keep radio awake (more reliable web)
  WiFi.setSleep(false);

  // Give the AP a stable IP (and DHCP range) you expect
  IPAddress ip(192,168,4,1);
  IPAddress gw(192,168,4,1);
  IPAddress mask(255,255,255,0);
  if (!WiFi.softAPConfig(ip, gw, mask)) {
    Serial.println("[WIFI] softAPConfig failed, using core default");
  }

  // Use the parameters you pass in (no hard-coded SSID)
  bool ok = WiFi.softAP(ssid, pwd, channel, hidden ? 1 : 0, 4 /*max conn*/);
  if (!ok) {
    Serial.println("[WIFI] softAP() failed!");
    return;
  }

  s_apIP = WiFi.softAPIP();

  Serial.println("========================================");
  Serial.println("[WIFI] Access Point started");
  Serial.printf("[WIFI]   SSID:     %s\n", ssid);
  Serial.printf("[WIFI]   Password: %s\n", (pwd && *pwd) ? pwd : "(open)");
  Serial.printf("[WIFI]   Channel:  %u\n", channel);
  Serial.printf("[WIFI]   Hidden:   %s\n", hidden ? "yes" : "no");
  Serial.printf("[WIFI]   AP IP:    %s\n", s_apIP.toString().c_str());
  Serial.println("[WIFI]   Browse:   http://192.168.4.1/");
  Serial.println("========================================");
}

IPAddress WifiMgr::apIP() { return s_apIP; }

bool WifiMgr::isAP() {
  // In Arduino-ESP32, mode is a bitmask
  return (WiFi.getMode() & WIFI_AP) != 0;
}
