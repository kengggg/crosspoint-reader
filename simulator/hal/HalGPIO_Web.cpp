// HalGPIO implementation for Emscripten web simulator.
// This file replaces lib/hal/HalGPIO.cpp.
// Button input comes from keyboard events via JS callbacks.

#include <HalGPIO.h>
#include <emscripten.h>
#include <cstdio>
#include <cstring>

// Button state management
static bool g_button_pressed[7] = {};     // Current state
static bool g_button_was_pressed[7] = {}; // Edge detection: was pressed this frame
static bool g_button_was_released[7] = {};// Edge detection: was released this frame
static bool g_button_prev[7] = {};        // Previous frame state
static unsigned long g_held_start = 0;    // When the last button was pressed
static int g_held_button = -1;            // Which button is being held

// Called from JS when a key is pressed/released
extern "C" {
  EMSCRIPTEN_KEEPALIVE
  void simulator_button_down(int buttonIndex) {
    if (buttonIndex >= 0 && buttonIndex < 7) {
      g_button_pressed[buttonIndex] = true;
    }
  }

  EMSCRIPTEN_KEEPALIVE
  void simulator_button_up(int buttonIndex) {
    if (buttonIndex >= 0 && buttonIndex < 7) {
      g_button_pressed[buttonIndex] = false;
    }
  }
}

void HalGPIO::begin() {
  memset(g_button_pressed, 0, sizeof(g_button_pressed));
  memset(g_button_was_pressed, 0, sizeof(g_button_was_pressed));
  memset(g_button_was_released, 0, sizeof(g_button_was_released));
  memset(g_button_prev, 0, sizeof(g_button_prev));
  printf("[SIM] HalGPIO initialized (keyboard input)\n");
}

void HalGPIO::update() {
  for (int i = 0; i < 7; i++) {
    g_button_was_pressed[i] = g_button_pressed[i] && !g_button_prev[i];
    g_button_was_released[i] = !g_button_pressed[i] && g_button_prev[i];

    if (g_button_was_pressed[i]) {
      g_held_start = millis();
      g_held_button = i;
    }

    g_button_prev[i] = g_button_pressed[i];
  }
}

bool HalGPIO::isPressed(uint8_t buttonIndex) const {
  if (buttonIndex >= 7) return false;
  return g_button_pressed[buttonIndex];
}

bool HalGPIO::wasPressed(uint8_t buttonIndex) const {
  if (buttonIndex >= 7) return false;
  return g_button_was_pressed[buttonIndex];
}

bool HalGPIO::wasAnyPressed() const {
  for (int i = 0; i < 7; i++) {
    if (g_button_was_pressed[i]) return true;
  }
  return false;
}

bool HalGPIO::wasReleased(uint8_t buttonIndex) const {
  if (buttonIndex >= 7) return false;
  return g_button_was_released[buttonIndex];
}

bool HalGPIO::wasAnyReleased() const {
  for (int i = 0; i < 7; i++) {
    if (g_button_was_released[i]) return true;
  }
  return false;
}

unsigned long HalGPIO::getHeldTime() const {
  if (g_held_button >= 0 && g_button_pressed[g_held_button]) {
    return millis() - g_held_start;
  }
  return 0;
}

void HalGPIO::startDeepSleep() {
  printf("[SIM] Deep sleep requested (ignored in web simulator)\n");
  // In the web simulator, "deep sleep" just clears the screen
  EM_ASM({
    if (typeof Module.onDeepSleep === 'function') {
      Module.onDeepSleep();
    }
  });
}

int HalGPIO::getBatteryPercentage() const {
  return 100;  // Always full battery in simulator
}

bool HalGPIO::isUsbConnected() const {
  return true;  // Always "connected" in simulator
}

HalGPIO::WakeupReason HalGPIO::getWakeupReason() const {
  // Simulate normal boot after flashing
  return WakeupReason::AfterFlash;
}
