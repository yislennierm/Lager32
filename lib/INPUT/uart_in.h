#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>
#include "driver/uart.h"

namespace UartIn {
  void begin(StreamBufferHandle_t buf, uart_port_t port, int baud);
}
