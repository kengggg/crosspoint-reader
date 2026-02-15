#pragma once
// FreeRTOS task shim for Emscripten web simulator.
// Tasks are registered and run cooperatively from the main emscripten loop.

#include "FreeRTOS.h"

typedef void* TaskHandle_t;
typedef void (*TaskFunction_t)(void*);

// Creates a task. In the simulator, this registers the task for cooperative scheduling.
BaseType_t xTaskCreate(TaskFunction_t pvTaskCode, const char* pcName, uint32_t usStackDepth, void* pvParameters,
                       UBaseType_t uxPriority, TaskHandle_t* pxCreatedTask);

// Deletes a task. Pass NULL to delete the calling task.
void vTaskDelete(TaskHandle_t xTaskToDelete);

// Delays the calling task. In the simulator, this yields back to the main fiber.
void vTaskDelay(TickType_t xTicksToDelay);

// Run one round of background tasks (called from simulator main loop).
void simulator_run_background_tasks(int elapsed_ms);

// Initialize the fiber system (called once at startup).
void simulator_fiber_init();

// Get current task handle (NULL if in main context)
TaskHandle_t xTaskGetCurrentTaskHandle();
