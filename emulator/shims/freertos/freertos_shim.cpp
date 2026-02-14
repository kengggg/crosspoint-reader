// FreeRTOS cooperative task scheduler for Emscripten web emulator.
//
// Uses Emscripten fibers (ASYNCIFY-based) to give each FreeRTOS task its own
// execution context. The main loop swaps to each task fiber when its delay
// expires; the task does one iteration of work, then swaps back via vTaskDelay.

#include "task.h"

#include <cstdio>
#include <cstring>
#include <vector>

#include <emscripten.h>
#include <emscripten/fiber.h>

// Each FreeRTOS task maps to an Emscripten fiber
static constexpr int MAX_TASKS = 16;
static constexpr size_t TASK_STACK_SIZE = 128 * 1024;      // 128KB stack per task
static constexpr size_t TASK_ASYNCIFY_SIZE = 128 * 1024;   // 128KB asyncify data per task

struct EmulatorTask {
  emscripten_fiber_t fiber;
  char* stack;
  char* asyncify_stack;
  TaskFunction_t fn;
  void* param;
  bool active;
  bool started;
  int delay_remaining_ms;
  char name[32];
};

static emscripten_fiber_t g_main_fiber;
static char g_main_asyncify_stack[TASK_ASYNCIFY_SIZE];
static EmulatorTask g_tasks[MAX_TASKS];
static int g_task_count = 0;
static int g_current_task = -1;  // -1 = main fiber

// Fiber entry point wrapper
static void task_fiber_entry(void* arg) {
  EmulatorTask* task = static_cast<EmulatorTask*>(arg);
  task->fn(task->param);
  // If the task function returns (rare - most have while(true) loops):
  task->active = false;
  // Swap back to main fiber
  emscripten_fiber_swap(&task->fiber, &g_main_fiber);
}

void emulator_fiber_init() {
  emscripten_fiber_init_from_current_context(&g_main_fiber, g_main_asyncify_stack, sizeof(g_main_asyncify_stack));
  g_task_count = 0;
  g_current_task = -1;
  memset(g_tasks, 0, sizeof(g_tasks));
}

BaseType_t xTaskCreate(TaskFunction_t pvTaskCode, const char* pcName, uint32_t usStackDepth, void* pvParameters,
                       UBaseType_t uxPriority, TaskHandle_t* pxCreatedTask) {
  (void)usStackDepth;
  (void)uxPriority;

  if (g_task_count >= MAX_TASKS) {
    printf("[EMU] ERROR: Max tasks (%d) exceeded!\n", MAX_TASKS);
    return pdFAIL;
  }

  int idx = g_task_count;
  EmulatorTask& task = g_tasks[idx];

  // Allocate stack and asyncify data on the heap
  task.stack = new char[TASK_STACK_SIZE];
  task.asyncify_stack = new char[TASK_ASYNCIFY_SIZE];
  task.fn = pvTaskCode;
  task.param = pvParameters;
  task.active = true;
  task.started = false;
  task.delay_remaining_ms = 0;
  strncpy(task.name, pcName ? pcName : "task", sizeof(task.name) - 1);
  task.name[sizeof(task.name) - 1] = '\0';

  // Initialize the fiber
  emscripten_fiber_init(&task.fiber, task_fiber_entry, &task, task.stack, TASK_STACK_SIZE, task.asyncify_stack,
                        TASK_ASYNCIFY_SIZE);

  if (pxCreatedTask) {
    *pxCreatedTask = reinterpret_cast<TaskHandle_t>(static_cast<intptr_t>(idx + 1));
  }

  g_task_count++;
  printf("[EMU] Task created: '%s' (index %d)\n", task.name, idx);
  return pdPASS;
}

void vTaskDelete(TaskHandle_t xTaskToDelete) {
  int idx;
  if (xTaskToDelete == nullptr) {
    // Delete calling task
    idx = g_current_task;
  } else {
    idx = static_cast<int>(reinterpret_cast<intptr_t>(xTaskToDelete)) - 1;
  }

  if (idx >= 0 && idx < g_task_count) {
    g_tasks[idx].active = false;
    printf("[EMU] Task deleted: '%s' (index %d)\n", g_tasks[idx].name, idx);

    // If deleting the currently running task, swap back to main
    if (idx == g_current_task) {
      g_current_task = -1;
      emscripten_fiber_swap(&g_tasks[idx].fiber, &g_main_fiber);
    }
  }
}

void vTaskDelay(TickType_t xTicksToDelay) {
  if (g_current_task >= 0) {
    // Set delay and swap back to main fiber
    g_tasks[g_current_task].delay_remaining_ms = static_cast<int>(xTicksToDelay * portTICK_PERIOD_MS);
    emscripten_fiber_swap(&g_tasks[g_current_task].fiber, &g_main_fiber);
  } else {
    // Called from main context - use emscripten_sleep
    emscripten_sleep(xTicksToDelay * portTICK_PERIOD_MS);
  }
}

void emulator_run_background_tasks(int elapsed_ms) {
  for (int i = 0; i < g_task_count; i++) {
    if (!g_tasks[i].active) continue;

    // Decrement delay
    g_tasks[i].delay_remaining_ms -= elapsed_ms;

    if (g_tasks[i].delay_remaining_ms <= 0) {
      g_tasks[i].delay_remaining_ms = 0;
      g_current_task = i;

      // Swap to the task fiber - it will either start or resume
      emscripten_fiber_swap(&g_main_fiber, &g_tasks[i].fiber);

      g_current_task = -1;
    }
  }
}

TaskHandle_t xTaskGetCurrentTaskHandle() {
  if (g_current_task >= 0) {
    return reinterpret_cast<TaskHandle_t>(static_cast<intptr_t>(g_current_task + 1));
  }
  return nullptr;
}
