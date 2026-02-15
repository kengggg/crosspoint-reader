#pragma once
// SPI shim for Emscripten web simulator - all operations are no-ops.

#include <cstdint>

class SPIClass {
 public:
  void begin(int sclk = -1, int miso = -1, int mosi = -1, int ss = -1) {
    (void)sclk;
    (void)miso;
    (void)mosi;
    (void)ss;
  }
  void end() {}
  void beginTransaction(void*) {}
  void endTransaction() {}
  uint8_t transfer(uint8_t data) { return data; }
};

extern SPIClass SPI;
