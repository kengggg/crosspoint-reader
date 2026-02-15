#pragma once
// WiFiClientSecure stub for Emscripten web simulator.

#include "WiFiClient.h"

class WiFiClientSecure : public WiFiClient {
 public:
  void setCACert(const char* rootCA) { (void)rootCA; }
  void setInsecure() {}
};
