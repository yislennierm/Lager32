// include/board_pins.h
#pragma once

// ===== Common UART config (you can adjust per-board below) =====
#ifndef BB_UART_BAUD
#define BB_UART_BAUD 2000000  // 2 Mbps
#endif

// ===== Board-specific pin maps =====
#if defined(BOARD_LOLIN_S2_MINI)
// SPI (recommended set for LOLIN S2 Mini)

// UART (FC TX -> ESP RX)
#define BB_UART_RX_PIN 18
#define BB_UART_TX_PIN -1

#elif defined(BOARD_WROVER)
// ESP32 WROVER-KIT (VSPI defaults)
#define PIN_NUM_SCLK 18
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CS    5
// UART
#define BB_UART_RX_PIN 16
#define BB_UART_TX_PIN -1

#elif defined(BOARD_S3_DEVKITC)
// ESP32-S3 DevKitC example
#define PIN_NUM_SCLK 36
#define PIN_NUM_MISO 37
#define PIN_NUM_MOSI 35
#define PIN_NUM_CS   10
#define BB_UART_RX_PIN 4
#define BB_UART_TX_PIN -1

#else
#error "Select a board in platformio.ini (e.g., -DBOARD_LOLIN_S2_MINI)."
#endif
