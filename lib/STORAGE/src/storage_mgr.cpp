#include "storage_mgr.h"
#include <Arduino.h>

StorageMgr& StorageMgr::instance() {
  static StorageMgr g;
  return g;
}

bool StorageMgr::begin(bool formatIfNeeded) {
  bool ok = flash_.begin(formatIfNeeded);
#ifdef LAGER32_SD
  ok = ok && sd_.begin(false); // don’t format SD automatically
#endif
  curr_ = &flash_; // default until load()
  kind_ = StorageKind::Flash;
  return ok;
}

bool StorageMgr::select(StorageKind k) {
  ILogSink* next = &flash_;
#ifdef LAGER32_SD
  if (k == StorageKind::SD) next = &sd_;
#endif
  curr_ = next;
  kind_ = k;
  return true;
}
