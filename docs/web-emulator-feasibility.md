# Web Emulator Feasibility Analysis

## Executive Summary

**Verdict: FEASIBLE, with significant but well-scoped effort.**

The CrossPoint Reader firmware has a clean 3-layer architecture (HAL → Libraries → Application) that makes a web emulator achievable. The codebase already contains a `CROSSPOINT_EMULATED` compile flag in the HAL layer, indicating emulation was considered from the start. Two viable approaches exist: **Emscripten/WASM compilation** (recommended) or **pure JavaScript reimplementation**. The WASM approach reuses ~80% of existing C++ code and produces a pixel-accurate emulation of the device.

---

## 1. Current Architecture Assessment

### 1.1 Three-Layer HAL Design

The firmware has clean separation between hardware and logic:

```
┌─────────────────────────────────────────────────────┐
│  APPLICATION LAYER (src/)                            │
│  Activities, Settings, Input Mapping, Main Loop      │
├─────────────────────────────────────────────────────┤
│  LIBRARY LAYER (lib/)                                │
│  Epub, GfxRenderer, EpdFont, ZipFile, Serialization  │
│  Txt, OpdsParser, KOReaderSync, Utf8, FsHelpers     │
├─────────────────────────────────────────────────────┤
│  HAL LAYER (lib/hal/)                                │
│  HalDisplay, HalGPIO, HalStorage                     │
│  ↓ wraps ↓                                           │
│  EInkDisplay, InputManager, BatteryMonitor,          │
│  SDCardManager (from open-x4-sdk)                    │
└─────────────────────────────────────────────────────┘
```

### 1.2 Key Metrics

| Metric | Value |
|--------|-------|
| Total source files | 252 (170 headers, 82 .cpp) |
| Application code (src/) | ~5,000 LOC |
| Library code (lib/) | ~15,000 LOC |
| HAL layer (lib/hal/) | ~350 LOC |
| Third-party (miniz, expat, picojpeg) | ~5,000 LOC |
| Display resolution | 480x800 pixels, 1-bit B&W |
| Frame buffer size | 48,000 bytes (480*800/8) |
| Target MCU | ESP32-C3 (RISC-V, ~380KB RAM) |

### 1.3 Existing Emulation Infrastructure

The HAL already has a conditional compilation guard:

```cpp
// lib/hal/HalGPIO.h:22-24
#if CROSSPOINT_EMULATED == 0
  InputManager inputMgr;
#endif
```

This means the authors already anticipated non-hardware builds. However, no emulated implementations currently exist.

---

## 2. Component-by-Component Analysis

### 2.1 Hardware Abstraction Layer (MUST REPLACE)

#### HalDisplay (lib/hal/HalDisplay.h) - **REPLACE ENTIRELY**
- Wraps `EInkDisplay` from open-x4-sdk via SPI
- 480x800 1-bit framebuffer (48KB)
- Three refresh modes: FULL, HALF, FAST
- Grayscale buffer support (2-bit)
- **Web replacement:** HTML5 Canvas rendering the framebuffer as an image

#### HalGPIO (lib/hal/HalGPIO.h) - **REPLACE ENTIRELY**
- 7 physical buttons with debounce via `InputManager`
- Deep sleep / wake management
- Battery ADC reading on GPIO0
- USB detection on GPIO20
- **Web replacement:** Keyboard event listeners mapped to virtual buttons

#### HalStorage (lib/hal/HalStorage.h) - **REPLACE ENTIRELY**
- Wraps `SDCardManager` from open-x4-sdk
- Full filesystem: open, read, write, mkdir, remove, list, exists
- `FsFile` type used pervasively throughout codebase
- **Web replacement:** Emscripten MEMFS/IDBFS or IndexedDB-backed virtual filesystem

### 2.2 Graphics Renderer (ADAPT)

#### GfxRenderer (lib/GfxRenderer/) - **MINOR CHANGES**
- Operates on raw framebuffer via `HalDisplay::getFrameBuffer()`
- Drawing primitives: pixels, lines, rects, arcs, polygons, text, bitmaps
- Coordinate rotation for 4 orientations
- Chunked buffer allocation for ESP32 memory fragmentation
- **Web adaptation:** Point `getFrameBuffer()` to a WASM memory buffer; remove chunked allocation (unnecessary in browser)

