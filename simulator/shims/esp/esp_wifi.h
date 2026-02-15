#pragma once
// ESP WiFi shim - not supported in web simulator.
// Just provides types needed for compilation.

#include "esp_http_client.h"

typedef enum {
  WIFI_MODE_NULL = 0,
  WIFI_MODE_STA,
  WIFI_MODE_AP,
  WIFI_MODE_APSTA,
} wifi_mode_t;

typedef enum {
  WIFI_PS_NONE = 0,
  WIFI_PS_MIN_MODEM,
  WIFI_PS_MAX_MODEM,
} wifi_ps_type_t;

inline esp_err_t esp_wifi_set_ps(wifi_ps_type_t type) {
  (void)type;
  return ESP_OK;
}
