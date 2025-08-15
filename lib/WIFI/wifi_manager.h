#pragma once
#include <Arduino.h>

namespace WifiMgr {
  void beginAP(const char* ssid, const char* pwd);
  // Future: void beginSTA(const char* ssid, const char* pwd);
}