#### EpdFont (lib/EpdFont/) - **NO CHANGES**
- Pre-compiled bitmap font data (Bookerly, NotoSans, OpenDyslexic, Ubuntu)
- Glyph lookup by Unicode codepoint
- 1-bit and 2-bit glyph rendering
- Pure C++ with no hardware dependencies
- Fonts compiled as const arrays in flash - **works directly in WASM**

### 2.3 EPUB Pipeline (MOSTLY PORTABLE)

#### Epub (lib/Epub/) - **MINOR CHANGES**
- ZIP extraction → XML parsing (Expat) → CSS parsing → page layout
- Depends on: `FsFile`, `Print`, `HalStorage`, `Serial.printf()`
- Core parsing logic is pure C++ algorithms
- **Web adaptation:** Replace `FsFile` with Emscripten file handles; `Print` interface maps to buffer

#### Section/Page/TextBlock - **NO CHANGES**
- Page layout and text rendering are pure algorithms
- Operate on `GfxRenderer` reference (already abstracted)
- Hyphenation engine is pure C++ (Liang algorithm with language tries)

### 2.4 Third-Party Libraries (ALL PORTABLE)

| Library | Purpose | WASM Status |
|---------|---------|-------------|
| **miniz** | ZIP deflate/inflate | Pure C, compiles to WASM directly |
| **expat** | XML parsing | Pure C, widely compiled to WASM |
| **picojpeg** | JPEG decoding | Pure C, minimal footprint |

### 2.5 Application Layer (ADAPT THREADING)

#### Activity System (src/activities/) - **MODERATE CHANGES**
- Base class `Activity` with `onEnter()`, `loop()`, `onExit()` lifecycle
- 20+ activity implementations
- Some activities (HomeActivity, EpubReaderActivity) use **FreeRTOS tasks and semaphores** for background rendering
- **Web adaptation:** Replace FreeRTOS tasks with `emscripten_set_main_loop()` or Web Workers

#### Main Loop (src/main.cpp) - **REWRITE FOR WEB**
- Arduino `setup()` + `loop()` pattern
- Power management, sleep/wake logic (not applicable to web)
- Activity navigation via function pointers (portable)
- **Web adaptation:** Convert to `emscripten_set_main_loop()` callback

### 2.6 Network/Sync (OPTIONAL - CAN SKIP)

| Component | Needed for Emulator? |
|-----------|---------------------|
| WiFi/WebServer | No - device-specific |
| KOReader Sync | Optional - use `fetch()` API if needed |
| OPDS Browser | Optional - use `fetch()` API if needed |
| OTA Updates | No - not applicable |

---

## 3. Approach Comparison

### Approach A: Emscripten/WASM Compilation (RECOMMENDED)

Compile the existing C++ codebase to WebAssembly using Emscripten, replacing only the HAL layer.

**Pros:**
- Reuses ~80% of existing code unmodified
- Pixel-accurate rendering (same GfxRenderer, same fonts, same layout)
- Bug-for-bug compatible with firmware behavior
- Changes stay in sync as firmware evolves
- Emscripten provides POSIX filesystem emulation (MEMFS, IDBFS)
- miniz/expat/picojpeg compile directly to WASM

**Cons:**
- WASM binary size (~2-5MB estimated, based on font data)
- Emscripten learning curve
- FreeRTOS threading requires adaptation (Emscripten has pthread support via Web Workers, but it adds complexity)
- Arduino `String` class needs a shim or replacement with `std::string`

**Architecture:**
```
┌──────────────────────────────────────────────────────┐
│  BROWSER                                              │
│  ┌──────────────────────────────────────────────────┐ │
│  │ Web Shell (HTML/CSS/JS)                          │ │
│  │ - Canvas element (480x800 or scaled)             │ │
│  │ - Keyboard/touch event capture                    │ │
│  │ - File drag-and-drop for EPUB loading            │ │
│  │ - Virtual button overlay                          │ │
│  ├──────────────────────────────────────────────────┤ │
│  │ JavaScript Bridge                                 │ │
│  │ - Input events → WASM function calls             │ │
│  │ - Framebuffer → Canvas ImageData transfer        │ │
│  │ - File API → Emscripten FS bridge                │ │
│  ├──────────────────────────────────────────────────┤ │
│  │ WASM Module (compiled C++)                        │ │
│  │ - All lib/ code (Epub, GfxRenderer, EpdFont...)  │ │
│  │ - All src/ activities                             │ │
│  │ - Web HAL implementations                         │ │
│  │   - HalDisplay → Canvas framebuffer              │ │
│  │   - HalGPIO → JS event bridge                    │ │
│  │   - HalStorage → Emscripten MEMFS/IDBFS         │ │
│  └──────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────┘
```

