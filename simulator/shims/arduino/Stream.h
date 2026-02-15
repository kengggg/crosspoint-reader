#pragma once
// Arduino Stream class shim for Emscripten web simulator

#include "Print.h"

class Stream : public Print {
 public:
  virtual ~Stream() = default;
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;

  // Timeout for readBytes etc.
  void setTimeout(unsigned long timeout) { timeout_ = timeout; }
  unsigned long getTimeout() const { return timeout_; }

  size_t readBytes(char* buffer, size_t length) {
    size_t count = 0;
    while (count < length) {
      int c = read();
      if (c < 0) break;
      *buffer++ = static_cast<char>(c);
      count++;
    }
    return count;
  }

  size_t readBytes(uint8_t* buffer, size_t length) {
    return readBytes(reinterpret_cast<char*>(buffer), length);
  }

  String readString() {
    String ret;
    int c = read();
    while (c >= 0) {
      ret += static_cast<char>(c);
      c = read();
    }
    return ret;
  }

  String readStringUntil(char terminator) {
    String ret;
    int c = read();
    while (c >= 0 && c != terminator) {
      ret += static_cast<char>(c);
      c = read();
    }
    return ret;
  }

 protected:
  unsigned long timeout_ = 1000;
};
