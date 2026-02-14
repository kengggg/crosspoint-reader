#pragma once
// WiFiClient stub for Emscripten web emulator.

#include "Stream.h"

class WiFiClient : public Stream {
 public:
  WiFiClient() = default;
  virtual ~WiFiClient() = default;

  int connect(const char* host, uint16_t port) {
    (void)host;
    (void)port;
    return 0;
  }
  bool connected() { return false; }
  void stop() {}
  operator bool() { return false; }

  size_t write(uint8_t c) override {
    (void)c;
    return 0;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
};
