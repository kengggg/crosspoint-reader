#pragma once
// QRCode stub for Emscripten web emulator.
// Used by CrossPointWebServerActivity for WiFi QR codes (non-functional in emulator).

#include <cstdint>

typedef struct {
  uint8_t version;
  uint8_t size;
  uint8_t ecc;
  uint8_t mode;
  uint8_t mask;
  uint8_t modules[177 * 177 / 8 + 1];
  uint16_t bufferSize;
} QRCode;

inline int qrcode_getBufferSize(uint8_t version) {
  (void)version;
  return 4096;
}

inline int8_t qrcode_initText(QRCode* qrcode, uint8_t* buffer, uint8_t version, uint8_t ecc, const char* data) {
  (void)qrcode;
  (void)buffer;
  (void)version;
  (void)ecc;
  (void)data;
  return 0;
}

inline bool qrcode_getModule(const QRCode* qrcode, uint8_t x, uint8_t y) {
  (void)qrcode;
  (void)x;
  (void)y;
  return false;
}

#define ECC_LOW 0
#define ECC_MEDIUM 1
#define ECC_QUARTILE 2
#define ECC_HIGH 3
