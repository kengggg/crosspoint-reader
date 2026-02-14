# Web Emulator for CrossPoint Reader: Deep Feasibility Analysis & Implementation Plan

## Executive Summary

**Verdict: FEASIBLE - recommended approach is Emscripten/WASM with ASYNCIFY.**

The CrossPoint Reader firmware has a clean 3-layer architecture (HAL -> Libraries -> Application) that makes web emulation achievable. After deep analysis of every source file, SDK interface, and hardware dependency, the critical finding is:

- **HAL layer** (the only truly hardware-specific code): **~350 LOC** across 3 files
- **Reusable application + library code**: **~20,000 LOC** across 200+ files
- **Ratio: 98% of code is hardware-independent**

The biggest revised finding: **24 out of 29 activities** use FreeRTOS tasks/semaphores in an identical pattern, making this the #1 architectural challenge. However, the pattern is so consistent it can be solved with a single compatibility shim rather than 24 individual refactors.

---

## 1. Hardware Reference: Xteink X4

### 1.1 Device Specifications

| Spec | Value |
|------|-------|
| Display | 4.3" E-Ink, 480x800 pixels, 220 PPI, no backlight, no touch |
| SoC | ESP32-C3 (RISC-V single-core, 160MHz) |
| RAM | 400KB SRAM (no PSRAM address mapping on C3) |
| Storage | 32GB microSD card (SPI interface) |
| Battery | 650 mAh Li-Po (~2 weeks daily use) |
| Connectivity | WiFi 2.4GHz, Bluetooth, USB-C (charge only, no MTP) |
| Dimensions | 114 x 69 x 5.9mm, 74g |
| Magnetic | MagSafe-compatible rear magnets |

### 1.2 Physical Button Layout

```
              ┌──────────────────────────────┐
              │ [Prev Page]  [Power] [Next]   │  ← Top edge (right side buttons)
              │                                │
              │                                │
    SD slot → │                                │ ← USB-C port
   Reset  →   │      4.3" E-Ink Display        │ ← Lanyard hole
              │        480 x 800               │
              │                                │
              │                                │
              │                                │
              │ [Back]  [OK]  [Left]  [Right]  │  ← Bottom edge (front buttons)
              └──────────────────────────────┘
```

**Button groups (as mapped in firmware):**
- **4 front buttons** (bottom): Back (idx 0), Confirm/OK (idx 1), Left (idx 2), Right (idx 3) - user-remappable
- **2 side buttons** (top-right): Up/PrevPage (idx 4), Down/NextPage (idx 5) - swappable
- **1 power button** (top): Power (idx 6) - fixed

### 1.3 Display Characteristics for Emulation

The e-ink display has unique visual properties that should be emulated:
- **1-bit monochrome** primary mode (black/white only, no grayscale)
- **2-bit grayscale** mode for cover images (4 levels: white, light gray, dark gray, black)
- **Full refresh**: entire screen flashes black then white (~2s), eliminates ghosting
- **Half refresh**: moderate quality/speed tradeoff (~1.7s)
- **Fast refresh**: partial update, may show ghosting (~0.5s)
- **Persistence**: e-ink retains image with zero power (relevant for sleep screen)

---

## 2. Architecture Deep Dive

### 2.1 Three-Layer Design

```
┌─────────────────────────────────────────────────────────────┐
│  APPLICATION LAYER (src/)                          ~5,000 LOC│
│  29 Activities, Settings, Input Mapping, Main Loop           │
│  CrossPointSettings, CrossPointState, RecentBooksStore       │
│  UITheme, ButtonNavigator, Battery                           │
├─────────────────────────────────────────────────────────────┤
│  LIBRARY LAYER (lib/)                             ~15,000 LOC│
│  GfxRenderer    - framebuffer drawing primitives             │
│  EpdFont        - bitmap font rendering (14 families)        │
│  Epub           - EPUB 2/3 parsing, CSS, layout, pagination  │
│  Txt            - plain text reader                          │
│  ZipFile        - ZIP container abstraction                  │
│  Serialization  - binary read/write templates                │
│  Utf8           - UTF-8 codepoint utilities                  │
│  OpdsParser     - OPDS catalog XML parsing                   │
│  KOReaderSync   - KOReader progress sync client              │
│  FsHelpers      - filesystem path utilities                  │
│  miniz          - ZIP deflate/inflate (pure C)               │
│  expat          - XML parsing (pure C)                       │
│  picojpeg       - JPEG decoding (pure C)                     │
│  JpegToBmpConverter - JPEG to BMP with dithering             │
├─────────────────────────────────────────────────────────────┤
│  HAL LAYER (lib/hal/)                               ~350 LOC │
│  HalDisplay  → EInkDisplay (open-x4-sdk)                     │
│  HalGPIO     → InputManager, BatteryMonitor (open-x4-sdk)   │
│  HalStorage  → SDCardManager (open-x4-sdk)                   │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Complete Activity Inventory

| Activity | FreeRTOS? | Needed in Emulator? | Notes |
|----------|:---------:|:-------------------:|-------|
| **BootActivity** | No | Yes | Splash screen |
| **SleepActivity** | No | Optional | Sleep image display |
| **HomeActivity** | Yes | Yes | Main menu + recent books |
| **MyLibraryActivity** | Yes | Yes | File browser |
| **RecentBooksActivity** | Yes | Yes | Recent books list |
| **EpubReaderActivity** | Yes | Yes | Core EPUB reader |
| **TxtReaderActivity** | Yes | Yes | Plain text reader |
| **XtcReaderActivity** | Yes | Yes | XTC format reader |
| **EpubReaderMenuActivity** | Yes | Yes | Reader overlay menu |
| **EpubReaderChapterSelectionActivity** | Yes | Yes | TOC navigation |
| **EpubReaderPercentSelectionActivity** | Yes | Yes | Progress slider |
| **XtcReaderChapterSelectionActivity** | Yes | Yes | XTC TOC |
| **KOReaderSyncActivity** | Yes | Optional | Reading sync |
| **SettingsActivity** | Yes | Yes | Settings menu |
| **ButtonRemapActivity** | Yes | Yes | Button config |
| **ClearCacheActivity** | Yes | Yes | Cache management |
| **KOReaderSettingsActivity** | Yes | Optional | KOReader config |
| **KOReaderAuthActivity** | Yes | Optional | KOReader login |
| **CalibreSettingsActivity** | Yes | Optional | Calibre config |
| **OtaUpdateActivity** | Yes | No | Firmware update |
| **KeyboardEntryActivity** | Yes | Yes | On-screen keyboard |
| **FullScreenMessageActivity** | No | Yes | Error/info screens |
| **CrossPointWebServerActivity** | Yes | No | WiFi file transfer |
| **WifiSelectionActivity** | Yes | No | WiFi picker |
| **NetworkModeSelectionActivity** | Yes | No | Network mode |
| **CalibreConnectActivity** | Yes | No | Calibre wireless |
| **OpdsBookBrowserActivity** | Yes | Optional | OPDS catalog |
| **ReaderActivity** | No | Yes | Reader dispatcher |

**Critical finding: 24/29 activities use FreeRTOS** in an identical pattern:
```cpp
// onEnter():
renderingMutex = xSemaphoreCreateMutex();
xTaskCreate(&Activity::taskTrampoline, "TaskName", stackSize, this, 1, &displayTaskHandle);

