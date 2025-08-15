#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WifiMgr {
public:
  // Start a softAP with a predictable IP (default 192.168.4.1).
  static void beginAP(const char* ssid,
                      const char* pwd,
                      uint8_t channel = 6,
                      bool hidden = false);

  static IPAddress apIP();
  static bool isAP();

private:
  static IPAddress s_apIP;
};
