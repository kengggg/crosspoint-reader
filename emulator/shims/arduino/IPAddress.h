#pragma once
// IPAddress shim for Emscripten web emulator

#include <cstdint>
#include <cstdio>

#include "Print.h"
#include "WString.h"

class IPAddress {
 public:
  IPAddress() : addr_{0, 0, 0, 0} {}
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : addr_{a, b, c, d} {}

  uint8_t operator[](int index) const { return addr_[index]; }
  uint8_t& operator[](int index) { return addr_[index]; }

  operator uint32_t() const {
    return static_cast<uint32_t>(addr_[0]) | (static_cast<uint32_t>(addr_[1]) << 8) |
           (static_cast<uint32_t>(addr_[2]) << 16) | (static_cast<uint32_t>(addr_[3]) << 24);
  }

  String toString() const {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", addr_[0], addr_[1], addr_[2], addr_[3]);
    return String(buf);
  }

 private:
  uint8_t addr_[4];
};