// displayTaskLoop() (background thread - [[noreturn]]):
while (true) {
    xSemaphoreTake(renderingMutex, portMAX_DELAY);
    // render to framebuffer
    // call displayBuffer()
    xSemaphoreGive(renderingMutex);
    delay(someMs);
}

// loop() (main thread):
if (buttonPressed) {
    xSemaphoreTake(renderingMutex, portMAX_DELAY);
    // update state
    updateRequired = true;
    xSemaphoreGive(renderingMutex);
}

// onExit():
xSemaphoreTake(renderingMutex, portMAX_DELAY);
vTaskDelete(displayTaskHandle);
// cleanup
```

### 2.3 Key Type Dependencies Across HAL Boundary

| Type | Origin | Used In | Count |
|------|--------|---------|-------|
| `FsFile` | SdFat (via SDCardManager) | 40+ files | ~200 call sites |
| `String` (Arduino) | Arduino.h | HalStorage, WebServer, settings | ~50 uses |
| `Print` | Arduino.h | Epub streaming, Serial | ~30 uses |
| `Serial` | HardwareSerial.h | All activities (logging) | ~580 printf calls |
| `millis()` | Arduino.h | Main loop, activities | ~80 uses |
| `delay()` | Arduino.h | Input polling, tasks | ~40 uses |
| `yield()` | Arduino.h | WebServer loop | ~5 uses |
| `PROGMEM` | pgmspace.h | Font data, images | ~20 uses |
| `TaskHandle_t` | FreeRTOS | 24 activities | 24 instances |
| `SemaphoreHandle_t` | FreeRTOS | 24 activities | 24 instances |
| `ESP.getFreeHeap()` | ESP class | main.cpp only | 3 calls |
| `digitalRead()` | Arduino.h | HalGPIO only | 1 call |
| `pinMode()` | Arduino.h | HalGPIO only | 2 calls |
| `SPI.begin()` | SPI.h | HalGPIO only | 1 call |

---

## 3. Recommended Approach: Emscripten/WASM with ASYNCIFY

### 3.1 Why WASM (not pure JS rewrite)

| Criterion | WASM | JS Rewrite |
|-----------|------|------------|
| Code reuse | ~80% as-is | 0% |
| Pixel accuracy | Identical to firmware | Different rendering |
| Maintenance sync | Same codebase | Two codebases diverge |
| Font rendering | Exact bitmap fonts | Browser fonts (different) |
| EPUB layout | Same hyphenation/pagination | Different library |
| Effort | ~2,500 LOC new | ~20,000 LOC rewrite |

### 3.2 Why ASYNCIFY (not pthreads or single-threaded refactor)

Given that **24 of 29 activities** use the FreeRTOS task+mutex pattern with `delay()` inside `[[noreturn]] displayTaskLoop()`, the threading approach is the pivotal architectural decision:

**Option A: Emscripten pthreads** (map FreeRTOS -> pthreads)
- Pros: Minimal code changes; direct mapping
- Cons: Requires `SharedArrayBuffer` (COOP/COEP headers), excludes Safari <16.4, complex deployment

**Option B: Single-threaded refactor** (remove all background tasks)
- Pros: Simplest runtime, no browser restrictions
- Cons: Must refactor 24 activities; display task `[[noreturn]]` loops can't run inline; massive code changes

**Option C: ASYNCIFY** (recommended)
- Pros: `delay()` and blocking calls "just work" via stack unwinding; FreeRTOS shim can implement `xSemaphoreTake` as `emscripten_sleep()`; minimal source changes
- Cons: 30-50% larger WASM binary; slight runtime overhead
- Why it wins: The FreeRTOS shim is ~50 LOC and covers all 24 activities

**ASYNCIFY explanation**: Emscripten's ASYNCIFY feature rewrites the WASM binary so that any call to `emscripten_sleep()` saves the entire call stack, yields to the browser event loop, and resumes later. This means `delay(10)` inside a `while(true)` loop becomes cooperative multitasking without any source code changes.

### 3.3 Final Architecture

```
┌───────────────────────────────────────────────────────────────┐
│  BROWSER                                                       │
│                                                                │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │ Web Shell (index.html + style.css + shell.js)             │ │
│  │                                                           │ │
│  │  ┌─────────────────────┐  ┌────────────────────────────┐ │ │
│  │  │  Device Frame SVG    │  │  Control Panel              │ │ │
│  │  │  ┌───────────────┐  │  │  [Load EPUB] [Persist On]  │ │ │
│  │  │  │               │  │  │  Keyboard shortcuts:        │ │ │
│  │  │  │  Canvas       │  │  │  ESC=Back  Enter=OK        │ │ │
│  │  │  │  480x800      │  │  │  Arrows=Navigate           │ │ │
│  │  │  │  (scaled 2x)  │  │  │  PgUp/PgDn=Page turn      │ │ │
│  │  │  │               │  │  │  P=Power                    │ │ │
│  │  │  └───────────────┘  │  │                             │ │ │
│  │  │  [Bk][OK][◄][►]    │  │  console.log output         │ │ │
│  │  │  [PgUp]    [PgDn]  │  │                             │ │ │
│  │  │       [Power]       │  │                             │ │ │
│  │  └─────────────────────┘  └────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────┘ │
│                              │                                  │
│                    JS Bridge │ (shell.js)                       │
│                              │                                  │
│  ┌───────────────────────────┴───────────────────────────────┐ │
│  │ WASM Module (crosspoint-emulator.wasm + .js)              │ │
│  │                                                           │ │
│  │  ┌─ Compatibility Shims (new) ──────────────────────────┐ │ │
│  │  │  Arduino.h     - String, Print, Serial, millis,      │ │ │
│  │  │                  delay→emscripten_sleep, PROGMEM      │ │ │
│  │  │  SdFat.h       - FsFile wrapping FILE* (MEMFS)       │ │ │
│  │  │  FreeRTOS.h    - xTaskCreate→emscripten_async_call,  │ │ │
│  │  │                  xSemaphore→emscripten_sleep-based    │ │ │
│  │  │  ESP.h         - Stubs for heap info                 │ │ │
│  │  │  WiFi/WebServer/WebSockets.h - Empty stubs           │ │ │
│  │  │  HardwareSerial.h - printf→console.log               │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  │                                                           │ │
│  │  ┌─ Web HAL (new) ─────────────────────────────────────┐ │ │
│  │  │  HalDisplay_Web  - 48KB framebuffer + JS callback   │ │ │
│  │  │  HalGPIO_Web     - keyboard state + stubs           │ │ │
│  │  │  HalStorage_Web  - Emscripten MEMFS + IDBFS         │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  │                                                           │ │
│  │  ┌─ Existing Code (unmodified) ────────────────────────┐ │ │
│  │  │  GfxRenderer, EpdFont, Epub, Txt, ZipFile,          │ │ │
│  │  │  miniz, expat, picojpeg, Serialization, Utf8,       │ │ │
│  │  │  All 29 Activities, Settings, State, RecentBooks,    │ │ │
│  │  │  MappedInputManager, UITheme, ButtonNavigator,       │ │ │
│  │  │  CssParser, Hyphenation, JpegToBmpConverter, etc.    │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────────────┘
```

---

## 4. Detailed Implementation Plan

### Phase 1: Build System & Compatibility Shims

**Goal:** Get `emcc` to compile the entire codebase (may not link yet).

#### 1.1 Create CMake build for Emscripten

```
emulator/
├── CMakeLists.txt                    # Main build
├── shims/
│   ├── Arduino.h                     # Arduino compatibility
│   ├── HardwareSerial.h              # Serial → printf
│   ├── SPI.h                         # Empty stub
│   ├── SdFat.h                       # FsFile → FILE*
│   ├── freertos/
│   │   ├── FreeRTOS.h                # FreeRTOS compat
│   │   ├── task.h                    # xTaskCreate shim
│   │   └── semphr.h                  # Semaphore shim
│   ├── esp_sleep.h                   # Sleep stubs
│   ├── WiFi.h                        # Empty
│   ├── WebServer.h                   # Empty
│   └── WebSocketsServer.h           # Empty
├── hal_web/
│   ├── HalDisplay_Web.cpp            # Canvas framebuffer
│   ├── HalGPIO_Web.cpp               # Keyboard input
│   ├── HalStorage_Web.cpp            # MEMFS filesystem
│   ├── BatteryMonitor.h              # Stub
│   ├── InputManager.h                # Stub (unused when EMULATED=1)
│   ├── EInkDisplay.h                 # Stub constants
│   └── SDCardManager.h               # Stub
├── web/
│   ├── index.html                    # Shell page
│   ├── style.css                     # E-ink styling
│   └── shell.js                      # JS bridge
└── main_web.cpp                      # Emscripten entry point
```

#### 1.2 Arduino.h Shim (Key Parts)

```cpp
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// --- Timing ---
unsigned long millis();       // emscripten_get_now()
void delay(unsigned long ms); // emscripten_sleep(ms) with ASYNCIFY
void yield();                 // emscripten_sleep(0)

