#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>
#include <freertos/queue.h>

#include "web_server.h"
#include "wifi_manager.h"
#include "status_led.h"
#include "uart_in.h"
#include "sd_card.h"

// Global IPC
StreamBufferHandle_t gLogBuf;     // bytes from UART -> SD
QueueHandle_t        gCmdQ;       // control commands (start/stop/mark)

void setup() {
  Serial.begin(115200);

  // Buffer: 16KB, trigger level 256 bytes
  gLogBuf = xStreamBufferCreate(16 * 1024, 256);
  gCmdQ   = xQueueCreate(8, sizeof(SdLogCommand));
  if (!gLogBuf || !gCmdQ) { while (true) { delay(1000); } }

  LedStatus::begin(LED_BUILTIN);
  LedStatus::setMode(LedMode::AP_READY);

  // Simple AP for now; you can add STA later in WifiMgr
  WifiMgr::beginAP("Lager32", "12345678");

  // Web UI + control endpoints
  WebUi::begin(gLogBuf, gCmdQ);

  // UART in: set your pins/baud in UartIn.cpp
  UartIn::begin(gLogBuf, UART_NUM_1, 250000);

  // SD log: set your SD pins in SdLog.cpp
  SdLog::begin(gLogBuf, gCmdQ);
}

void loop() {
  // Keep this very light
  LedStatus::tick();
  delay(1);
}
