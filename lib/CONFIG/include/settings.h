#pragma once
#include <Arduino.h>
#include <Preferences.h>
enum class StorageKind : uint8_t { Flash = 0, SD = 1 };

namespace Settings {
  bool load(StorageKind& out);     // returns true if found, sets 'out'
  void save(StorageKind kind);
  const char* toString(StorageKind k);           // "flash" | "sd"
  StorageKind fromString(const String& s);       // default Flash
}