// --- PROGMEM (no-op in WASM) ---
#define PROGMEM
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#define pgm_read_word(addr) (*(const uint16_t*)(addr))

// --- GPIO stubs ---
#define INPUT 0
#define HIGH 1
#define LOW 0
inline void pinMode(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t) { return LOW; }

// --- Arduino String class ---
class String {
  std::string _str;
public:
  String() = default;
  String(const char* s) : _str(s ? s : "") {}
  String(const std::string& s) : _str(s) {}
  String(int val) : _str(std::to_string(val)) {}
  const char* c_str() const { return _str.c_str(); }
  size_t length() const { return _str.length(); }
  bool isEmpty() const { return _str.empty(); }
  operator const char*() const { return _str.c_str(); }
  String& operator+=(const char* s) { _str += s; return *this; }
  String& operator+=(const String& s) { _str += s._str; return *this; }
  bool operator==(const char* s) const { return _str == s; }
  bool operator==(const String& s) const { return _str == s._str; }
  bool operator!=(const char* s) const { return _str != s; }
  int indexOf(char c) const { auto p = _str.find(c); return p == std::string::npos ? -1 : (int)p; }
  String substring(unsigned int from, unsigned int to = UINT_MAX) const {
    return String(_str.substr(from, to - from));
  }
  // ... (extend as needed based on usage patterns found)
};
```

#### 1.3 FsFile Shim (Key Parts)

```cpp
#pragma once
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>

using oflag_t = int;
constexpr oflag_t O_RDONLY = 0x00;
constexpr oflag_t O_WRONLY = 0x01;
constexpr oflag_t O_RDWR   = 0x02;
constexpr oflag_t O_CREAT  = 0x40;
constexpr oflag_t O_TRUNC  = 0x200;
constexpr oflag_t O_APPEND = 0x400;
constexpr oflag_t O_AT_END = O_APPEND;

class FsFile {
  FILE* _fp = nullptr;
  std::string _path;
  bool _isDir = false;
  DIR* _dir = nullptr;

public:
  FsFile() = default;
  ~FsFile() { close(); }

  // Move semantics (FsFile is passed around by value in some places)
  FsFile(FsFile&& other) noexcept;
  FsFile& operator=(FsFile&& other) noexcept;

  bool open(const char* path, oflag_t flags);
  void close();
  bool isOpen() const { return _fp != nullptr || _dir != nullptr; }
  bool isDir() const { return _isDir; }
  operator bool() const { return isOpen(); }

