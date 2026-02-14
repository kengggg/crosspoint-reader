#pragma once
// GPIO driver shim - no-ops for web emulator.

#include <cstdint>

typedef int gpio_num_t;

inline int gpio_set_direction(gpio_num_t gpio, int mode) {
  (void)gpio;
  (void)mode;
  return 0;
}

inline int gpio_set_level(gpio_num_t gpio, uint32_t level) {
  (void)gpio;
  (void)level;
  return 0;
}

inline int gpio_get_level(gpio_num_t gpio) {
  (void)gpio;
  return 0;
}
