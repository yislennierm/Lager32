// src/main.cpp — WebUI first, RF later
#define BB_WEBUI_ONLY 1

#include <Arduino.h>

#if defined(ESP32)
  #include <SPIFFS.h>
  #include "esp_task_wdt.h"
#endif

#include "rxtx_common.h"  // brings in common.h + connectionState
#include "options.h"
#include "devWIFI.h"      // declares WIFI_device + devices* API

// ELRS-style globals used by WIFI/devices
Stream*       SerialLogger = nullptr;
unsigned long rebootTime   = 0;
extern bool   webserverPreventAutoStart;


// --- ELRS globals used by WIFI/devices framework ---
//Stream*       SerialLogger = nullptr;   // used by logging
//unsigned long rebootTime   = 0;         // WIFI device schedules reboots via this
extern bool   webserverPreventAutoStart; // defined in devWIFI.cpp

// --- stub only while WebUI-only ---
static inline void setConnectionState(connectionState_e st) { connectionState = st; }

// ====== factory reset helper (no config.* calls in WebUI-only) ======
void resetConfigAndReboot() {
  yield();
  SPIFFS.format();
  yield();
  SPIFFS.begin();
  // options_SetTrueDefaults();  // <-- remove (not exported in your trimmed set)
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  SerialLogger = &Serial;

#if defined(ESP32)
  SPIFFS.begin(true);
#endif

  if (!options_init()) {
    static device_affinity_t wifi_device[] = { { &WIFI_device, 1 } };
    devicesRegister(wifi_device, (uint8_t)1); // avoid overload ambiguity
    devicesInit();
    setConnectionState(hardwareUndefined);
  } else {
    static device_affinity_t ui_devices[] = { { &WIFI_device, 0 } };
    devicesRegister(ui_devices, (uint8_t)1);
    devicesInit();
    webserverPreventAutoStart = false;
  }

  devicesStart();
}

void loop() {
  devicesUpdate(millis());
#if defined(ESP32)
  if (rebootTime != 0 && millis() > rebootTime) ESP.restart();
#endif
}
