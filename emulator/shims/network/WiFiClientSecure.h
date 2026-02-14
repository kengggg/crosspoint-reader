#pragma once
// WiFiClientSecure stub for Emscripten web emulator.

#include "WiFiClient.h"

class WiFiClientSecure : public WiFiClient {
 public:
  void setCACert(const char* rootCA) { (void)rootCA; }
  void setInsecure() {}
};
