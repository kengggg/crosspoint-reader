#pragma once
// esp_sleep shim for Emscripten web simulator

#include <cstdint>

typedef enum {
  ESP_SLEEP_WAKEUP_UNDEFINED = 0,
  ESP_SLEEP_WAKEUP_GPIO = 7,
} esp_sleep_source_t;

typedef enum {
  ESP_GPIO_WAKEUP_GPIO_LOW = 0,
  ESP_GPIO_WAKEUP_GPIO_HIGH = 1,
} esp_gpio_wakeup_level_t;

inline esp_sleep_source_t esp_sleep_get_wakeup_cause() {
  return ESP_SLEEP_WAKEUP_UNDEFINED;
}

inline void esp_deep_sleep_start() {
  // In web simulator, deep sleep just means "go idle"
}

inline void esp_deep_sleep_enable_gpio_wakeup(uint64_t gpio_mask, int level) {
  (void)gpio_mask;
  (void)level;
}

typedef enum {
  ESP_RST_UNKNOWN = 0,
  ESP_RST_POWERON = 1,
  ESP_RST_DEEPSLEEP = 5,
} esp_reset_reason_t;

inline esp_reset_reason_t esp_reset_reason() {
  return ESP_RST_POWERON;
}

inline void esp_restart() {}
