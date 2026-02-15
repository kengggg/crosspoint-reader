#pragma once
// Arduino HardwareSerial shim for Emscripten web simulator
// Routes Serial output to browser console via printf.

#include <cstdarg>
#include <cstdio>

#include "Print.h"
#include "Stream.h"

// Declare Arduino global functions so files that only include HardwareSerial.h
// (not Arduino.h) can still call millis(), delay(), etc.
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void yield();

class HardwareSerial : public Stream {
 public:
  void begin(unsigned long baud) { (void)baud; }
  void end() {}

  operator bool() const { return true; }

  size_t write(uint8_t c) override {
    putchar(c);
    return 1;
  }
  size_t write(const uint8_t* buffer, size_t size) override {
    return fwrite(buffer, 1, size, stdout);
  }

  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }

  using Print::printf;
  using Print::println;
  using Print::print;
};

extern HardwareSerial Serial;