### Approach B: Pure JavaScript Reimplementation

Rewrite the entire reader in JavaScript/TypeScript using browser-native APIs.

**Pros:**
- Native browser integration (smaller bundle)
- Can use existing JS EPUB libraries (epub.js, readium)
- No WASM complexity
- Better debugging experience

**Cons:**
- Complete rewrite (~20,000 LOC)
- Will NOT be pixel-accurate (different rendering engine)
- Diverges from firmware immediately (two codebases to maintain)
- Loses all the custom typography (hyphenation, font rendering, page layout)
- Doesn't actually "emulate" the device - it's a different reader

### Approach C: Hybrid (WASM core + JS UI shell)

Use WASM for the EPUB parsing/rendering pipeline, JS for the UI chrome.

**Pros:**
- Best of both worlds
- EPUB rendering is pixel-accurate
- UI can use modern web patterns
- Smaller WASM module (only lib/ code)

**Cons:**
- More complex JS↔WASM bridge
- Activity system split across two languages
- Still needs HAL replacement

### Recommendation: **Approach A (Emscripten/WASM)** for true emulation

If the goal is to **emulate the device** (same UI, same rendering, same behavior), Approach A is the only viable path. The other approaches create a different product.

---

## 4. Detailed Implementation Plan

### Phase 1: Build Infrastructure

**Goal:** Get the existing codebase compiling with Emscripten.

1. Create `platformio` environment or standalone CMake build for Emscripten
2. Create Arduino compatibility shim:
   - `Arduino.h` → Provide `String`, `Print`, `millis()`, `delay()`, `Serial`, `PROGMEM`
   - `SPI.h` → Empty stub (not needed)
   - `WiFi.h` / `WebServer.h` / `WebSockets.h` → Empty stubs
3. Create `SdFat` compatibility shim:
   - `FsFile` class backed by Emscripten MEMFS (`fopen`/`fread`/`fwrite`)
   - `oflag_t` constants mapping to POSIX flags
4. Create web HAL implementations:
   - `HalDisplay_Web.cpp` → Framebuffer in WASM linear memory
   - `HalGPIO_Web.cpp` → Input state from JS bridge
   - `HalStorage_Web.cpp` → Emscripten filesystem API
5. Compile with `emcc` and verify it links

**Key types needing shims:**

| Arduino Type | Web Shim |
|-------------|----------|
| `String` | Thin wrapper around `std::string` |
| `Print` | Base class with `write()` → buffer |
| `FsFile` | Wrapper around `FILE*` (Emscripten MEMFS) |
| `Serial` | `printf()` to browser console |
| `millis()` | `emscripten_get_now()` or `clock()` |
| `delay(ms)` | `emscripten_sleep(ms)` (with ASYNCIFY) |
| `yield()` | `emscripten_sleep(0)` |
| `PROGMEM` | No-op (no flash/RAM distinction in WASM) |
| `ESP.getFreeHeap()` | Stub returning large value |
| `xTaskCreate()` | `pthread_create()` or single-threaded adaptation |
| `xSemaphoreTake()` | `pthread_mutex_lock()` or no-op |

### Phase 2: Web HAL Implementations

#### HalDisplay_Web
```
- Allocate 48KB framebuffer in WASM memory
- Expose framebuffer pointer to JavaScript
- On displayBuffer(): call JS function to render framebuffer to Canvas
- Refresh mode → visual effect (optional: simulate e-ink flash animation)
- grayscale buffers → 2-bit to 8-bit expansion for Canvas
```

#### HalGPIO_Web
```
- Maintain button state array [7] in WASM memory
- JS keyboard handler writes to state array:
  Escape → BTN_BACK, Enter → BTN_CONFIRM
  Arrow keys → BTN_LEFT/RIGHT/UP/DOWN
  P → BTN_POWER
- getBatteryPercentage() → return 100
- isUsbConnected() → return false
- startDeepSleep() → no-op or show "device sleeping" overlay
- getWakeupReason() → return AfterFlash
```