  // Read/Write
  size_t read(void* buf, size_t nbyte);
  int read() { uint8_t b; return read(&b, 1) == 1 ? b : -1; }
  size_t write(const void* buf, size_t nbyte);
  size_t write(uint8_t b) { return write(&b, 1); }

  // Positioning
  bool seek(uint32_t pos);
  bool seekSet(uint32_t pos) { return seek(pos); }
  uint32_t curPosition();
  uint32_t fileSize();
  size_t available();

  // File info
  bool getName(char* name, size_t size);
  uint32_t size() { return fileSize(); }

  // Directory iteration
  bool openNext(FsFile* dirFile, oflag_t flags = O_RDONLY);
  void rewindDirectory();
};
```

#### 1.4 FreeRTOS Shim with ASYNCIFY

This is the most critical shim. The pattern across all 24 activities is:
1. Create a background task with `[[noreturn]]` loop
2. Synchronize with mutex
3. Background task calls `delay()` between iterations

With ASYNCIFY, `delay()` yields to the browser. The trick is that `xTaskCreate` must launch the task as a cooperatively scheduled coroutine.

```cpp
#pragma once
#include <cstdint>
#include <functional>
#include <vector>

// Minimal FreeRTOS shim using Emscripten ASYNCIFY
// All "tasks" run cooperatively on the main thread via emscripten_async_call

#define portMAX_DELAY 0xFFFFFFFF
#define configMINIMAL_STACK_SIZE 1024

using BaseType_t = int;
using UBaseType_t = unsigned int;
using StackType_t = uint8_t;
using TaskFunction_t = void(*)(void*);

struct TaskControlBlock {
  TaskFunction_t func;
  void* param;
  bool deleted = false;
};
using TaskHandle_t = TaskControlBlock*;

struct SemaphoreData {
  bool locked = false;
};
using SemaphoreHandle_t = SemaphoreData*;

// --- Task Management ---
// With ASYNCIFY, we schedule tasks via emscripten_async_call.
// The task function contains delay() calls that yield back to the browser.
BaseType_t xTaskCreate(TaskFunction_t func, const char* name,
                       uint32_t stackSize, void* param,
                       UBaseType_t priority, TaskHandle_t* handle);

void vTaskDelete(TaskHandle_t handle);

// --- Semaphores ---
SemaphoreHandle_t xSemaphoreCreateMutex();
BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, uint32_t timeout);
BaseType_t xSemaphoreGive(SemaphoreHandle_t sem);
void vSemaphoreDelete(SemaphoreHandle_t sem);
```

**Implementation approach for ASYNCIFY:**

Since all task loops call `delay()` which maps to `emscripten_sleep()`, and ASYNCIFY allows the stack to unwind and resume, the background tasks will naturally yield. The key insight is that with ASYNCIFY, we don't need true threads - the `delay()` inside `displayTaskLoop()` gives the main loop a chance to run.

The execution flow becomes:
```
Browser event loop
  → main loop() runs
    → processes input
    → sets updateRequired = true
  → background task resumes from delay()
    → checks updateRequired
    → renders to framebuffer
    → calls displayBuffer() → triggers JS Canvas update
    → calls delay(N) → yields back to browser
  → (repeat)
```

### Phase 2: Web HAL Implementations

#### 2.1 HalDisplay_Web.cpp

```cpp
#include <HalDisplay.h>
#include <emscripten.h>
#include <cstring>

static uint8_t framebuffer[HalDisplay::BUFFER_SIZE];
static uint8_t grayscaleLsb[HalDisplay::BUFFER_SIZE];
static uint8_t grayscaleMsb[HalDisplay::BUFFER_SIZE];

// Stub EInkDisplay constants for compilation
namespace EInkDisplay_Constants {
  constexpr uint16_t DISPLAY_WIDTH = 480;
  constexpr uint16_t DISPLAY_HEIGHT = 800;
}

HalDisplay::HalDisplay() {}
HalDisplay::~HalDisplay() {}

void HalDisplay::begin() {
  memset(framebuffer, 0xFF, BUFFER_SIZE);  // White screen
}

void HalDisplay::clearScreen(uint8_t color) const {
  memset(framebuffer, color, BUFFER_SIZE);
}

void HalDisplay::drawImage(const uint8_t* imageData, uint16_t x, uint16_t y,
                           uint16_t w, uint16_t h, bool) const {
  // Copy image data into framebuffer at position
  for (int row = 0; row < h && (y + row) < DISPLAY_HEIGHT; row++) {
    size_t srcOffset = row * (w / 8);
    size_t dstOffset = (y + row) * DISPLAY_WIDTH_BYTES + (x / 8);
    size_t bytes = std::min((size_t)(w / 8), (size_t)(DISPLAY_WIDTH_BYTES - x / 8));
    memcpy(&framebuffer[dstOffset], &imageData[srcOffset], bytes);
  }
}

uint8_t* HalDisplay::getFrameBuffer() const {
  return framebuffer;
}

// JS function to render framebuffer to canvas
EM_JS(void, js_render_framebuffer, (const uint8_t* buf, int size, int mode), {
  if (typeof Module.renderFramebuffer === 'function') {
    Module.renderFramebuffer(buf, size, mode);
  }
});

void HalDisplay::displayBuffer(RefreshMode mode, bool) {
  js_render_framebuffer(framebuffer, BUFFER_SIZE, static_cast<int>(mode));
}

void HalDisplay::refreshDisplay(RefreshMode mode, bool turnOff) {
  displayBuffer(mode, turnOff);
}

void HalDisplay::deepSleep() { /* no-op */ }

// Grayscale support
void HalDisplay::copyGrayscaleBuffers(const uint8_t* lsb, const uint8_t* msb) {
  memcpy(grayscaleLsb, lsb, BUFFER_SIZE);
  memcpy(grayscaleMsb, msb, BUFFER_SIZE);
}
void HalDisplay::copyGrayscaleLsbBuffers(const uint8_t* lsb) {
  memcpy(grayscaleLsb, lsb, BUFFER_SIZE);
}
void HalDisplay::copyGrayscaleMsbBuffers(const uint8_t* msb) {
  memcpy(grayscaleMsb, msb, BUFFER_SIZE);
}
void HalDisplay::cleanupGrayscaleBuffers(const uint8_t* bw) {
  memcpy(framebuffer, bw, BUFFER_SIZE);
}

