#include "log_writer.h"

static StreamBufferHandle_t sBuf = nullptr;
static QueueHandle_t sCmdQ = nullptr;
static ILogSink* sSink = nullptr;

static void logWriterTask(void*) {
  uint8_t chunk[1024];

  for (;;) {
    // process commands (non-blocking)
    LogCommand cmd;
    if (xQueueReceive(sCmdQ, &cmd, 0) == pdTRUE) {
      switch (cmd) {
        case LogCommand::Start:
          if (sSink) sSink->startFile(nullptr);
          break;
        case LogCommand::Stop:
          if (sSink) sSink->stopFile();
          break;
        case LogCommand::Mark: {
          static const char mark[] = "#MARK\n";
          if (sSink) {
            sSink->write(reinterpret_cast<const uint8_t*>(mark), sizeof(mark) - 1);
            sSink->flush();
          }
        } break;
      }
    }

    // pull bytes from UART buffer and write
    size_t n = xStreamBufferReceive(sBuf, chunk, sizeof(chunk), 50 / portTICK_PERIOD_MS);
    if (n > 0 && sSink) {
      sSink->write(chunk, n);
    }
  }
}

void LogWriter::begin(StreamBufferHandle_t buf, QueueHandle_t cmdQ, ILogSink* sink) {
  sBuf = buf;
  sCmdQ = cmdQ;
  sSink = sink;
  xTaskCreate(logWriterTask, "log_writer", 4096, nullptr, 3, nullptr);
}
