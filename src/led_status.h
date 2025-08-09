#pragma once
#include <stdint.h>
#include <stdbool.h>

// ==== Config (you may override before including) ====
#ifndef LED_GPIO
#define LED_GPIO 15          // LOLIN S2 Mini onboard LED
#endif

#ifndef LED_ACTIVE_HIGH
#define LED_ACTIVE_HIGH 1    // set to 0 if LED is active-low
#endif
// ====================================================

typedef enum {
    LED_STAT_OFF = 0,
    LED_STAT_BOOTING,
    LED_STAT_IDLE_WAITING_BB,   // SD OK, waiting for blackbox stream
    LED_STAT_LOGGING,           // stream active
    LED_STAT_SERIAL_LOST,       // no frames for a while
    LED_STAT_SD_ERROR,          // mount/write/fs error
    LED_STAT_LOW_SPACE,         // disk free below threshold
    LED_STAT_FILE_CLOSING,      // brief strobe, then auto-revert
    LED_STAT_FATAL,             // unrecoverable
} led_status_t;

#ifdef __cplusplus
extern "C" {
#endif

void led_status_init(void);
void led_set_status(led_status_t st);
// Pulse FILE_CLOSING pattern for duration_ms, then auto-revert to previous status.
void led_pulse_file_closing(uint32_t duration_ms);
led_status_t led_get_status(void);

#ifdef __cplusplus
}
#endif
