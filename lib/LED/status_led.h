#pragma once
#include <Arduino.h>

enum class LedMode {
  OFF = 0,
  AP_READY,
  LOGGING,
  ERROR
};

namespace LedStatus {
  void begin(int pin);
  void setMode(LedMode m);
  void tick();
}
