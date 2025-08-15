#pragma once
#include <Arduino.h>
#include <vector>

struct LogFileInfo {
  String name;
  size_t size;
  time_t mtime;
};

enum class LogCommand : uint8_t { Start, Stop, Mark };

class ILogSink {
public:
  virtual ~ILogSink() = default;

  // lifecycle
  virtual bool begin(bool formatIfNeeded = false) = 0;

  // file control
  virtual bool startFile(const char* suggestedBaseName = nullptr) = 0;
  virtual void stopFile() = 0;

  // writes -------------- required by log_writer.cpp
  virtual size_t write(const uint8_t* data, size_t len) = 0;
  virtual void   flush() = 0;

  // mgmt
  virtual std::vector<LogFileInfo> list() = 0;
  virtual bool    remove(const char* name) = 0;
  virtual bool    exists(const char* name) = 0;
  virtual String  pathOf(const char* name) = 0;
  virtual bool    wipeAll() = 0;
  virtual size_t  freeSpace() = 0;
};
