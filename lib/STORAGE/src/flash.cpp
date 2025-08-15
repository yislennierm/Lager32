#include "flash.h"
#include <FS.h>        // ensures File APIs are available

bool FlashLogSink::begin(bool formatIfNeeded) {
  return LittleFS.begin(formatIfNeeded);
}

bool FlashLogSink::startFile(const char* suggestedBaseName) {
  stopFile();

  const String base = (suggestedBaseName && suggestedBaseName[0])
                        ? String(suggestedBaseName)
                        : String("log");

  // Create /log0000.bbl .. /log9999.bbl (or <base>####.bbl)
  String candidate;
  for (int i = 0; i < 10000; ++i) {
    char idx[5];
    snprintf(idx, sizeof(idx), "%04d", i);
    candidate = "/";
    candidate += base;
    if (!candidate.endsWith(".bbl")) candidate += idx;
    if (!candidate.endsWith(".bbl")) candidate += ".bbl";
    if (!LittleFS.exists(candidate)) break;
  }

  file_ = LittleFS.open(candidate, "w");
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
    if (!f.isDirectory()) {
      LogFileInfo i;
      i.name = String(f.name());
      i.size = (size_t)f.size();
      out.push_back(std::move(i));
    }
    f.close();
  }
  root.close();
  return out;
}

bool FlashLogSink::remove(const char* name) {
  return LittleFS.remove(pathOf(name));
}

bool FlashLogSink::exists(const char* name) {
  return LittleFS.exists(pathOf(name));
}

String FlashLogSink::pathOf(const char* name) {
  if (!name || !name[0]) return String();
  if (name[0] == '/') return String(name);
  return String("/") + name;
}

bool FlashLogSink::wipeAll() {
  // Quick-and-dirty wipe: format via begin(true)
  LittleFS.end();
  return LittleFS.begin(true);
}

size_t FlashLogSink::freeSpace() {
  // Arduino-ESP32 LittleFS API doesn’t expose free space reliably on all cores.
  // If your core provides totalBytes/usedBytes, you can uncomment:
  // size_t total = LittleFS.totalBytes();
  // size_t used  = LittleFS.usedBytes();
  // return (total > used) ? (total - used) : 0;
  return 0;
}
