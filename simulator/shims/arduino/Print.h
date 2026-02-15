#pragma once
// Arduino Print class shim for Emscripten web simulator

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "WString.h"

class Print {
 public:
  virtual ~Print() = default;

  virtual size_t write(uint8_t c) = 0;
  virtual size_t write(const uint8_t* buffer, size_t size) {
    size_t n = 0;
    while (size--) {
      if (write(*buffer++)) n++;
      else break;
    }
    return n;
  }
  size_t write(const char* str) {
    if (!str) return 0;
    return write(reinterpret_cast<const uint8_t*>(str), strlen(str));
  }
  virtual void flush() {}

  size_t print(const char* s) { return write(s); }
  size_t print(const String& s) { return write(s.c_str()); }
  size_t print(char c) { return write(static_cast<uint8_t>(c)); }
  size_t print(int n) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", n);
    return write(buf);
  }
  size_t print(unsigned int n) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", n);
    return write(buf);
  }
  size_t print(long n) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%ld", n);
    return write(buf);
  }
  size_t print(unsigned long n) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%lu", n);
    return write(buf);
  }

  size_t println() { return write("\r\n"); }
  size_t println(const char* s) { return print(s) + println(); }
  size_t println(const String& s) { return print(s) + println(); }
  size_t println(int n) { return print(n) + println(); }
  size_t println(unsigned long n) { return print(n) + println(); }

  int printf(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) write(reinterpret_cast<const uint8_t*>(buf), len > 511 ? 511 : len);
    return len;
  }
};
