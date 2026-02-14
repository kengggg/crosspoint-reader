#pragma once
// ESP HTTP client shim - network operations not supported in web emulator.

#include <cstdint>
#include <cstdlib>
#include <cstring>

// In the real ESP32/Arduino build, Arduino symbols (Serial, millis, min, etc.)
// are globally available. Include Arduino.h so firmware code that only includes
// ESP-IDF headers still gets those symbols.
#include "Arduino.h"

typedef int esp_err_t;
#ifndef ESP_OK
#define ESP_OK 0
#endif
#ifndef ESP_FAIL
#define ESP_FAIL -1
#endif
#define ESP_ERR_NO_MEM 0x101
#define ESP_ERR_HTTPS_OTA_IN_PROGRESS 0x2001

inline const char* esp_err_to_name(esp_err_t code) {
  switch (code) {
    case ESP_OK: return "ESP_OK";
    case ESP_FAIL: return "ESP_FAIL";
    case ESP_ERR_NO_MEM: return "ESP_ERR_NO_MEM";
    default: return "UNKNOWN";
  }
}

typedef void* esp_http_client_handle_t;

// HTTP event types
typedef enum {
  HTTP_EVENT_ERROR = 0,
  HTTP_EVENT_ON_CONNECTED,
  HTTP_EVENT_HEADER_SENT,
  HTTP_EVENT_ON_HEADER,
  HTTP_EVENT_ON_DATA,
  HTTP_EVENT_ON_FINISH,
  HTTP_EVENT_DISCONNECTED,
} esp_http_client_event_id_t;

// HTTP event structure
typedef struct {
  esp_http_client_event_id_t event_id;
  esp_http_client_handle_t client;
  void* data;
  int data_len;
  void* user_data;
  char* header_key;
  char* header_value;
} esp_http_client_event_t;

typedef esp_err_t (*http_event_handle_cb)(esp_http_client_event_t* evt);
typedef esp_err_t (*esp_crt_bundle_attach_t)(void* conf);
typedef esp_err_t (*http_client_init_cb_t)(esp_http_client_handle_t);

typedef struct {
  const char* url;
  const char* cert_pem;
  http_event_handle_cb event_handler;
  int timeout_ms;
  int buffer_size;
  int buffer_size_tx;
  bool skip_cert_common_name_check;
  esp_crt_bundle_attach_t crt_bundle_attach;
  bool keep_alive_enable;
} esp_http_client_config_t;

inline esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t* config) {
  (void)config;
  return nullptr;
}
inline esp_err_t esp_http_client_perform(esp_http_client_handle_t client) {
  (void)client;
  return ESP_FAIL;
}
inline esp_err_t esp_http_client_cleanup(esp_http_client_handle_t client) {
  (void)client;
  return ESP_OK;
}
inline int esp_http_client_get_status_code(esp_http_client_handle_t client) {
  (void)client;
  return 0;
}
inline int64_t esp_http_client_get_content_length(esp_http_client_handle_t client) {
  (void)client;
  return 0;
}
inline esp_err_t esp_http_client_set_url(esp_http_client_handle_t client, const char* url) {
  (void)client;
  (void)url;
  return ESP_FAIL;
}
inline esp_err_t esp_http_client_set_header(esp_http_client_handle_t client, const char* key, const char* value) {
  (void)client;
  (void)key;
  (void)value;
  return ESP_OK;
}
inline esp_err_t esp_http_client_open(esp_http_client_handle_t client, int len) {
  (void)client;
  (void)len;
  return ESP_FAIL;
}
inline int esp_http_client_read(esp_http_client_handle_t client, char* buf, int len) {
  (void)client;
  (void)buf;
  (void)len;
  return -1;
}
inline esp_err_t esp_http_client_close(esp_http_client_handle_t client) {
  (void)client;
  return ESP_OK;
}
inline int esp_http_client_fetch_headers(esp_http_client_handle_t client) {
  (void)client;
  return -1;
}
inline bool esp_http_client_is_chunked_response(esp_http_client_handle_t client) {
  (void)client;
  return false;
}
inline esp_err_t esp_http_client_get_chunk_length(esp_http_client_handle_t client, int* len) {
  (void)client;
  if (len) *len = 0;
  return ESP_OK;
}
