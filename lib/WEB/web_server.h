#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>
#include <freertos/queue.h>

namespace WebUi {
  void begin(StreamBufferHandle_t buf, QueueHandle_t cmdQ);
}
