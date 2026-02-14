#pragma once
// Base64 stub for Emscripten web emulator.

#include "WString.h"
#include <cstdint>
#include <cstring>

inline String base64_encode(const uint8_t* data, size_t length) {
  static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String result;
  for (size_t i = 0; i < length; i += 3) {
    uint32_t n = (static_cast<uint32_t>(data[i]) << 16);
    if (i + 1 < length) n |= (static_cast<uint32_t>(data[i + 1]) << 8);
    if (i + 2 < length) n |= static_cast<uint32_t>(data[i + 2]);

    result += table[(n >> 18) & 0x3F];
    result += table[(n >> 12) & 0x3F];
    result += (i + 1 < length) ? table[(n >> 6) & 0x3F] : '=';
    result += (i + 2 < length) ? table[n & 0x3F] : '=';
  }
  return result;
}

inline String base64_encode(const String& text) {
  return base64_encode(reinterpret_cast<const uint8_t*>(text.c_str()), text.length());
}

// Provide base64 namespace used by some ESP32 code
namespace base64 {
inline String encode(const uint8_t* data, size_t length) { return base64_encode(data, length); }
inline String encode(const String& text) { return base64_encode(text); }
inline String encode(const char* text) { return base64_encode(reinterpret_cast<const uint8_t*>(text), strlen(text)); }
}
