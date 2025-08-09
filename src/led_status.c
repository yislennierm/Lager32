#include "led_status.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/portmacro.h"
#include "driver/gpio.h"
#include <string.h>

// ---- Pattern encoding ----
// Sequence of durations in ms: ON, OFF, ON, OFF... terminated by 0.
// Special first items:
//   0      -> constant OFF
//   0xFFFF -> SOLID ON

// Base durations (ms)
#define S   150
#define M   400
#define L   600
#define GAP 1000

static const uint16_t PATTERN_OFF[]          = { 0 };
//static const uint16_t PATTERN_SOLID[]        = { 0xFFFF, 0 };
static const uint16_t PATTERN_BOOTING[]      = { S, S, S, 600, 0 };
static const uint16_t PATTERN_IDLE[]         = { S, 1850, 0 };
static const uint16_t PATTERN_LOGGING[]      = { 0xFFFF, 0 };
static const uint16_t PATTERN_SERIAL_LOST[]  = { S, S, 700, 0 };
static const uint16_t PATTERN_SD_ERROR[]     = { S, S, S, 700, 0 };
static const uint16_t PATTERN_LOW_SPACE[]    = { L, 200, S, GAP, 0 };
static const uint16_t PATTERN_FILE_CLOSE[]   = { 80, 80, 80, 80, 80, 400, 0 };
static const uint16_t PATTERN_FATAL[]        = { 2000, 200, 0 };

typedef struct { const uint16_t *seq; } led_pattern_t;

static const led_pattern_t PATTERNS[] = {
    [LED_STAT_OFF]             = { PATTERN_OFF },
    [LED_STAT_BOOTING]         = { PATTERN_BOOTING },
    [LED_STAT_IDLE_WAITING_BB] = { PATTERN_IDLE },
    [LED_STAT_LOGGING]         = { PATTERN_LOGGING },
    [LED_STAT_SERIAL_LOST]     = { PATTERN_SERIAL_LOST },
    [LED_STAT_SD_ERROR]        = { PATTERN_SD_ERROR },
    [LED_STAT_LOW_SPACE]       = { PATTERN_LOW_SPACE },
    [LED_STAT_FILE_CLOSING]    = { PATTERN_FILE_CLOSE },
    [LED_STAT_FATAL]           = { PATTERN_FATAL },
};

static TaskHandle_t  s_led_task      = NULL;
static TimerHandle_t s_revert_timer  = NULL;
static const uint16_t *s_current_seq = PATTERN_OFF;
static bool          s_phase_on      = true;
static size_t        s_idx           = 0;

static led_status_t  s_current_status   = LED_STAT_OFF;
static led_status_t  s_last_persistent  = LED_STAT_OFF;

// IDF v5 critical section spinlock
static portMUX_TYPE s_spin = portMUX_INITIALIZER_UNLOCKED;

static inline void led_write(bool on)
{
#if LED_ACTIVE_HIGH
    gpio_set_level(LED_GPIO, on ? 1 : 0);
#else
    gpio_set_level(LED_GPIO, on ? 0 : 1);
#endif
}

static void IRAM_ATTR set_pattern_for_status(led_status_t st)
{
    s_current_seq = PATTERNS[st].seq ? PATTERNS[st].seq : PATTERN_OFF;
    s_idx         = 0;
    s_phase_on    = true;
    s_current_status = st;
}

static void led_revert_cb(TimerHandle_t xTimer)
{
    portENTER_CRITICAL(&s_spin);
    set_pattern_for_status(s_last_persistent);
    portEXIT_CRITICAL(&s_spin);
}

static void led_task(void *arg)
{
    while (1) {
        if (!s_current_seq || s_current_seq[0] == 0) {
            // OFF special
            led_write(false);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        if (s_current_seq[0] == 0xFFFF) {
            // SOLID ON special
            led_write(true);
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        uint16_t dur = s_current_seq[s_idx];
        if (dur == 0) {
            // restart pattern
            s_idx = 0;
            s_phase_on = true;
            continue;
        }

        led_write(s_phase_on);
        vTaskDelay(pdMS_TO_TICKS(dur));

        s_phase_on = !s_phase_on;
        s_idx++;
    }
}

void led_status_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    led_write(false);

    s_revert_timer = xTimerCreate("led_revert",
                                  pdMS_TO_TICKS(100),
                                  pdFALSE,
                                  NULL,
                                  led_revert_cb);

    xTaskCreatePinnedToCore(led_task, "led_task", 2048, NULL, 1, &s_led_task, tskNO_AFFINITY);

    set_pattern_for_status(LED_STAT_BOOTING);
    s_last_persistent = LED_STAT_BOOTING;
}

void led_set_status(led_status_t st)
{
    portENTER_CRITICAL(&s_spin);
    if (st != LED_STAT_FILE_CLOSING) {
        s_last_persistent = st;
        if (s_revert_timer) xTimerStop(s_revert_timer, 0);
    }
    set_pattern_for_status(st);
    portEXIT_CRITICAL(&s_spin);
}

void led_pulse_file_closing(uint32_t duration_ms)
{
    if (!s_revert_timer) return;
    portENTER_CRITICAL(&s_spin);
    set_pattern_for_status(LED_STAT_FILE_CLOSING);
    xTimerStop(s_revert_timer, 0);
    xTimerChangePeriod(s_revert_timer, pdMS_TO_TICKS(duration_ms), 0);
    xTimerStart(s_revert_timer, 0);
    portEXIT_CRITICAL(&s_spin);
}

led_status_t led_get_status(void)
{
    return s_current_status;
}
