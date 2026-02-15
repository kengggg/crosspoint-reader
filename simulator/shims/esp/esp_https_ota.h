#pragma once
// OTA update shim - not supported in web simulator.

#include "esp_http_client.h"

typedef struct {
  const esp_http_client_config_t* http_config;
  http_client_init_cb_t http_client_init_cb;
} esp_https_ota_config_t;

typedef void* esp_https_ota_handle_t;

inline esp_err_t esp_https_ota_begin(const esp_https_ota_config_t* config, esp_https_ota_handle_t* handle) {
  (void)config;
  (void)handle;
  return ESP_FAIL;
}
inline esp_err_t esp_https_ota_perform(esp_https_ota_handle_t handle) {
  (void)handle;
  return ESP_FAIL;
}
inline esp_err_t esp_https_ota_finish(esp_https_ota_handle_t handle) {
  (void)handle;
  return ESP_OK;
}
inline int esp_https_ota_get_image_len_read(esp_https_ota_handle_t handle) {
  (void)handle;
  return 0;
}
inline int esp_https_ota_get_image_size(esp_https_ota_handle_t handle) {
  (void)handle;
  return 0;
}
inline bool esp_https_ota_is_complete_data_received(esp_https_ota_handle_t handle) {
  (void)handle;
  return false;
}
