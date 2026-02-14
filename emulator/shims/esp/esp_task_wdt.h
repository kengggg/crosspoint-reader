#pragma once
// ESP task watchdog timer shim - all no-ops in web emulator.

inline void esp_task_wdt_reset() {}
inline void esp_task_wdt_init(int timeout, bool panic) {
  (void)timeout;
  (void)panic;
}
inline void esp_task_wdt_add(void* handle) { (void)handle; }
inline void esp_task_wdt_delete(void* handle) { (void)handle; }
