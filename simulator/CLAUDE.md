# CLAUDE.md — Web Simulator

Guidelines for AI agents working on the CrossPoint Reader web simulator.

## What This Is

A browser-based emulator that compiles the **exact same firmware** (C++20, ESP32-C3 target) to WebAssembly via Emscripten. The firmware runs unmodified — only hardware abstractions are replaced with browser equivalents.

**Golden rule: never modify firmware code (`src/`, `lib/`) to accommodate the simulator.** All simulator logic lives in `simulator/`.

## Build & Run

```bash
# Requires: Emscripten SDK (emsdk), CMake 3.13+
source <path-to-emsdk>/emsdk_env.sh

cd simulator/
./build.sh          # Build only
./build.sh serve    # Build + serve at localhost:8080
```

Output lands in `simulator/build/`. Must be served via HTTP (not `file://`).

## Architecture Overview

```
simulator/
├── main_simulator.cpp       # Emscripten entry point (replaces Arduino main())
├── hal/                     # HAL implementations → browser APIs
│   ├── HalDisplay_Web.cpp   # Framebuffer → HTML <canvas>
│   ├── HalGPIO_Web.cpp      # Buttons → keyboard/mouse events
│   └── HalStorage_Web.cpp   # SD card → Emscripten MEMFS (in-memory)
├── shell/                   # Web UI layer
│   ├── index.html           # Canvas (480x800) + buttons + file upload
│   ├── shell.js             # Rendering, input dispatch, EPUB upload
│   └── style.css            # Device-frame styling, responsive layout
├── shims/                   # Compatibility stubs (found before real SDK headers)
│   ├── arduino/             # Arduino.h, Serial, SPI, Print, Stream
│   ├── esp/                 # esp_sleep, esp_wifi, esp_task_wdt, etc.
│   ├── freertos/            # Cooperative task scheduler via Emscripten fibers
│   ├── network/             # WiFi, WebServer, HTTPClient, WebSockets stubs
│   ├── sdk/                 # EInkDisplay, InputManager, BatteryMonitor, SDCardManager
│   └── thirdparty/          # ArduinoJson, QRCode, MD5Builder, base64
└── CMakeLists.txt           # Emscripten cross-compile configuration
```

## Critical Design Constraints

### 1. Include Path Ordering (CMakeLists.txt)
Simulator shim directories are listed **before** firmware includes. This ensures `#include <Arduino.h>` resolves to our shim, not the real ESP-IDF SDK. Breaking this order will cause build failures or link against wrong headers.

### 2. Display Coordinate Rotation
- Physical e-ink panel: **800x480** (landscape)
- Firmware logical coords: **480x800** (portrait) — GfxRenderer rotates 90 CW
- Canvas: **480x800** (portrait) — `shell.js` applies inverse rotation when reading the framebuffer
- Framebuffer is 1-bit packed (MSB first, 1=white, 0=black), 100 bytes/row in physical orientation

### 3. FreeRTOS → Emscripten Fibers
Each `xTaskCreate()` allocates an Emscripten fiber (128KB stack + 128KB asyncify data). The main loop calls `simulator_run_background_tasks()` to swap to fibers whose `vTaskDelay()` has elapsed. Max 16 concurrent tasks. Requires `-sASYNCIFY=1` linker flag.

### 4. MEMFS Filesystem
Emscripten's in-memory POSIX filesystem replaces the SD card. Files at `/sd/books/` (uploaded EPUBs) and `/sd/.crosspoint/` (cache). **All data is lost on page refresh** — this is expected.

### 5. Grayscale Anti-aliasing
2-bit grayscale uses separate LSB/MSB planes. Level 0 = keep existing BW pixel (transparent merge). Levels 2/3 = light/dark gray overlaid onto the BW framebuffer. The merge logic in `shell.js:renderGrayscale()` must preserve BW pixels where grayscale is zero.

## Adding New Shims

When firmware adds a new `#include` that doesn't exist in the simulator:

1. Identify which shim category it belongs to (`arduino/`, `esp/`, `network/`, `sdk/`, `thirdparty/`)
2. Create a stub header that satisfies the API contract with no-op or minimal implementations
3. Only stub what the firmware actually calls — don't implement entire APIs speculatively
4. If the feature is network-dependent, the stub should gracefully degrade (return error codes, empty data)

## What's Stubbed Out (Not Functional)

- **WiFi/networking**: All network features return stubs (WiFi, OPDS, Calibre sync, OTA, KOReader sync, WebServer)
- **Battery**: Always reports 100%, USB always "connected"
- **Deep sleep**: Triggers JS callback to blank canvas (no actual power savings)
- **Screen rotation**: Physical rotation not simulated (firmware rotation settings still work)
- **ArduinoJson**: Heavy parsing operations are stubbed in `network_heavy_stubs.cpp`

## Common Pitfalls

- **Missing `emscripten_sleep()` calls**: Any blocking firmware loop must yield via `emscripten_sleep()` or `vTaskDelay()`. Infinite loops without yields will freeze the browser tab.
- **Canvas scaling**: Canvas is 1:1 pixel mapping (480x800). Do not use CSS scaling on the canvas element — it causes nearest-neighbor artifacts on anti-aliased text.
- **Memory**: Initial heap is 32MB (`-sINITIAL_MEMORY=33554432`) with growth enabled. Much more than the 380KB on real hardware, but the firmware's caching strategy is still important to validate.
- **`delay()` in shims**: Maps to `emscripten_sleep()`, which yields to the browser event loop. This is correct but means timing-sensitive code may behave differently than on hardware.

## Key Exported Functions (C++ → JavaScript)

Defined with `EMSCRIPTEN_KEEPALIVE` or in `-sEXPORTED_FUNCTIONS`:

- `simulator_button_down(int buttonId)` / `simulator_button_up(int buttonId)` — input from shell.js
- `js_render_framebuffer(pointer, size, mode)` — called by HalDisplay to trigger canvas render
- `js_render_grayscale(lsbPtr, msbPtr, size)` — grayscale overlay rendering
- `js_on_deep_sleep()` — called when firmware enters deep sleep

## Key JavaScript → WASM Interactions (shell.js)

- `Module.renderFramebuffer(ptr, size, mode)` — reads HEAPU8, rotates, draws to canvas
- `Module.renderGrayscale(lsbPtr, msbPtr, size)` — merges gray pixels onto existing canvas
- `Module.onDeepSleep()` — blanks canvas, shows sleep indicator
- File upload: `FS.writeFile('/sd/books/<name>', data)` via Emscripten FS API

## Button Mapping

| ID | Constant     | Keyboard        | Function       |
|----|-------------|-----------------|----------------|
| 0  | BTN_BACK    | Escape/Backspace| Go back        |
| 1  | BTN_CONFIRM | Enter/Space     | Select/confirm |
| 2  | BTN_LEFT    | Arrow Left      | Previous item  |
| 3  | BTN_RIGHT   | Arrow Right     | Next item      |
| 4  | BTN_UP      | Arrow Up        | Page back      |
| 5  | BTN_DOWN    | Arrow Down      | Page forward   |
| 6  | BTN_POWER   | P               | Sleep/wake     |
