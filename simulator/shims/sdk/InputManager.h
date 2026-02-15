#pragma once
// InputManager stub for Emscripten web simulator.
// The actual button state is managed by HalGPIO_Web.cpp.
// This stub satisfies the #include in HalGPIO.h but the InputManager member
// is excluded via #if CROSSPOINT_EMULATED == 0 guard.

#include <cstdint>

class InputManager {
 public:
  static constexpr uint8_t POWER_BUTTON_PIN = 3;

  void begin() {}
  void update() {}
  bool isPressed(uint8_t buttonIndex) const {
    (void)buttonIndex;
    return false;
  }
  bool wasPressed(uint8_t buttonIndex) const {
    (void)buttonIndex;
    return false;
  }
  bool wasAnyPressed() const { return false; }
  bool wasReleased(uint8_t buttonIndex) const {
    (void)buttonIndex;
    return false;
  }
  bool wasAnyReleased() const { return false; }
  unsigned long getHeldTime() const { return 0; }
};