#### HalStorage_Web
```
- Use Emscripten MEMFS for runtime filesystem
- On startup: mount IDBFS for persistence across sessions
- File drag-and-drop: JS writes files to MEMFS, triggers FS sync
- All FsFile operations map to standard POSIX file I/O through Emscripten
```

### Phase 3: Main Loop Adaptation

The Arduino `setup()`/`loop()` pattern must be converted to browser-compatible execution:

**Option A: Emscripten ASYNCIFY**
```cpp
// Keep setup() + loop() structure
// Emscripten ASYNCIFY allows delay() and yield() to work
// Compile with: -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536
void main() {
  setup();
  while(true) {
    loop();
  }
}
```

**Option B: emscripten_set_main_loop**
```cpp
// Convert to callback-based loop
void main() {
  setup();
  emscripten_set_main_loop(loop, 0, 1);  // 0 = use requestAnimationFrame
}
```

Option A is simpler but increases WASM binary size due to ASYNCIFY instrumentation.
Option B is more efficient but requires ensuring `loop()` never blocks.

### Phase 4: Threading Adaptation

Two activities use FreeRTOS tasks: `HomeActivity` and `EpubReaderActivity`.

**Options:**
1. **Emscripten pthreads** (requires SharedArrayBuffer, COOP/COEP headers)
2. **Single-threaded adaptation** - inline the background work into the main loop
3. **Web Workers** - offload rendering to a worker

Single-threaded adaptation is simplest: move the task body into the `loop()` method with a state machine pattern. The background tasks primarily do page rendering and cover thumbnail generation, which can be spread across multiple `loop()` iterations.

### Phase 5: Web Shell (HTML/CSS/JS)

