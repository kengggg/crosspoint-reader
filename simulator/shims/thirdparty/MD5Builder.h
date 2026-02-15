#pragma once
// MD5Builder stub for Emscripten web simulator.
// Used by KOReaderSync for document ID and credential hashing.
// Provides a basic MD5-like hash (not cryptographically accurate but functional).

#include "WString.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

class MD5Builder {
 public:
  void begin() { hash_ = 0x12345678; }

  void add(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
      hash_ = hash_ * 31 + data[i];
    }
  }
  void add(const char* data) { add(reinterpret_cast<const uint8_t*>(data), strlen(data)); }
  void add(const String& data) { add(data.c_str()); }

  void calculate() {
    // Hash is already computed incrementally
  }

  String toString() {
    char buf[33];
    snprintf(buf, sizeof(buf), "%08x%08x%08x%08x", (uint32_t)(hash_ >> 96), (uint32_t)(hash_ >> 64),
             (uint32_t)(hash_ >> 32), (uint32_t)hash_);
    // Use a simpler approach since we don't have 128-bit int
    snprintf(buf, sizeof(buf), "%08x%08x%08x%08x",
             static_cast<uint32_t>(hash_),
             static_cast<uint32_t>(hash_ ^ 0xdeadbeef),
             static_cast<uint32_t>(hash_ ^ 0xcafebabe),
             static_cast<uint32_t>(hash_ ^ 0x13371337));
    return String(buf);
  }

  void getBytes(uint8_t* output) {
    uint32_t parts[4] = {
        static_cast<uint32_t>(hash_),
        static_cast<uint32_t>(hash_ ^ 0xdeadbeef),
        static_cast<uint32_t>(hash_ ^ 0xcafebabe),
        static_cast<uint32_t>(hash_ ^ 0x13371337)};
    memcpy(output, parts, 16);
  }

 private:
  uint64_t hash_ = 0;
};
