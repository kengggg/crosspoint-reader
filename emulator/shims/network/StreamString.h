#pragma once
// StreamString stub for Emscripten web emulator.

#include "Stream.h"
#include "WString.h"

class StreamString : public Stream {
 public:
  size_t write(uint8_t c) override {
    str_ += static_cast<char>(c);
    return 1;
  }
  int available() override { return static_cast<int>(str_.length() - pos_); }
  int read() override {
    if (pos_ < str_.length()) return str_[pos_++];
    return -1;
  }
  int peek() override {
    if (pos_ < str_.length()) return str_[pos_];
    return -1;
  }
  const String& readString() { return str_; }
  const char* c_str() const { return str_.c_str(); }

 private:
  String str_;
  unsigned int pos_ = 0;
};
