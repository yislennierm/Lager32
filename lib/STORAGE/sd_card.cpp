#include "sd_card.h"

static StreamBufferHandle_t sBuf = nullptr;
static QueueHandle_t        sCmdQ = nullptr;
static File sFile;
static volatile bool sLogging = false;

// TODO: set your SD SPI pins here (SPI mode)
static const int PIN_SD_CS   = 5;   // change for your board
// Make sure SPI pins are wired to default VSPI (SCLK=18, MISO=19, MOSI=23 on many ESP32s);
// ESP32-S2 pinout differs—adjust for your hardware/wiring.

bool SdLog::isLogging() { return sLogging; }

void SdLog::begin(StreamBufferHandle_t buf, QueueHandle_t cmdQ) {
  sBuf = buf; sCmdQ = cmdQ;

  if (!SD.begin(PIN_SD_CS)) {
    // No SD; we’ll still run but Start will fail
  }

  xTaskCreate([](void*){
    uint8_t chunk[2048];
    uint32_t sinceFlush = 0;

    for (;;) {
      // Commands first
      SdLogCommand cmd;
      if (xQueueReceive(sCmdQ, &cmd, 0) == pdTRUE) {
        if (cmd.type == SdLogCommand::Start) {
          if (sFile) sFile.close();
          sFile = SD.open("/log.bin", FILE_WRITE);
          sLogging = sFile ? true : false;
        } else if (cmd.type == SdLogCommand::Stop) {
          if (sFile) { sFile.flush(); sFile.close(); }
          sLogging = false;
        } else if (cmd.type == SdLogCommand::Mark) {
          if (sFile) {
            uint32_t mark = 0xDEADBEEF;
            sFile.write((uint8_t*)&mark, sizeof(mark));
          }
        }
      }

      size_t got = 0;
      if (sLogging && sFile) {
        got = xStreamBufferReceive(sBuf, chunk, sizeof(chunk), 50 / portTICK_PERIOD_MS);
        if (got) {
          sFile.write(chunk, got);
          sinceFlush += got;
          if (sinceFlush >= 16 * 1024) { sFile.flush(); sinceFlush = 0; }
        }
      } else {
        // drain so buffer doesn’t grow forever
        (void)xStreamBufferReceive(sBuf, chunk, sizeof(chunk), 10 / portTICK_PERIOD_MS);
      }
    }
  }, "sd_writer", 6144, nullptr, 2, nullptr);
}
