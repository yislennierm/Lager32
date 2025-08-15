#include <Arduino.h>          // for Serial
#include "uart_in.h"
#include "driver/uart.h"
#include "esp_err.h"

// ------- Compile-time debug knobs --------
#ifndef UART_DEBUG
#define UART_DEBUG 1          // 0 = silent, 1 = print stats
#endif
#ifndef UART_DEBUG_HEXDUMP
#define UART_DEBUG_HEXDUMP 0  // 1 = print first bytes of each read as hex
#endif

// ------- HW config (change to your board) -------
static const int PIN_UART_RX = 18;   // <-- set your RX pin
static const int PIN_UART_TX = 17;   // <-- set your TX pin (or UART_PIN_NO_CHANGE)

// ------- Module state -------
static StreamBufferHandle_t sBuf = nullptr;
static uart_port_t sPort = UART_NUM_1;

static void hexdump(const uint8_t* p, size_t n, size_t max = 64) {
#if UART_DEBUG && UART_DEBUG_HEXDUMP
  size_t m = n < max ? n : max;
  for (size_t i = 0; i < m; ++i) {
    if (i && (i % 16) == 0) Serial.println();
    Serial.printf("%02X ", p[i]);
  }
  if (m) Serial.println();
#endif
}

void UartIn::begin(StreamBufferHandle_t buf, uart_port_t port, int baud) {
  sBuf  = buf;
  sPort = port;

  // UART config
  uart_config_t cfg{};
  cfg.baud_rate = baud;
  cfg.data_bits = UART_DATA_8_BITS;
  cfg.parity    = UART_PARITY_DISABLE;
  cfg.stop_bits = UART_STOP_BITS_1;
  cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  cfg.rx_flow_ctrl_thresh = 0;

  esp_err_t err;

  err = uart_param_config(sPort, &cfg);
#if UART_DEBUG
  if (err != ESP_OK) Serial.printf("[UART] uart_param_config err=%d\n", err);
#endif

  // RX buffer 8k, no TX buffer
  err = uart_driver_install(sPort, 8192, 0, 0, nullptr, 0);
#if UART_DEBUG
  if (err != ESP_OK) Serial.printf("[UART] uart_driver_install err=%d\n", err);
#endif

  err = uart_set_pin(
    sPort,
    (PIN_UART_TX >= 0) ? PIN_UART_TX : UART_PIN_NO_CHANGE,
    (PIN_UART_RX >= 0) ? PIN_UART_RX : UART_PIN_NO_CHANGE,
    UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
#if UART_DEBUG
  if (err != ESP_OK) Serial.printf("[UART] uart_set_pin err=%d\n", err);

  Serial.printf("[UART] start port=%d baud=%d RX=%d TX=%d\n",
                (int)sPort, baud, PIN_UART_RX, PIN_UART_TX);
#endif

  // RX task
  xTaskCreate([](void*) {
    uint8_t tmp[1024];

    // stats
    size_t bytes_read_total   = 0;
    size_t bytes_enq_total    = 0;
    size_t bytes_dropped_total= 0;
    TickType_t last_stat = xTaskGetTickCount();

    for (;;) {
      // read with small timeout to yield
      int n = uart_read_bytes(sPort, tmp, sizeof(tmp), 10 / portTICK_PERIOD_MS);
      if (n > 0) {
        bytes_read_total += n;

        size_t sent = xStreamBufferSend(sBuf, tmp, n, 0); // non-blocking
        bytes_enq_total += sent;
        if ((int)sent < n) bytes_dropped_total += (n - sent);

#if UART_DEBUG_HEXDUMP
        hexdump(tmp, (size_t)n, 64);
#endif
      }

#if UART_DEBUG
      // print once per second
      TickType_t now = xTaskGetTickCount();
      if (now - last_stat >= pdMS_TO_TICKS(1000)) {
        size_t f = xStreamBufferSpacesAvailable(sBuf);
        Serial.printf("[UART] read=%u enq=%u drop=%u sbuf_free=%u\n",
                      (unsigned)bytes_read_total,
                      (unsigned)bytes_enq_total,
                      (unsigned)bytes_dropped_total,
                      (unsigned)f);
        bytes_read_total = bytes_enq_total = bytes_dropped_total = 0;
        last_stat = now;
      }
#endif
    }
  }, "uart_rx", 4096, nullptr, 3, nullptr);
}
