#pragma once
// Main Arduino compatibility shim for Emscripten web emulator.
// Provides the subset of Arduino API used by CrossPoint firmware.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "HardwareSerial.h"
#include "IPAddress.h"
#include "Print.h"
#include "SPI.h"
#include "Stream.h"
#include "WString.h"
#include "pgmspace.h"

// Arduino pin modes
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2
#define INPUT_PULLDOWN 0x3

// Digital values
#define HIGH 0x1
#define LOW 0x0

// Timing
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void yield();

// GPIO stubs
inline void pinMode(uint8_t pin, uint8_t mode) {
  (void)pin;
  (void)mode;
}

inline int digitalRead(uint8_t pin) {
  (void)pin;
  return LOW;
}

inline void digitalWrite(uint8_t pin, uint8_t val) {
  (void)pin;
  (void)val;
}

inline int analogRead(uint8_t pin) {
  (void)pin;
  return 0;
}

// Provide min/max as global functions via std::min/std::max (no macros to avoid conflicts)
using std::min;
using std::max;
#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif
#ifndef map
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
#endif
#ifndef abs
#define abs(x) ((x) >= 0 ? (x) : -(x))
#endif

// ESP class stub for heap info logging in main loop
class EspClass {
 public:
  uint32_t getFreeHeap() { return 256 * 1024; }
  uint32_t getHeapSize() { return 512 * 1024; }
  uint32_t getMinFreeHeap() { return 128 * 1024; }
  const char* getSdkVersion() { return "emulator"; }
  void restart() {}
};

extern EspClass ESP;

// Random
inline long random(long max) { return rand() % max; }
inline long random(long min, long max) { return min + rand() % (max - min); }
inline void randomSeed(unsigned long seed) { srand(seed); }

// Bit manipulation
#define bit(b) (1UL << (b))
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

// String conversion
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2
