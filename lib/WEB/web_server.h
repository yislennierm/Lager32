#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "storage.h"   // ILogSink

namespace WebUi {
  // Web only needs the command queue and a storage sink to list/download files.
  void begin(QueueHandle_t cmdQ, ILogSink* storage);
}
