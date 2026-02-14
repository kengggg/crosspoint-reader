#pragma once
// WiFiUDP stub for Emscripten web emulator.

#include <cstdint>
#include <cstddef>

class WiFiUDP {
 public:
  uint8_t begin(uint16_t port) {
    (void)port;
    return 0;
  }
  void stop() {}
  int parsePacket() { return 0; }
  int read(unsigned char* buffer, size_t len) {
    (void)buffer;
    (void)len;
    return 0;
  }
  size_t write(const uint8_t* buffer, size_t size) {
    (void)buffer;
    (void)size;
    return 0;
  }
};