EM_JS(void, js_render_grayscale, (const uint8_t* lsb, const uint8_t* msb, int size), {
  if (typeof Module.renderGrayscale === 'function') {
    Module.renderGrayscale(lsb, msb, size);
  }
});

void HalDisplay::displayGrayBuffer(bool) {
  js_render_grayscale(grayscaleLsb, grayscaleMsb, BUFFER_SIZE);
}
```

#### 2.2 HalGPIO_Web.cpp

```cpp
#include <HalGPIO.h>
#include <emscripten.h>

// Button state managed from JavaScript
// JS calls Module._setButtonState(index, pressed) via exported function
static bool buttonState[7] = {};
static bool buttonPrevState[7] = {};
static bool buttonPressed[7] = {};   // Edge: not pressed → pressed
static bool buttonReleased[7] = {};  // Edge: pressed → not pressed
static unsigned long holdStartTime = 0;
static int heldButton = -1;

extern "C" {
  EMSCRIPTEN_KEEPALIVE void setButtonState(int index, bool pressed) {
    if (index >= 0 && index < 7) buttonState[index] = pressed;
  }
}

void HalGPIO::begin() { /* no-op, no SPI needed */ }

void HalGPIO::update() {
  for (int i = 0; i < 7; i++) {
    buttonPressed[i] = buttonState[i] && !buttonPrevState[i];
    buttonReleased[i] = !buttonState[i] && buttonPrevState[i];
    if (buttonPressed[i]) { holdStartTime = millis(); heldButton = i; }
    if (buttonReleased[i] && heldButton == i) { heldButton = -1; }
    buttonPrevState[i] = buttonState[i];
  }
}

bool HalGPIO::isPressed(uint8_t idx) const { return idx < 7 && buttonState[idx]; }
bool HalGPIO::wasPressed(uint8_t idx) const { return idx < 7 && buttonPressed[idx]; }
bool HalGPIO::wasAnyPressed() const {
  for (int i = 0; i < 7; i++) if (buttonPressed[i]) return true;
  return false;
}
bool HalGPIO::wasReleased(uint8_t idx) const { return idx < 7 && buttonReleased[idx]; }
bool HalGPIO::wasAnyReleased() const {
  for (int i = 0; i < 7; i++) if (buttonReleased[i]) return true;
  return false;
}
unsigned long HalGPIO::getHeldTime() const {
  return heldButton >= 0 ? millis() - holdStartTime : 0;
}

void HalGPIO::startDeepSleep() {
  // Show "device sleeping" overlay in JS
  EM_ASM({ if (typeof Module.onSleep === 'function') Module.onSleep(); });
}

int HalGPIO::getBatteryPercentage() const { return 100; }
bool HalGPIO::isUsbConnected() const { return false; }

HalGPIO::WakeupReason HalGPIO::getWakeupReason() const {
  return WakeupReason::AfterFlash;  // Simulate fresh boot
}
```

#### 2.3 HalStorage_Web.cpp

```cpp
#include <HalStorage.h>
#include <emscripten.h>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>

// Uses Emscripten's MEMFS (auto-provided)
// For persistence: mount IDBFS at /persist and sync

HalStorage HalStorage::instance;
HalStorage::HalStorage() {}

bool HalStorage::begin() {
  // Create default directory structure
  mkdir("/books", true);
  mkdir("/crosspoint", true);
  initialized = true;
  return true;
}

bool HalStorage::ready() const { return initialized; }

std::vector<String> HalStorage::listFiles(const char* path, int maxFiles) {
  std::vector<String> result;
  DIR* dir = opendir(path);
  if (!dir) return result;
  struct dirent* entry;
  while ((entry = readdir(dir)) && (int)result.size() < maxFiles) {
    if (entry->d_name[0] == '.') continue;
    result.push_back(String(entry->d_name));
  }
  closedir(dir);
  return result;
}

String HalStorage::readFile(const char* path) {
  FILE* f = fopen(path, "r");
  if (!f) return String();
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::string content(size, '\0');
  fread(&content[0], 1, size, f);
  fclose(f);
  return String(content);
}

// ... (remaining methods map similarly to POSIX)

FsFile HalStorage::open(const char* path, const oflag_t oflag) {
  FsFile f;
  f.open(path, oflag);
  return f;
}

bool HalStorage::exists(const char* path) {
  struct stat st;
  return stat(path, &st) == 0;
}

bool HalStorage::mkdir(const char* path, const bool) {
  return ::mkdir(path, 0755) == 0 || errno == EEXIST;
}

// ... etc
```

### Phase 3: Web Entry Point

#### 3.1 main_web.cpp

```cpp
#include <emscripten.h>

// Pull in the original main.cpp's setup() and loop()
// We use a web-specific wrapper

extern void setup();
extern void loop();

// Emscripten entry: setup once, then loop via ASYNCIFY
int main() {
  setup();

  // With ASYNCIFY, we can just loop forever - delay() yields
  emscripten_set_main_loop(loop, 0, 0);

  return 0;
}
```

#### 3.2 Conditional Compilation in main.cpp

The original `main.cpp` needs minimal `#ifdef` guards:

```cpp
// At top of main.cpp:
#ifdef __EMSCRIPTEN__
  #define CROSSPOINT_EMULATED 1
#endif

// In setup():
#ifndef __EMSCRIPTEN__
  // Only start serial if USB connected (hardware-specific)
  if (gpio.isUsbConnected()) {
    Serial.begin(115200);
    // ...
  }
#endif

// In loop():
#ifndef __EMSCRIPTEN__
  // ESP-specific memory logging
  if (Serial && millis() - lastMemPrint >= 10000) {
    Serial.printf("...", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMinFreeHeap());
  }
#endif
```

### Phase 4: JavaScript Bridge & Web Shell

#### 4.1 shell.js (Core Bridge)

