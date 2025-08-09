#include "Arduino"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/uart.h"
#include "driver/spi_master.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_system.h"

#include "led_status.h"

#include "sdkconfig.h"





// ============================
// === USER CONFIG ZONE =======
// ============================

// ----- SD over SPI pins (adjust to your wiring) -----
// Lolin S2 Mini defaults (adjust if needed)
#define PIN_NUM_MOSI   35
#define PIN_NUM_MISO   37
#define PIN_NUM_SCLK   36
#define PIN_NUM_CS     34

// ----- UART for Blackbox stream -----
#define BB_UART_PORT       UART_NUM_1
#define BB_UART_RX_PIN     18   // FC TX -> ESP32 RX
#define BB_UART_TX_PIN     -1   // not used
#define BB_UART_BAUD       2000000  // 2,000,000 baud typical for Serial Blackbox

// File & buffering
#define MOUNT_POINT        "/sdcard"
#define MAX_FILE_INDEX     9999
#define PREBUF_SIZE        8192     // pre-header ring buffer
#define UART_BUF_SIZE      4096     // driver ring buffer
#define READ_CHUNK         1024     // read chunk from UART
#define IDLE_TIMEOUT_MS    1200     // close file after idle
#define SERIAL_LOST_MS     3000     // status indication when waiting with no data
#define FLUSH_BYTES        (64*1024)

// Space monitoring
#define LOW_SPACE_BYTES    (50ULL * 1024ULL * 1024ULL)  // 50 MB

// ============================

static const char *TAG = "bb_logger";

static sdmmc_card_t *s_card = NULL;
static int s_spi_host_slot = -1;

// ---------- LED/state resolver flags ----------
static volatile bool s_fatal = false;
static volatile bool s_sd_ok = false;
static volatile bool s_sd_write_error = false;
static volatile bool s_sd_low_space = false;
static volatile bool s_logging_active = false;
static volatile bool s_ever_saw_header = false;
static volatile uint32_t s_uart_quiet_ms = 0;
// ---------------------------------------------

typedef enum {
    WAIT_HEADER = 0,
    LOGGING
} log_state_t;

static FILE *s_logf = NULL;
static log_state_t s_state = WAIT_HEADER;
static int s_next_index = 1;
static uint64_t s_last_rx_us = 0;
static size_t s_since_flush = 0;

// simple ring buffer to catch header bytes before we open the file
typedef struct {
    uint8_t data[PREBUF_SIZE];
    size_t  head;   // write index
    bool    filled; // wrapped at least once
} ring_t;

static ring_t s_prebuf = { .head = 0, .filled = false };

// line buffer to detect "H " header lines
#define BB_LINE_MAX 512
static char s_line[BB_LINE_MAX];
static size_t s_linelen = 0;
static bool s_at_line_start = true;

// Forward decl
static void decide_led_status(void);

static void ring_put(ring_t *r, const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        r->data[r->head] = buf[i];
        r->head = (r->head + 1) % PREBUF_SIZE;
        if (r->head == 0) r->filled = true;
    }
}

static void ring_dump_to_file(ring_t *r, FILE *f) {
    if (!f) return;
    if (!r->filled && r->head == 0) return;

    if (r->filled) {
        // write from head to end, then from 0 to head-1
        fwrite(&r->data[r->head], 1, PREBUF_SIZE - r->head, f);
        fwrite(&r->data[0],       1, r->head,                f);
    } else {
        fwrite(&r->data[0], 1, r->head, f);
    }
    // clear
    r->head = 0;
    r->filled = false;
}

static bool path_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

