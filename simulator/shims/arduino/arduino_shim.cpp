// Arduino shim implementations for Emscripten web simulator.

#include "Arduino.h"

#include <emscripten.h>

HardwareSerial Serial;
SPIClass SPI;
EspClass ESP;

unsigned long millis() {
  return static_cast<unsigned long>(emscripten_get_now());
}

unsigned long micros() {
  return static_cast<unsigned long>(emscripten_get_now() * 1000.0);
}

void delay(unsigned long ms) {
  // In ASYNCIFY mode, this yields to the browser event loop for ms milliseconds.
  // Without ASYNCIFY, this is a no-op (tight spin would block the browser).
#ifdef __EMSCRIPTEN_ASYNCIFY__
  emscripten_sleep(ms);
#else
  (void)ms;
#endif
}

void yield() {
  // Give browser a chance to run events
#ifdef __EMSCRIPTEN_ASYNCIFY__
  emscripten_sleep(0);
#endif
}