```javascript
// --- Framebuffer Rendering ---
Module.renderFramebuffer = function(bufPtr, size, refreshMode) {
  const canvas = document.getElementById('display');
  const ctx = canvas.getContext('2d');
  const imageData = ctx.createImageData(480, 800);
  const buf = Module.HEAPU8.subarray(bufPtr, bufPtr + size);

  // Convert 1-bit packed buffer to RGBA
  for (let byte = 0; byte < size; byte++) {
    for (let bit = 7; bit >= 0; bit--) {
      const pixel = (buf[byte] >> bit) & 1;
      const idx = (byte * 8 + (7 - bit)) * 4;
      const color = pixel ? 245 : 15;  // E-ink white/black
      imageData.data[idx]     = color;   // R
      imageData.data[idx + 1] = color;   // G
      imageData.data[idx + 2] = color;   // B
      imageData.data[idx + 3] = 255;     // A
    }
  }

  // Optional: simulate refresh mode
  if (refreshMode === 0) { // FULL_REFRESH
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, 480, 800);
    setTimeout(() => ctx.putImageData(imageData, 0, 0), 150);
  } else {
    ctx.putImageData(imageData, 0, 0);
  }
};

// --- Grayscale Rendering ---
Module.renderGrayscale = function(lsbPtr, msbPtr, size) {
  const canvas = document.getElementById('display');
  const ctx = canvas.getContext('2d');
  const imageData = ctx.createImageData(480, 800);
  const lsb = Module.HEAPU8.subarray(lsbPtr, lsbPtr + size);
  const msb = Module.HEAPU8.subarray(msbPtr, msbPtr + size);

  for (let byte = 0; byte < size; byte++) {
    for (let bit = 7; bit >= 0; bit--) {
      const l = (lsb[byte] >> bit) & 1;
      const m = (msb[byte] >> bit) & 1;
      const gray = (m << 1 | l);  // 0-3
      const color = [15, 95, 175, 245][gray];  // 4 gray levels
      const idx = (byte * 8 + (7 - bit)) * 4;
      imageData.data[idx] = imageData.data[idx+1] = imageData.data[idx+2] = color;
      imageData.data[idx+3] = 255;
    }
  }
  ctx.putImageData(imageData, 0, 0);
};

// --- Input Handling ---
const keyMap = {
  'Escape':   0,  // BTN_BACK
  'Enter':    1,  // BTN_CONFIRM
  'ArrowLeft':  2,  // BTN_LEFT
  'ArrowRight': 3,  // BTN_RIGHT
  'ArrowUp':    4,  // BTN_UP (side)
  'ArrowDown':  5,  // BTN_DOWN (side)
  'PageUp':     4,  // Also maps to side button
  'PageDown':   5,
  'KeyP':       6,  // BTN_POWER
};

document.addEventListener('keydown', (e) => {
  const btn = keyMap[e.code];
  if (btn !== undefined) {
    Module._setButtonState(btn, true);
    e.preventDefault();
  }
});

document.addEventListener('keyup', (e) => {
  const btn = keyMap[e.code];
  if (btn !== undefined) {
    Module._setButtonState(btn, false);
    e.preventDefault();
  }
});

// Virtual buttons (click/touch)
document.querySelectorAll('.device-btn').forEach(el => {
  const btn = parseInt(el.dataset.btn);
  el.addEventListener('pointerdown', () => Module._setButtonState(btn, true));
  el.addEventListener('pointerup', () => Module._setButtonState(btn, false));
  el.addEventListener('pointerleave', () => Module._setButtonState(btn, false));
});

// --- File Loading (drag & drop) ---
const dropZone = document.getElementById('drop-zone');
dropZone.addEventListener('drop', async (e) => {
  e.preventDefault();
  for (const file of e.dataTransfer.files) {
    const data = new Uint8Array(await file.arrayBuffer());
    const path = '/books/' + file.name;
    Module.FS.writeFile(path, data);
    console.log('Loaded:', path, data.length, 'bytes');
  }
});

// --- Sleep overlay ---
Module.onSleep = function() {
  document.getElementById('sleep-overlay').style.display = 'flex';
  // Wake on any button press
};
```

#### 4.2 index.html

```html
<!DOCTYPE html>
<html>
<head>
  <title>CrossPoint Reader Emulator</title>
  <style>
    body { background: #2a2a2a; color: #eee; font-family: system-ui; margin: 0;
           display: flex; justify-content: center; align-items: center; min-height: 100vh; }
    .emulator { display: flex; gap: 24px; align-items: flex-start; }

    /* Device frame */
    .device { background: #1a1a1a; border-radius: 16px; padding: 20px;
              box-shadow: 0 8px 32px rgba(0,0,0,0.5); position: relative; }
    #display { image-rendering: pixelated; border: 1px solid #333;
               /* E-ink appearance */ filter: contrast(0.95) brightness(0.98); }

    /* Buttons */
    .btn-row { display: flex; gap: 8px; justify-content: center; margin-top: 12px; }
    .device-btn { background: #333; border: 1px solid #555; color: #aaa;
                  padding: 8px 16px; border-radius: 6px; cursor: pointer;
                  font-size: 12px; user-select: none; }
    .device-btn:active { background: #555; }
    .side-btns { display: flex; flex-direction: column; gap: 8px; margin-top: 40px; }

    /* Control panel */
    .panel { background: #222; border-radius: 12px; padding: 20px; min-width: 280px; }
    #drop-zone { border: 2px dashed #555; border-radius: 8px; padding: 32px;
                 text-align: center; color: #888; margin: 12px 0; }
    #drop-zone.drag-over { border-color: #4af; background: rgba(68,170,255,0.1); }
    .shortcut { display: flex; justify-content: space-between; padding: 4px 0;
                font-size: 13px; color: #999; }
    kbd { background: #333; padding: 2px 6px; border-radius: 3px; font-size: 11px; }
  </style>
</head>
<body>
  <div class="emulator">
    <div class="device">
      <canvas id="display" width="480" height="800"></canvas>
      <div class="btn-row">
        <button class="device-btn" data-btn="0">Back</button>
        <button class="device-btn" data-btn="1">OK</button>
        <button class="device-btn" data-btn="2">&larr;</button>
        <button class="device-btn" data-btn="3">&rarr;</button>
      </div>
      <div class="btn-row">
        <button class="device-btn" data-btn="4">Pg&uarr;</button>
        <button class="device-btn" data-btn="5">Pg&darr;</button>
        <button class="device-btn" data-btn="6">Power</button>
      </div>
    </div>
    <div class="panel">
      <h3>CrossPoint Reader Emulator</h3>
      <div id="drop-zone"
           ondragover="event.preventDefault(); this.classList.add('drag-over')"
           ondragleave="this.classList.remove('drag-over')">
        Drop EPUB/TXT files here
      </div>
      <h4>Keyboard Shortcuts</h4>
      <div class="shortcut"><span>Back</span><kbd>Esc</kbd></div>
      <div class="shortcut"><span>OK/Confirm</span><kbd>Enter</kbd></div>
      <div class="shortcut"><span>Navigate</span><kbd>&larr; &rarr;</kbd></div>
      <div class="shortcut"><span>Page Turn</span><kbd>PgUp PgDn</kbd></div>
      <div class="shortcut"><span>Power</span><kbd>P</kbd></div>
    </div>
  </div>
  <script src="shell.js"></script>
  <script src="crosspoint-emulator.js"></script>
</body>
</html>
```

