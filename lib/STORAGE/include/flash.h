#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "storage.h"

// Concrete storage sink that writes to internal flash via LittleFS
class FlashLogSink : public ILogSink {
public:
  bool   begin(bool formatIfNeeded = false) override;
  bool   startFile(const char* suggestedBaseName = nullptr) override;
  void   stopFile() override;
  size_t write(const uint8_t* data, size_t len) override;
  void   flush() override;

  std::vector<LogFileInfo> list() override;
  bool   remove(const char* name) override;
  bool   exists(const char* name) override;
  String pathOf(const char* name) override;
  bool   wipeAll() override;
  size_t freeSpace() override;

private:
  File file_;
};
