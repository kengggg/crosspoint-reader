#pragma once
// BatteryMonitor stub for Emscripten web simulator.
// Always reports 100% battery.

#include <cstdint>

class BatteryMonitor {
 public:
  explicit BatteryMonitor(uint8_t pin) { (void)pin; }
  int readPercentage() const { return 100; }
};