### Phase 5: Build Configuration

#### 5.1 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.13)
project(crosspoint-emulator)

set(CMAKE_CXX_STANDARD 20)

# Source directories
set(SRC_DIR ${CMAKE_SOURCE_DIR}/..)
set(LIB_DIR ${SRC_DIR}/lib)
set(APP_DIR ${SRC_DIR}/src)
set(SHIM_DIR ${CMAKE_SOURCE_DIR}/shims)
set(HAL_DIR ${CMAKE_SOURCE_DIR}/hal_web)

# Include paths (shims first to override Arduino/ESP headers)
include_directories(
  ${SHIM_DIR}           # Arduino.h, SPI.h, FreeRTOS shims (MUST BE FIRST)
  ${HAL_DIR}            # BatteryMonitor.h, InputManager.h stubs
  ${LIB_DIR}/hal        # HalDisplay.h, HalGPIO.h, HalStorage.h
  ${LIB_DIR}/GfxRenderer
  ${LIB_DIR}/EpdFont
  ${LIB_DIR}/EpdFont/builtinFonts
  ${LIB_DIR}/Epub
  ${LIB_DIR}/Txt
  ${LIB_DIR}/ZipFile
  ${LIB_DIR}/Serialization
  ${LIB_DIR}/Utf8
  ${LIB_DIR}/miniz
  ${LIB_DIR}/expat
  ${LIB_DIR}/picojpeg
  ${LIB_DIR}/JpegToBmpConverter
  ${LIB_DIR}/FsHelpers
  ${LIB_DIR}/OpdsParser
  ${LIB_DIR}/KOReaderSync
  ${APP_DIR}
)

# Compile definitions
add_definitions(
  -DCROSSPOINT_EMULATED=1
  -DCROSSPOINT_VERSION="emulator"
  -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES=1
  -DEINK_DISPLAY_SINGLE_BUFFER_MODE=1
  -DXML_GE=0
  -DXML_CONTEXT_BYTES=1024
  -DUSE_UTF8_LONG_NAMES=1
  -D__EMSCRIPTEN__
)