```
┌─────────────────────────────────────────┐
│  ┌─────────────────────────────────┐    │
│  │                                 │    │
│  │    Canvas (480x800 scaled)      │    │
│  │    E-ink appearance CSS filter  │    │
│  │                                 │    │
│  │                                 │    │
│  │                                 │    │
│  └─────────────────────────────────┘    │
│                                          │
│  [Back] [<] [>] [OK] [Up] [Dn] [Pwr]   │
│                                          │
│  ┌──────────────────────────────────┐   │
│  │ Drop EPUB files here             │   │
│  └──────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

**Features:**
- Canvas element with CSS filter to simulate e-ink look (grayscale, slight blur)
- Virtual button bar (clickable) + keyboard shortcuts
- Drag-and-drop zone for loading EPUB files into the virtual SD card
- Optional: device frame SVG overlay for visual authenticity
- Responsive scaling for different screen sizes

### Phase 6: Polish and Optional Features

- E-ink refresh animation (black flash on FULL_REFRESH)
- Grayscale rendering for cover images
- LocalStorage/IndexedDB persistence (books survive page refresh)
- URL parameter to load EPUB from URL
- Share button to export reading progress
- Mobile touch support (swipe for page turn)

---

## 5. Risk Assessment

### Low Risk
| Item | Reason |
|------|--------|
| Epub parsing | Pure C++ algorithms, well-abstracted |
| Font rendering | Compiled bitmap data, no hardware deps |
| Page layout | Pure algorithms operating on GfxRenderer |
| ZIP/XML/JPEG | Standard C libraries, widely WASM-proven |
| Hyphenation | Pure C++ Liang algorithm |
| Settings/state | Binary serialization, filesystem-only dependency |

### Medium Risk
| Item | Reason | Mitigation |
|------|--------|-----------|
| FsFile shim completeness | Used in ~40 files with varied patterns (seek, tell, read, write) | Emscripten MEMFS handles most POSIX semantics |
| Arduino String | Used alongside std::string; implicit conversions | Create thin wrapper, gradually migrate to std::string |
| Threading model | FreeRTOS tasks in HomeActivity and EpubReaderActivity | Single-thread adaptation or Emscripten pthreads |
| WASM binary size | Font data alone may be 1-2MB | Tree-shake unused fonts; use WASM compression |
| Memory usage | Original targets 380KB; browser has GB | Not a real risk - may need to remove artificial limits |

### High Risk
| Item | Reason | Mitigation |
|------|--------|-----------|
| ASYNCIFY overhead | If using delay()/yield(), ASYNCIFY increases binary by 30-50% | Use emscripten_set_main_loop instead; refactor blocking calls |
| Open-X4-SDK dependencies | SDCardManager, InputManager, BatteryMonitor, EInkDisplay are closed-source symlinks | Replace entirely with web HAL; don't try to compile SDK |
| Cross-browser SharedArrayBuffer | Required for pthreads; needs COOP/COEP headers | Use single-threaded approach to avoid requirement |

---

## 6. What We Have vs. What's Needed

### What We Have (Reusable as-is)
- Complete EPUB 2 & 3 parsing engine
- CSS parser and style application
- Page layout and pagination engine
- Hyphenation for 6 languages
- GfxRenderer drawing primitives
- 14 pre-compiled font families (4 styles each)
- Text rendering with Unicode support
- Image dithering algorithms (Atkinson, Floyd-Steinberg)
- JPEG decoding (picojpeg)
- ZIP handling (miniz)
- XML parsing (expat)
- Activity-based navigation system
- Settings management
- Binary serialization
- Recent books tracking
- UTF-8 utilities
- Path normalization

### What Must Be Created
1. **Arduino compatibility shim** (~300 LOC)
   - `String`, `Print`, `Serial`, `millis()`, `delay()`, `PROGMEM`
2. **FsFile / SdFat shim** (~200 LOC)
   - Map `FsFile` methods to POSIX `FILE*` operations
3. **HalDisplay_Web** (~100 LOC)
   - Framebuffer allocation + JS bridge for Canvas rendering
4. **HalGPIO_Web** (~80 LOC)
   - Button state from keyboard events, stub battery/sleep
5. **HalStorage_Web** (~50 LOC)
   - Delegate to Emscripten MEMFS (mostly identical to current impl but using FILE*)
6. **Web main loop** (~50 LOC)
   - `emscripten_set_main_loop()` setup
7. **CMakeLists.txt / build config** (~100 LOC)
   - Emscripten build with proper flags
8. **Web shell** (~500 LOC HTML/CSS/JS)
   - Canvas, virtual buttons, file drop zone, keyboard handling
9. **JS↔WASM bridge** (~200 LOC JS)
   - Framebuffer transfer, input events, file system operations
10. **Threading adaptation** (~200 LOC changes)
    - Convert FreeRTOS usage in HomeActivity and EpubReaderActivity

**Total new code: ~1,800 LOC**
**Total modified existing code: ~500 LOC (mostly `#ifdef CROSSPOINT_EMULATED` guards)**

### What Can Be Dropped (Not Needed for Web)
- WiFi / WebServer / WebSocket code
- OTA update system
- Deep sleep / power management
- Battery monitoring
- Serial debugging (redirect to console.log)
- SPI initialization
- Calibre connectivity

---

## 7. Estimated Effort Breakdown

| Phase | Description | Scope |
|-------|-------------|-------|
| **Phase 1** | Build infrastructure + Arduino/SdFat shims | CMake, shims, first compile |
| **Phase 2** | Web HAL implementations | HalDisplay, HalGPIO, HalStorage for web |
| **Phase 3** | Main loop + threading adaptation | Convert Arduino loop, fix FreeRTOS usage |
| **Phase 4** | Web shell + JS bridge | HTML/Canvas UI, input handling, file loading |
| **Phase 5** | Integration testing + bug fixes | End-to-end EPUB reading |
| **Phase 6** | Polish | E-ink effects, persistence, mobile support |

---

## 8. Conclusion

The CrossPoint Reader firmware is **well-suited for web emulation** due to its clean HAL architecture. The key insight is that the HAL layer is only ~350 lines of code, while the reusable library and application code is ~20,000 lines. The ratio heavily favors porting.

The `CROSSPOINT_EMULATED` flag already in the codebase suggests this was an anticipated path. The recommended approach (Emscripten/WASM) would produce a pixel-accurate emulator that stays in sync with firmware development, requiring only ~1,800 lines of new code and ~500 lines of modifications to existing code.

The primary technical challenges are:
1. **FsFile shim** - Emscripten's MEMFS handles this well
2. **FreeRTOS threading** - Solvable with single-threaded adaptation
3. **Arduino type compatibility** - Small shim layer needed

None of these are blocking issues. The project is feasible.
