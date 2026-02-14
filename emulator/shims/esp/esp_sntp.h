#pragma once
// SNTP shim for web emulator - time is already available via JS Date.

#define SNTP_OPMODE_POLL 0
#define ESP_SNTP_OPMODE_POLL 0

typedef enum {
  SNTP_SYNC_STATUS_RESET = 0,
  SNTP_SYNC_STATUS_COMPLETED = 1,
  SNTP_SYNC_STATUS_IN_PROGRESS = 2,
} sntp_sync_status_t;

inline void esp_sntp_setoperatingmode(int mode) { (void)mode; }
inline void esp_sntp_setservername(int idx, const char* name) {
  (void)idx;
  (void)name;
}
inline void esp_sntp_init() {}
inline void esp_sntp_stop() {}
inline bool esp_sntp_enabled() { return false; }
inline sntp_sync_status_t sntp_get_sync_status() { return SNTP_SYNC_STATUS_COMPLETED; }
