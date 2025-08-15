#pragma once
#define BB_WEBUI_ONLY 1      // build without RF
#define TARGET_RX 1          // so RxConfig is visible to devWIFI.cpp

// already added earlier:
#define Regulatory_Domain_ISM_2400
#define DEVICE_NAME "BB-Blackbox"
#define LATEST_COMMIT  'l','o','c','a','l'
#define LATEST_VERSION '0','.','0','-','b','b'
#ifndef VERSION
#define VERSION "0.0-bb"
#endif
