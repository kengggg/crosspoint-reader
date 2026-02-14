// Emscripten entry point for CrossPoint Reader web emulator.
//
// Replaces the Arduino framework's main() which calls setup() and loop().
// Uses emscripten_set_main_loop for the browser event loop, and
// Emscripten fibers for cooperative FreeRTOS task scheduling.

#include <emscripten.h>
#include <cstdio>

// FreeRTOS fiber system (our shim)
extern void emulator_fiber_init();
extern void emulator_run_background_tasks(int elapsed_ms);

// Arduino firmware entry points (defined in main.cpp)
extern void setup();
extern void loop();

// Timing for the main loop
static double g_last_time = 0;

// Main loop callback called by the browser's requestAnimationFrame
static void em_main_loop() {
  double now = emscripten_get_now();
  int elapsed_ms = static_cast<int>(now - g_last_time);
  if (elapsed_ms < 1) elapsed_ms = 1;
  if (elapsed_ms > 100) elapsed_ms = 100;  // Cap to prevent huge jumps
  g_last_time = now;

  // Run the firmware's loop() function.
  // This calls gpio.update(), activity->loop(), and delay(10).
  // delay(10) is a no-op in non-ASYNCIFY main context.
  loop();

  // Run background tasks (FreeRTOS task fibers).
  // Each task gets swapped in if its delay has elapsed.
  emulator_run_background_tasks(elapsed_ms);
}

int main() {
  printf("[EMU] CrossPoint Reader Web Emulator starting...\n");

  // Initialize the fiber system for cooperative multitasking
  emulator_fiber_init();

  // Run the firmware's setup() function
  printf("[EMU] Running firmware setup()...\n");
  setup();
  printf("[EMU] Setup complete, starting main loop.\n");

  // Start the main loop at ~60fps
  g_last_time = emscripten_get_now();
  emscripten_set_main_loop(em_main_loop, 60, 0);

  return 0;
}
