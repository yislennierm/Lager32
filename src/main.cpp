// Arduino SD hot-plug demo for ESP32-S2 (Lolin S2 Mini)
// - Uses Arduino SD library over SPI
// - No CD pin: polls & retries with backoff
// - Writes to /LOG0001.TXT (then 0002, ...); resumes after reinsert

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// ---- Wire these to your board (Lolin S2 Mini defaults) ----
#define PIN_SCK 36
#define PIN_MISO 37
#define PIN_MOSI 35
#define PIN_CS 34

SPIClass sdSPI(FSPI);

// ---- Hot-plug state ----
static bool g_sd_ok = false;
static uint32_t g_next_try_ms = 0;
static uint32_t g_backoff_ms = 250;          // start small
static const uint32_t BACKOFF_MAX_MS = 8000; // cap retries

// ---- Logging state ----
File logFile;
uint32_t lastWriteMs = 0;
int lineCounter = 0;

// ---- Helpers ----
static bool sd_try_mount()
{
    sdSPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
    // 20 MHz is usually safe; lower if your wiring is long/iffy
    if (!SD.begin(PIN_CS, sdSPI, 20000000))
    {
        SD.end();
        return false;
    }
    if (SD.cardType() == CARD_NONE)
    {
        SD.end();
        return false;
    }
    return true;
}

static void sd_mark_broken_and_unmount(const char *why)
{
    if (logFile)
    {
        logFile.flush();
        logFile.close();
    }
    SD.end();
    g_sd_ok = false;
    Serial.printf("[SD] unmounted (%s). Will retry.\n", why);
}

// Find next /LOG0001.TXT .. /LOG9999.TXT that doesn't exist
static bool find_next_log(char *path, size_t pathlen)
{
    for (int i = 1; i <= 9999; ++i)
    {
        snprintf(path, pathlen, "/LOG%04d.TXT", i);
        if (!SD.exists(path))
            return true;
    }
    return false;
}

static bool open_log_if_needed()
{
    if (logFile)
        return true;
    char path[24];
    if (!find_next_log(path, sizeof(path)))
    {
        Serial.println("[SD] No free filename slots.");
        return false;
    }
    logFile = SD.open(path, FILE_WRITE);
    if (!logFile)
    {
        Serial.printf("[SD] Open %s failed.\n", path);
        return false;
    }
    Serial.printf("[SD] Logging to %s\n", path);
    lineCounter = 0;
    lastWriteMs = 0;
    return true;
}

// ---- Non-blocking SD service: call often ----
static void sd_service()
{
    uint32_t now = millis();
    if (!g_sd_ok)
    {
        if ((int32_t)(now - g_next_try_ms) >= 0)
        {
            if (sd_try_mount())
            {
                g_sd_ok = true;
                g_backoff_ms = 250;
                Serial.println("[SD] mounted.");
            }
            else
            {
                g_next_try_ms = now + g_backoff_ms;
                g_backoff_ms = min(g_backoff_ms * 2, BACKOFF_MAX_MS);
                // Serial.printf("[SD] mount failed, retry in %ums\n", g_backoff_ms);
            }
        }
    }
}

// ---- Arduino entry points ----
void setup()
{
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && (millis() - t0) < 3000)
        delay(10);
    Serial.println("\n=== SD hot-plug demo (Arduino SD over SPI) ===");

    // Start trying to mount right away
    sd_service();
}

void loop()
{
    sd_service();

    if (!g_sd_ok)
    {
        // Not mounted: make sure file is closed, and show status occasionally
        static uint32_t lastMsg = 0;
        if (logFile)
        {
            logFile.close();
        }
        if (millis() - lastMsg > 2000)
        {
            lastMsg = millis();
            Serial.println("[SD] Waiting for card…");
        }
        delay(50);
        return;
    }

    // Mounted: ensure we have a log file
    if (!open_log_if_needed())
    {
        // Can't open a file right now; try again later
        delay(100);
        return;
    }

    // Write once per second
    uint32_t now = millis();
    if (now - lastWriteMs >= 1000)
    {
        lastWriteMs = now;

        char line[64];
        int n = snprintf(line, sizeof(line), "t=%lu ms, line=%d\r\n",
        (unsigned long)now, lineCounter++);
        size_t w = logFile.write((const uint8_t *)line, (size_t)n);

        if (w != (size_t)n)
        {
            Serial.println("[SD] write error, unmounting.");
            sd_mark_broken_and_unmount("write fail");
            return;
        }

        

        Serial.printf("[SD] wrote %d bytes\n", (int)w);
    }

    delay(10); // keep it light
}
