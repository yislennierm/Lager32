#include "flash.h"

bool FlashLogSink::begin(bool formatIfNeeded) {
  return LittleFS.begin(formatIfNeeded);
}

bool FlashLogSink::startFile(const char* suggestedBaseName) {
  if (file_) file_.close();

  String base = suggestedBaseName ? String(suggestedBaseName) : String("log");
  // /log0000.bbl .. /log9999.bbl
  for (int i = 0; i < 10000; ++i) {
    char idx[5]; snprintf(idx, sizeof(idx), "%04d", i);
    currentName_ = "/" + base + idx + ".bbl";
    if (!LittleFS.exists(currentName_)) break;
  }

  file_ = LittleFS.open(currentName_, "w");  // write new file
  return (bool)file_;
}

void FlashLogSink::stopFile() {
  if (file_) { file_.flush(); file_.close(); }
}

size_t FlashLogSink::write(const uint8_t* data, size_t len) {
  return file_ ? file_.write(data, len) : 0;
}

void FlashLogSink::flush() {
  if (file_) file_.flush();
}

std::vector<LogFileInfo> FlashLogSink::list() {
  std::vector<LogFileInfo> out;
  File root = LittleFS.open("/");
  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    LogFileInfo i; i.name = f.name(); i.size = f.size(); i.mtime = 0;
    out.push_back(i);
  }
  return out;
}

bool FlashLogSink::remove(const char* name) { return LittleFS.remove(name); }
bool FlashLogSink::exists(const char* name) { return LittleFS.exists(name); }
String FlashLogSink::pathOf(const char* name) { return String(name); }

bool FlashLogSink::wipeAll() {
  File root = LittleFS.open("/");
  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    LittleFS.remove(f.name());
  }
  return true;
}

size_t FlashLogSink::freeSpace() {
  size_t total = LittleFS.totalBytes();
  size_t used  = LittleFS.usedBytes();
  return (total > used) ? (total - used) : 0;
}
