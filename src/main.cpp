#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>
#include <freertos/queue.h>
#include <LittleFS.h>
#include <Preferences.h>
#include "web_server.h"
#include "wifi_manager.h"
#include "status_led.h"
#include "uart_in.h"

// STORAGE + LOGGER (from lib/STORAGE and lib/LOGGER, etc.)
#include <storage.h>
#include <flash.h>
#include <log_writer.h>

// Some ESP32 S2 boards don’t define this; pick the right pin for your board if needed.
#ifndef LED_BUILTIN
#define LED_BUILTIN 15
#endif

// ---------- Global IPC ----------
//StreamBufferHandle_t gLogBuf = nullptr;   // bytes from UART -> LogWriter
QueueHandle_t        gCmdQ   = nullptr;   // control commands (start/stop/mark)

// Global storage so WebUi handlers can access it (for listing/downloading)
ILogSink* gStorage = nullptr;

/*
static void printFsStats() {
  size_t total = LittleFS.totalBytes();
  size_t used  = LittleFS.usedBytes();
  Serial.printf("[FS] LittleFS: total=%u used=%u free=%u\n",
                (unsigned)total, (unsigned)used, (unsigned)(total > used ? total - used : 0));
}*/

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(500));
  Serial.println();
  Serial.println("=== Lager32 boot ===");
   // ---------- Wi-Fi ----------
  // Simple AP for now; you can add STA logic to WifiMgr later.
  WifiMgr::beginAP("Lager32", "12345678");   // will print the IP in Serial

  /*
  // ---------- IPC ----------
  // Buffer: 16KB, trigger level 256 bytes (tweak as you like)
  gLogBuf = xStreamBufferCreate(16 * 1024, 256);
  gCmdQ   = xQueueCreate(8, sizeof(LogCommand));
  if (!gLogBuf || !gCmdQ) {
    Serial.println("[FATAL] IPC create failed");
    while (true) { delay(1000); }
  }*/

  // ---------- LED ----------
  LedStatus::begin(LED_BUILTIN);
  LedStatus::setMode(LedMode::AP_READY);

 /*
  // ---------- Storage (LittleFS in internal flash) ----------
  // Use our FlashLogSink (lib/STORAGE). Try normal mount first, then format if needed.
  static FlashLogSink flashSink;
  gStorage = &flashSink;
  gStorage->begin(true);   // <-- pass bool to match ILogSink
*/
/*
  if (!gStorage->begin(false)) {
    Serial.println("[FS] Mount failed, trying to format...");
    if (!gStorage->begin(true)) {
      Serial.println("[FS] Mount failed even after format. Aborting.");
      while (true) { delay(1000); }
    }
  }*/
  //printFsStats();

  // ---------- Log Writer task ----------
  // This task reads from gLogBuf and writes to the active sink (gStorage).
  //LogWriter::begin(gLogBuf, gCmdQ, gStorage);

  

  // ---------- Web UI ----------
  // Serves the UI + control endpoints (start/stop/mark, list/download).
  WebUi::begin(gCmdQ, gStorage );


  // ---------- UART in ----------
  // Set UART pins/baud in UartIn.cpp for your hardware.
  //UartIn::begin(gLogBuf, UART_NUM_1, 115200);

  Serial.println("[OK] Setup complete.");
}

void loop() {
  // Keep this very light — all real work is in tasks.
  LedStatus::tick();
  delay(1);
}
