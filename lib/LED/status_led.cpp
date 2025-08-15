#include "status_led.h"

static int sPin = -1;
static LedMode sMode = LedMode::OFF;
static uint32_t sT0 = 0;

void LedStatus::begin(int pin) {
  sPin = pin;
  pinMode(sPin, OUTPUT);
  digitalWrite(sPin, LOW);
}

void LedStatus::setMode(LedMode m) {
  sMode = m;
  sT0 = millis();
}

void LedStatus::tick() {
  if (sPin < 0) return;
  uint32_t t = millis() - sT0;
  bool on = false;
  switch (sMode) {
    case LedMode::OFF:      on = false;                       break;
    case LedMode::AP_READY: on = (t % 1000) < 100;            break; // short blip
    case LedMode::LOGGING:  on = (t % 250)  < 50;             break; // fast blink
    case LedMode::ERROR:    on = (t % 500)  < 250;            break; // 1:1
  }
  digitalWrite(sPin, on ? HIGH : LOW);
}
