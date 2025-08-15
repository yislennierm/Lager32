#include "settings.h"
#include <Preferences.h>

static const char* NS   = "lager32";
static const char* KEY  = "storage"; // uint8_t 0=flash 1=sd

bool Settings::load(StorageKind& out) {
  Preferences p;
  if (!p.begin(NS, /*readOnly=*/true)) return false;
  uint8_t v = p.getUChar(KEY, 0xFF);
  p.end();
  if (v == 0xFF) return false;
  out = (v == 1) ? StorageKind::SD : StorageKind::Flash;
  return true;
}

void Settings::save(StorageKind kind) {
  Preferences p;
  if (!p.begin(NS, /*readOnly=*/false)) return;
  p.putUChar(KEY, (kind == StorageKind::SD) ? 1 : 0);
  p.end();
}

const char* Settings::toString(StorageKind k) {
  return (k == StorageKind::SD) ? "sd" : "flash";
}

StorageKind Settings::fromString(const String& s) {
  return (s == "sd") ? StorageKind::SD : StorageKind::Flash;
}
