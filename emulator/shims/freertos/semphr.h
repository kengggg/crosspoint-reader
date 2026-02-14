#pragma once
// FreeRTOS semaphore/mutex shim for Emscripten web emulator.
// In single-threaded cooperative mode, mutexes are no-ops.

#include "FreeRTOS.h"

typedef void* SemaphoreHandle_t;

inline SemaphoreHandle_t xSemaphoreCreateMutex() {
  // Return a non-null dummy value
  return reinterpret_cast<SemaphoreHandle_t>(1);
}

inline BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xBlockTime) {
  (void)xSemaphore;
  (void)xBlockTime;
  return pdTRUE;
}

inline BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore) {
  (void)xSemaphore;
  return pdTRUE;
}

inline void vSemaphoreDelete(SemaphoreHandle_t xSemaphore) { (void)xSemaphore; }