static esp_err_t find_next_filename(char *out, size_t outlen) {
    for (int i = s_next_index; i <= MAX_FILE_INDEX; ++i) {
        snprintf(out, outlen, MOUNT_POINT"/LOG%04d.BFL", i);
        if (!path_exists(out)) {
            s_next_index = i + 1;
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

static void close_current_file(void) {
    if (s_logf) {
        ESP_LOGI(TAG, "Closing file");
        fflush(s_logf);
        fclose(s_logf);
        s_logf = NULL;
    }
    s_logging_active = false;
    s_state = WAIT_HEADER;
    s_since_flush = 0;
    s_linelen = 0;
    s_at_line_start = true;
    decide_led_status();
}

// LED: short helper to show “closing” pulse, then return via resolver
static inline void indicate_file_closing_then_idle(void) {
    led_pulse_file_closing(400);                 // quick strobe burst
    vTaskDelay(pdMS_TO_TICKS(450));              // let the pulse sequence play
}

static esp_err_t open_new_file_and_dump_prebuf(void) {
    char path[64];
    if (find_next_filename(path, sizeof(path)) != ESP_OK) {
        ESP_LOGE(TAG, "No free filename slots");
        s_sd_write_error = true; // treat as SD error for LED
        decide_led_status();
        return ESP_FAIL;
    }
    s_logf = fopen(path, "wb");
    if (!s_logf) {
        ESP_LOGE(TAG, "fopen(%s) failed", path);
        s_sd_write_error = true;
        decide_led_status();
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Opened %s", path);
    // dump pre-header bytes (so we don't lose early header data)
    ring_dump_to_file(&s_prebuf, s_logf);
    s_since_flush = 0;
    s_state = LOGGING;
    s_logging_active = true;
    decide_led_status();
    return ESP_OK;
}

// Very light header detection: true if a line starts with "H " (or "H\t")
static bool is_blackbox_header_line(const char *line, size_t len) {
    if (len < 2) return false;
    return (line[0] == 'H' && (line[1] == ' ' || line[1] == '\t'));
}

static bool space_low_now(void)
{
    size_t total_bytes = 0, free_bytes = 0;
    if (esp_vfs_fat_info(MOUNT_POINT, &total_bytes, &free_bytes) != ESP_OK) {
        // If we can’t query, don’t flip to LOW_SPACE just because of a transient error
        return false;
    }
    return free_bytes < LOW_SPACE_BYTES;
}

static esp_err_t init_sdspi(void) {
    ESP_LOGI(TAG, "Mounting SD (SDSPI)");
    // SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64 * 1024
    };
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    esp_err_t ret = spi_bus_initialize(host.slot, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        s_sd_ok = false;
        decide_led_status();
        return ret;
    }
    s_spi_host_slot = host.slot;

    // Device (CS)
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    // Mount config
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = true
    };

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_vfs_fat_sdspi_mount failed: %s", esp_err_to_name(ret));
        s_sd_ok = false;
        decide_led_status();
        spi_bus_free(s_spi_host_slot);
        s_spi_host_slot = -1;
        return ret;
    }
    s_sd_ok = true;
    s_sd_write_error = false;
    s_sd_low_space = space_low_now();
    decide_led_status();

    sdmmc_card_print_info(stdout, s_card);
    return ESP_OK;
}

static void deinit_sdspi(void) {
    if (s_card) {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
        s_card = NULL;
    }
    if (s_spi_host_slot >= 0) {
        spi_bus_free(s_spi_host_slot);
        s_spi_host_slot = -1;
    }
    s_sd_ok = false;
    decide_led_status();
}

static esp_err_t init_uart(void) {
    uart_config_t cfg = {
        .baud_rate = BB_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if ESP_IDF_VERSION_MAJOR >= 5
        .source_clk = UART_SCLK_DEFAULT
#endif
    };
    ESP_ERROR_CHECK(uart_driver_install(BB_UART_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(BB_UART_PORT, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(BB_UART_PORT, BB_UART_TX_PIN, BB_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    return ESP_OK;
}

// ---------- LED priority resolver ----------
static void decide_led_status(void)
{
    if (s_fatal) {
        led_set_status(LED_STAT_FATAL);
        return;
    }
    if (!s_sd_ok || s_sd_write_error) {
        led_set_status(LED_STAT_SD_ERROR);
        return;
    }
    if (s_sd_low_space) {
        led_set_status(LED_STAT_LOW_SPACE);
        return;
    }
    if (s_logging_active) {
        led_set_status(LED_STAT_LOGGING);
        return;
    }
    if (!s_ever_saw_header) {
        // Before first header
        if (s_uart_quiet_ms > SERIAL_LOST_MS) {
            // Still “idle” is less alarming pre-header, but you asked to show SERIAL_LOST here
            led_set_status(LED_STAT_SERIAL_LOST);
        } else {
            led_set_status(LED_STAT_IDLE_WAITING_BB);
        }
        return;
    }
    // After header seen once this session
    if (s_uart_quiet_ms > SERIAL_LOST_MS) {
        led_set_status(LED_STAT_SERIAL_LOST);
        return;
    }
    led_set_status(LED_STAT_IDLE_WAITING_BB);
}
// -------------------------------------------

static void uart_logger_task(void *arg) {
    uint8_t buf[READ_CHUNK];
    s_last_rx_us = esp_timer_get_time();

    // Start “waiting” (resolver will pick exact pattern)
    s_logging_active = false;
    s_ever_saw_header = false;
    s_sd_low_space = space_low_now();
    decide_led_status();

    for (;;) {
        int got = uart_read_bytes(BB_UART_PORT, buf, sizeof(buf), pdMS_TO_TICKS(50));
        uint64_t now_us = esp_timer_get_time();

        // update quiet time
        s_uart_quiet_ms = (uint32_t)((now_us - s_last_rx_us) / 1000ULL);

        if (got > 0) {
            s_last_rx_us = now_us;
            s_uart_quiet_ms = 0;

            // Always push into prebuffer until we open a file
            if (s_state == WAIT_HEADER) {
                ring_put(&s_prebuf, buf, got);
            }

            // Walk bytes to detect header line starts
            for (int i = 0; i < got; ++i) {
                uint8_t b = buf[i];

                // Build line buffer to detect "H "
                if (s_at_line_start) {
                    s_linelen = 0;
                }
                if (s_linelen < BB_LINE_MAX - 1) {
                    s_line[s_linelen++] = (char)b;
                    s_line[s_linelen] = '\0';
                }

                if (b == '\n' || b == '\r') {
                    // End of line -> check
                    if (is_blackbox_header_line(s_line, s_linelen)) {
                        s_ever_saw_header = true;
                        if (s_state == WAIT_HEADER) {
                            if (open_new_file_and_dump_prebuf() != ESP_OK) {
                                ESP_LOGE(TAG, "Failed to open file");
                                // s_sd_write_error already set in open_new_file...
                            }
                        } else {
                            // Optional: split per header
                            // indicate_file_closing_then_idle();
                            // close_current_file();
                            // open_new_file_and_dump_prebuf();
                        }
                        decide_led_status();
                    }
                    s_at_line_start = true;
                    s_linelen = 0;
                } else {
                    s_at_line_start = false;
                }
            }

            // Write raw stream to file if logging
            if (s_state == LOGGING && s_logf) {
                size_t w = fwrite(buf, 1, got, s_logf);
                if (w != (size_t)got || ferror(s_logf)) {
                    ESP_LOGE(TAG, "Write error");
                    s_sd_write_error = true;
                    decide_led_status();
                } else {
                    s_since_flush += w;
                    if (s_since_flush >= FLUSH_BYTES) {
                        if (fflush(s_logf) != 0) {
                            ESP_LOGE(TAG, "fflush error");
                            s_sd_write_error = true;
                        }
                        s_since_flush = 0;
                    }
                }
            }
        } else {
            // timeout: check idle close
            if (s_state == LOGGING && (now_us - s_last_rx_us) >= (uint64_t)IDLE_TIMEOUT_MS * 1000ULL) {
                ESP_LOGI(TAG, "Idle timeout reached");
                indicate_file_closing_then_idle();
                close_current_file();
            }
        }

        // Periodic low-space check (cheap)
        static uint32_t last_space_check_ms = 0;
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
        if ((now_ms - last_space_check_ms) > 2000) { // every 2s
            last_space_check_ms = now_ms;
            bool low = space_low_now();
            if (low != s_sd_low_space) {
                s_sd_low_space = low;
                decide_led_status();
            }
        }

        // Update LED if quiet state changed meaningfully
        static uint32_t prev_quiet_bucket = 0;
        uint32_t bucket = (s_uart_quiet_ms > SERIAL_LOST_MS) ? 1 : 0;
        if (bucket != prev_quiet_bucket) {
            prev_quiet_bucket = bucket;
            decide_led_status();
        }
    }
}

void main(void) {



    // LED: boot pattern
    led_status_init();
    led_set_status(LED_STAT_BOOTING);

    ESP_LOGI(TAG, "Starting Serial Blackbox logger");

    // Mount SD
    if (init_sdspi() != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed; restarting in 3s");
        vTaskDelay(pdMS_TO_TICKS(3000));
        deinit_sdspi();
        esp_restart();
    }

    // Prepare next filename index by scanning (optional quick pass)
    for (int i = 1; i <= MAX_FILE_INDEX; ++i) {
        char p[64];
        snprintf(p, sizeof(p), MOUNT_POINT"/LOG%04d.BFL", i);
        if (!path_exists(p)) {
            s_next_index = i;
            break;
        }
    }

    // Init UART
    if (init_uart() != ESP_OK) {
        s_fatal = true;
        decide_led_status();
        ESP_LOGE(TAG, "UART init failed; halting");
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    // From here, the UART task will drive status changes
    xTaskCreatePinnedToCore(uart_logger_task, "uart_logger", 4096, NULL, 5, NULL, tskNO_AFFINITY);

    // After boot, let resolver pick the steady-state
    vTaskDelay(pdMS_TO_TICKS(1500));
    decide_led_status();
}
