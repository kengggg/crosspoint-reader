#pragma once
// FreeRTOS type shim for Emscripten web emulator.
// Provides the types and macros used by CrossPoint firmware.

// Include common C++ headers that the real FreeRTOS/ESP-IDF transitively provides.
// Without these, firmware code that only includes FreeRTOS headers may miss them.
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>

typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;

#define pdTRUE 1
#define pdFALSE 0
#define pdPASS pdTRUE
#define pdFAIL pdFALSE

#define portMAX_DELAY 0xFFFFFFFF
#define portTICK_PERIOD_MS 1

#define configMINIMAL_STACK_SIZE 128

#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(xTimeInMs))
#define portTICK_RATE_MS portTICK_PERIOD_MS