# Collect sources (exclude network activities for initial build)
file(GLOB_RECURSE LIB_SOURCES
  ${LIB_DIR}/GfxRenderer/*.cpp
  ${LIB_DIR}/Epub/**/*.cpp
  ${LIB_DIR}/Txt/*.cpp
  ${LIB_DIR}/ZipFile/*.cpp
  ${LIB_DIR}/Utf8/*.cpp
  ${LIB_DIR}/miniz/*.c
  ${LIB_DIR}/expat/*.c
  ${LIB_DIR}/picojpeg/*.c
  ${LIB_DIR}/JpegToBmpConverter/*.cpp
  ${LIB_DIR}/FsHelpers/*.cpp
  ${LIB_DIR}/OpdsParser/*.cpp
  ${LIB_DIR}/KOReaderSync/*.cpp
)

file(GLOB_RECURSE APP_SOURCES ${APP_DIR}/*.cpp)

# HAL web implementations
set(HAL_SOURCES
  ${HAL_DIR}/HalDisplay_Web.cpp
  ${HAL_DIR}/HalGPIO_Web.cpp
  ${HAL_DIR}/HalStorage_Web.cpp
)

# Shim implementations
set(SHIM_SOURCES
  ${SHIM_DIR}/Arduino.cpp
  ${SHIM_DIR}/FsFile.cpp
  ${SHIM_DIR}/FreeRTOS.cpp
)

add_executable(crosspoint-emulator
  main_web.cpp
  ${LIB_SOURCES}
  ${APP_SOURCES}
  ${HAL_SOURCES}
  ${SHIM_SOURCES}
)

# Emscripten link flags
set_target_properties(crosspoint-emulator PROPERTIES
  SUFFIX ".js"
  LINK_FLAGS "\
    -sASYNCIFY \
    -sASYNCIFY_STACK_SIZE=65536 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sINITIAL_MEMORY=67108864 \
    -sEXPORTED_FUNCTIONS=['_main','_setButtonState'] \
    -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS'] \
    -sFORCE_FILESYSTEM=1 \
    --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html \
    -O2 \
  "
)
```

#### 5.2 Build Commands

```bash
# Install Emscripten (one-time)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh

# Build
cd crosspoint-reader/emulator
mkdir build && cd build
emcmake cmake ..
emmake make -j$(nproc)

# Output: crosspoint-emulator.js + crosspoint-emulator.wasm
# Serve: python3 -m http.server 8080
```

---

## 5. Risk Matrix (Revised)

### Low Risk (Proven Approaches)

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| miniz/expat/picojpeg WASM compilation | Low | Very Low | Widely proven, pure C |
| Font bitmap data in WASM | Low | Very Low | Const arrays compile directly |
| GfxRenderer framebuffer ops | Low | Low | Pure pointer math, no hardware |
| Activity lifecycle | Low | Low | Pure virtual methods, std::function |
| Settings/state serialization | Low | Low | Binary read/write on FILE* |

### Medium Risk (Solvable with Effort)

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| FsFile shim completeness | Medium | Medium | ~200 LOC; test incrementally as each activity exercises new FsFile methods |
| Arduino String compatibility | Medium | Medium | ~100 LOC wrapper; most code uses std::string |
| ASYNCIFY binary size | Medium | High | Expected 30-50% increase (3-7MB); mitigate with `-Oz` + gzip |
| 24-activity FreeRTOS pattern | High | Medium | Single shim file covers all; ASYNCIFY makes delay() cooperative |
| GfxRenderer chunked allocation | Low | Medium | May need `#ifdef` to use single malloc instead of 6x8KB chunks |

### High Risk (Needs Careful Handling)

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| ASYNCIFY + FreeRTOS interaction | High | Medium | The `[[noreturn]] displayTaskLoop()` pattern with infinite `while(true)` + `delay()` must correctly yield. Test with simplest activity first (BootActivity) |
| Network activities in WASM | Medium | High | WiFi/WebServer/WebSocket won't compile. **Exclude at build level** with CMake; stub the includes |
| `open-x4-sdk` header dependencies | High | Medium | SDK headers not in repo (git submodule). Must create stub headers for `EInkDisplay.h`, `InputManager.h`, `BatteryMonitor.h`, `SDCardManager.h` |
| ESP32-specific APIs in main.cpp | Medium | Low | `ESP.getFreeHeap()`, `esp_sleep_*`, `esp_reset_reason()` - all behind `#ifdef __EMSCRIPTEN__` |

---

## 6. Development Phases & Milestones

### Phase 1: Skeleton Build (Milestone: `emcc` compiles and links)
1. Create `emulator/` directory structure
2. Write Arduino.h shim (String, Print, Serial, millis, delay, PROGMEM)
3. Write FsFile/SdFat shim
4. Write FreeRTOS shim (task + semaphore with ASYNCIFY)
5. Write stub headers for open-x4-sdk libraries
6. Write empty stubs for WiFi/WebServer/WebSocket
7. Write HalDisplay_Web, HalGPIO_Web, HalStorage_Web
8. Create CMakeLists.txt
9. Iterate: compile, fix errors, repeat until clean build

### Phase 2: First Frame (Milestone: BootActivity renders splash screen)
1. Create web shell (index.html + shell.js)
2. Implement JS framebuffer-to-Canvas rendering
3. Implement main_web.cpp entry point
4. Boot to BootActivity
5. Verify: splash screen appears on Canvas

### Phase 3: Navigation (Milestone: Home screen with button input)
1. Implement keyboard → button state bridge
2. Implement virtual button clicks
3. Navigate: Boot → Home screen
4. Verify: can scroll menu items, select options

### Phase 4: Core Reading (Milestone: Open and read an EPUB)
1. Implement file drag-and-drop → MEMFS
2. Navigate: Home → My Library → select EPUB
3. Verify: EPUB opens, pages render, page turns work
4. Fix any FsFile shim gaps discovered during EPUB loading

### Phase 5: Full Feature Parity (Milestone: All non-network activities work)
1. Test all reader activities (EPUB, TXT, XTC)
2. Test settings activities
3. Test chapter selection, percent jump, bookmarks
4. Test cover image rendering (grayscale)
5. Fix remaining shim issues

### Phase 6: Polish (Milestone: Production-ready demo)
1. E-ink refresh animation (black flash on FULL_REFRESH)
2. Device frame SVG for visual authenticity
3. IDBFS persistence (books survive page reload)
4. Mobile touch support
5. Loading indicator during EPUB processing
6. Console log panel for debug output
7. URL parameter to load EPUB from remote URL

---

## 7. What We Have vs. What's Needed (Final Inventory)

### Reusable As-Is (~20,000 LOC)
| Component | Files | LOC (est.) |
|-----------|-------|------------|
| EPUB engine (parse, layout, render) | 30+ | ~5,000 |
| GfxRenderer (all drawing primitives) | 6 | ~3,500 |
| EpdFont (bitmap fonts, 14 families) | 4 + data | ~1,000 + data |
| All 29 activities | 60+ | ~6,000 |
| Settings, State, RecentBooks | 6 | ~800 |
| MappedInputManager | 2 | ~110 |
| Third-party (miniz, expat, picojpeg) | 12 | ~5,000 |
| Utilities (Utf8, ZipFile, Serialization, etc.) | 10+ | ~1,000 |

### Must Create (~2,500 LOC new)
| Component | LOC (est.) | Complexity |
|-----------|------------|------------|
| Arduino.h shim | ~250 | Medium |
| FsFile/SdFat.h shim | ~250 | Medium |
| FreeRTOS shim | ~100 | High (ASYNCIFY interaction) |
| Stub headers (SDK, WiFi, ESP) | ~150 | Low |
| HalDisplay_Web.cpp | ~120 | Low |
| HalGPIO_Web.cpp | ~100 | Low |
| HalStorage_Web.cpp | ~150 | Medium |
| main_web.cpp | ~30 | Low |
| CMakeLists.txt | ~80 | Medium |
| shell.js (JS bridge) | ~300 | Medium |
| index.html + style.css | ~400 | Low |
| **Total** | **~1,930** | |

### Must Modify (~200 LOC changes to existing files)
| File | Change | LOC |
|------|--------|-----|
| main.cpp | `#ifdef __EMSCRIPTEN__` guards for ESP-specific code | ~30 |
| GfxRenderer.cpp | Optional: `#ifdef` for chunked vs single allocation | ~10 |
| Activity.h | Include path adjustment (HardwareSerial.h → our shim) | ~2 |
| Various activities | None if FreeRTOS shim works correctly | 0 |
| **Total** | | **~42** |

### Can Drop Entirely (Not Needed)
- WiFi/WebServer/WebSocket code (5 source files)
- OTA update system
- Calibre connectivity
- Deep sleep / power management logic in main.cpp
- Battery monitoring
- SPI initialization
- USB detection

---

## 8. Conclusion

The CrossPoint Reader web emulator is **feasible and well-scoped**. The clean HAL architecture means we replace ~350 LOC of hardware code to unlock ~20,000 LOC of reusable application logic.

**Key architectural decision:** Use Emscripten ASYNCIFY to handle the pervasive FreeRTOS task+mutex pattern across 24 activities. This single choice avoids refactoring the entire activity codebase while producing correct cooperative multitasking behavior in the browser.

**The critical path is:**
1. Get the FreeRTOS shim working with ASYNCIFY (highest risk, highest reward)
2. Get BootActivity rendering to Canvas (proves the full pipeline)
3. Get EPUB reading working (proves FsFile shim + Epub pipeline + GfxRenderer)

Once those three milestones are hit, the rest is incremental work with decreasing risk.
