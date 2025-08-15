#pragma once
#include "storage.h"
#include "flash.h"
// Enable SD later with -DLAGER32_SD and provide SdLogSink
#ifdef LAGER32_SD
  #include "sd.h"
#endif
#include "settings.h"

class StorageMgr {
public:
  static StorageMgr& instance();

  bool begin(bool formatIfNeeded = false);              // init flash, and SD if compiled
  bool select(StorageKind kind);                        // switch current sink
  ILogSink* current()            { return curr_; }
  StorageKind kind()       const { return kind_; }

private:
  StorageMgr() = default;
  StorageKind kind_ = StorageKind::Flash;
  FlashLogSink flash_;
#ifdef LAGER32_SD
  SdLogSink    sd_;
#endif
  ILogSink*    curr_ = &flash_;
};
