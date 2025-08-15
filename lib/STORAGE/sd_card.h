#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>
#include <freertos/queue.h>

struct SdLogCommand {
  enum Type : uint8_t { Start, Stop, Mark } type;
};

namespace SdLog {
  void begin(StreamBufferHandle_t buf, QueueHandle_t cmdQ);
  bool isLogging();
}
