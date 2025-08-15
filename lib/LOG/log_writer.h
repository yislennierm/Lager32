#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>
#include <freertos/queue.h>
#include "storage.h"  // contains enum class LogCommand

namespace LogWriter {
  void begin(StreamBufferHandle_t buf, QueueHandle_t cmdQ, ILogSink* sink);
}
