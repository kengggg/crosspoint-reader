# Web Emulator for CrossPoint Reader: Implementation Plan

## What Are We Building?

We want to run the CrossPoint Reader firmware **inside a web browser** -- so anyone can try it without owning the physical Xteink X4 device. They open a webpage, see the e-ink screen, press virtual buttons, drop in an EPUB file, and read it exactly as the real device would display it.

### The Core Idea

The CrossPoint firmware is written in C++. It talks to hardware (screen, buttons, SD card). We can't run hardware in a browser, but we CAN:

1. **Compile the C++ code to WebAssembly** (WASM) -- this is what Emscripten does. It takes C++ and produces code that browsers can run.
2. **Fake the hardware** -- instead of talking to a real e-ink screen, we render to an HTML Canvas. Instead of reading physical buttons, we read keyboard presses. Instead of an SD card, we use the browser's filesystem.

The firmware never knows the difference. It calls `display.drawPixel(x, y)` and doesn't care whether that goes to real e-ink or to a Canvas.

### Why This Works

The firmware already separates hardware from logic via 3 files called the "HAL" (Hardware Abstraction Layer):

```
Firmware code                    HAL                      Hardware
─────────────                    ───                      ────────
"draw a pixel at (10,20)"  →  HalDisplay  →  E-Ink Screen
"is Back button pressed?"  →  HalGPIO     →  Physical Buttons
"read file /books/my.epub" →  HalStorage  →  SD Card
```

For the emulator, we just swap what's on the right side:

```
Firmware code (UNCHANGED)        HAL (OUR NEW VERSION)    Browser
─────────────────────────        ────────────────────     ───────
"draw a pixel at (10,20)"  →  HalDisplay_Web  →  HTML Canvas
"is Back button pressed?"  →  HalGPIO_Web     →  Keyboard Events
"read file /books/my.epub" →  HalStorage_Web  →  Browser Memory
```

---

## The 7 Domains

The emulator is built from 7 independent domains. Each one solves one problem. Here they are in the order we'd build them:

```
┌──────────────────────────────────────────────────────────┐
│                    WEB EMULATOR                           │
│                                                           │
│  Domain 1: Build System         "How to compile it"       │
│  Domain 2: Arduino Shim         "Fake the Arduino APIs"   │
│  Domain 3: Filesystem Shim      "Fake the SD card"        │
│  Domain 4: Threading Shim       "Fake the multitasking"   │
│  Domain 5: Display HAL          "Fake the e-ink screen"   │
│  Domain 6: Input HAL            "Fake the buttons"        │
│  Domain 7: Web Shell            "The webpage itself"      │
│                                                           │
└──────────────────────────────────────────────────────────┘
```

---

## Domain 1: Build System

### What problem does this solve?

The firmware currently builds with PlatformIO targeting an ESP32 chip. We need a second build target that compiles the **same C++ source files** but outputs a `.wasm` file for browsers instead of a `.bin` file for the ESP32.

### What is Emscripten?

Emscripten is a compiler (like GCC or Clang) that takes C/C++ code and outputs WebAssembly instead of machine code. You use it almost identically to a normal compiler:

```bash
# Normal: compile C++ to native binary
g++ main.cpp -o program

# Emscripten: compile C++ to WASM
emcc main.cpp -o program.js    # produces program.js + program.wasm
```

### What do we need?

A `CMakeLists.txt` file (build recipe) that tells Emscripten:
- Which source files to compile (all the firmware `.cpp` files)
- Where to find headers (our shim headers FIRST, then firmware headers)
- What compile flags to use (same `-D` defines as `platformio.ini`)
- What Emscripten features to enable (ASYNCIFY for threading, filesystem support)

### What does the directory look like?

```
crosspoint-reader/                  ← existing firmware repo (UNTOUCHED)
├── src/                            ← firmware source
├── lib/                            ← firmware libraries
├── platformio.ini                  ← existing ESP32 build config
│
└── emulator/                       ← ALL NEW CODE LIVES HERE
    ├── CMakeLists.txt              ← build recipe for Emscripten
    ├── shims/                      ← Domain 2, 3, 4 (fake headers)
    ├── hal_web/                    ← Domain 5, 6 (fake hardware)
    └── web/                        ← Domain 7 (the webpage)
```

**Key point:** We never modify files outside `emulator/`. The firmware compiles unmodified because our shim headers "pretend" to be Arduino/FreeRTOS/SdFat.

### How does the include trick work?

When the firmware says `#include <Arduino.h>`, the compiler searches include paths in order. We put our `emulator/shims/` directory FIRST:

```
Search order:
  1. emulator/shims/Arduino.h        ← FOUND! Uses our fake version
  2. ~/.platformio/packages/Arduino.h ← Never reached
```

So the firmware includes our fake `Arduino.h` without knowing it.

---

## Domain 2: Arduino Compatibility Shim

### What problem does this solve?

The firmware uses Arduino-specific types and functions everywhere. The browser doesn't have Arduino. We need to provide fake versions of everything the firmware expects from Arduino.

### What does the firmware use from Arduino?

| Arduino Thing | What it does | How many places | Our fake version |
|---------------|-------------|-----------------|------------------|
| `String` | Text type (like `std::string`) | ~50 places | Thin class wrapping `std::string` |
| `Serial.printf(...)` | Debug logging | ~580 places | Redirects to `printf()` → browser console |
| `millis()` | Milliseconds since boot | ~80 places | Uses Emscripten's clock |
| `delay(ms)` | Wait N milliseconds | ~40 places | `emscripten_sleep(ms)` (yields to browser) |
| `yield()` | Let other tasks run | ~5 places | `emscripten_sleep(0)` |
| `PROGMEM` | "Store in flash memory" | ~20 places | Does nothing (WASM has no flash/RAM split) |
| `pinMode()` | Configure a GPIO pin | 2 places | Does nothing |
| `digitalRead()` | Read a GPIO pin | 1 place | Returns 0 |
| `Print` | Base class for output streams | ~30 places | Simple class with `write()` method |

### Example: How the `String` shim works

The firmware writes code like:
```cpp
String title = epub.getTitle();
Serial.printf("Reading: %s\n", title.c_str());
```

Our fake `String` class makes this work:
```cpp
class String {
  std::string _str;   // Real storage
public:
  String(const char* s) : _str(s) {}
  const char* c_str() const { return _str.c_str(); }
  // ... other methods the firmware uses
};
```

### Example: How `millis()` works

On real Arduino, `millis()` returns milliseconds since the chip powered on. In the browser:
```cpp
unsigned long millis() {
  return (unsigned long)emscripten_get_now();  // Browser's high-res timer
}
```

### Example: How `Serial` works

On real hardware, `Serial` sends text over USB. In the browser:
```cpp
class FakeSerial {
  void printf(const char* fmt, ...) {
    // Just call standard printf, which Emscripten routes to console.log
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }
};
FakeSerial Serial;
```

---

## Domain 3: Filesystem Shim

### What problem does this solve?

The firmware reads and writes files on an SD card. The browser doesn't have an SD card. We need to fake a filesystem.

### Good news: Emscripten already has this

Emscripten provides a virtual filesystem called **MEMFS** (Memory File System). It works exactly like a real filesystem (`fopen`, `fread`, `fwrite`, directories) but lives in browser memory. We just need to bridge the gap between the firmware's `FsFile` type and standard C `FILE*`.

### What is `FsFile`?

`FsFile` is a class from the SdFat library (Arduino SD card library). The firmware uses it to read/write files. It has methods like:

```cpp
FsFile file = Storage.open("/books/my.epub", O_RDONLY);
file.read(buffer, 256);     // Read 256 bytes
file.seek(1024);            // Jump to position 1024
uint32_t size = file.fileSize();
file.close();
```

### Our fake FsFile

We create a `FsFile` class that wraps the standard C `FILE*` pointer:

```cpp
class FsFile {
  FILE* _fp;   // Standard C file pointer (works on Emscripten's MEMFS)
public:
  bool open(const char* path, int flags) {
    _fp = fopen(path, flagsToMode(flags));
    return _fp != nullptr;
  }
  size_t read(void* buf, size_t n) {
    return fread(buf, 1, n, _fp);
  }
  bool seek(uint32_t pos) {
    return fseek(_fp, pos, SEEK_SET) == 0;
  }
  uint32_t fileSize() {
    long cur = ftell(_fp);
    fseek(_fp, 0, SEEK_END);
    long size = ftell(_fp);
    fseek(_fp, cur, SEEK_SET);
    return size;
  }
  void close() { if (_fp) fclose(_fp); _fp = nullptr; }
};
```

### How do EPUB files get into the virtual filesystem?

When the user drags an EPUB file onto the webpage, JavaScript writes it into Emscripten's MEMFS:

```
User drops "book.epub" onto the webpage
    ↓
JavaScript reads the file bytes
    ↓
JavaScript calls: Module.FS.writeFile("/books/book.epub", bytes)
    ↓
Now the firmware can open "/books/book.epub" as if it were on an SD card
```

### Optional: Persistence with IDBFS

MEMFS is lost when you close the tab. Emscripten also supports **IDBFS** (IndexedDB File System) which saves files in the browser's IndexedDB storage. This would let books and settings survive across page reloads.

---

## Domain 4: Threading Shim

### What problem does this solve?

The firmware uses **FreeRTOS** (a real-time operating system for microcontrollers) to run background tasks. Browsers don't have FreeRTOS. We need to fake it.

### Why does the firmware use threads?

On the real device, there are two things happening at once:
1. **Main loop**: polls buttons, handles user input
2. **Display task**: renders the screen (slow on e-ink, ~500ms-2s)

The firmware runs these on separate FreeRTOS threads so button presses are responsive even while the screen is updating.

### How pervasive is this?

**24 out of 29 activities** use the exact same pattern:

```cpp
// In onEnter() -- create background task:
xTaskCreate(&MyActivity::displayTaskLoop, "Task", 4096, this, 1, &taskHandle);

// displayTaskLoop() -- runs forever in background:
void displayTaskLoop() {
  while (true) {
    mutex.lock();
    renderScreen();      // Draw to framebuffer
    displayBuffer();     // Push to e-ink
    mutex.unlock();
    delay(100);          // Wait 100ms, then check again
  }
}

// loop() -- main thread, called every frame:
void loop() {
  if (buttonPressed) {
    mutex.lock();
    updateState();       // Change what needs to be drawn
    mutex.unlock();
  }
}
```

### The solution: Emscripten ASYNCIFY

**ASYNCIFY** is an Emscripten feature that makes blocking calls (like `delay()`) work in the browser. Normally, you can't call `delay(100)` in a browser because JavaScript is single-threaded -- the page would freeze. ASYNCIFY solves this by:

1. When `delay(100)` is called, it **saves the entire call stack**
2. Yields control back to the browser (so the page stays responsive)
3. After 100ms, **restores the call stack** and continues where it left off

This means the firmware's `while(true) { ... delay(100); }` loop works correctly -- each `delay()` gives the browser a chance to process input, render the canvas, etc.

### Our FreeRTOS shim

We provide fake versions of the FreeRTOS functions:

```cpp
// xTaskCreate → schedule the function to run via Emscripten
xTaskCreate(func, name, stack, param, priority, &handle)
  → emscripten_async_call(func, param, 0);  // "call func soon"

// xSemaphoreTake → in single-threaded mode, just track lock state
xSemaphoreTake(mutex, timeout)
  → mutex.locked = true;  // No contention in single-threaded mode

// delay(ms) → yield to browser
delay(100)
  → emscripten_sleep(100);  // ASYNCIFY saves stack, waits, resumes
```

Because all 24 activities use the identical pattern, this one shim file handles everything.

---

## Domain 5: Display HAL

### What problem does this solve?

The firmware renders to a 48,000-byte framebuffer (480x800 pixels, 1 bit per pixel). On real hardware, this buffer is sent to the e-ink controller via SPI. We need to display it on an HTML Canvas instead.

### How the framebuffer works

Each byte stores 8 pixels (1 = white, 0 = black):

```
Byte 0:  [1][1][0][1][0][0][1][1]  = pixels 0-7
Byte 1:  [0][1][1][1][1][0][0][1]  = pixels 8-15
...
Byte 47999: last 8 pixels of the display
```

### Our HalDisplay_Web

```
Firmware calls          Our implementation           Result
───────────────         ──────────────────           ──────
clearScreen(0xFF)   →   memset(buffer, 0xFF)     →  Buffer is all white
drawImage(...)      →   copy into buffer         →  Image in buffer
displayBuffer()     →   call JavaScript          →  Canvas shows the image
```

The key function is `displayBuffer()`. It calls into JavaScript:

```
C++ side:                           JavaScript side:
displayBuffer(FAST_REFRESH)    →    renderFramebuffer(pointer, size, mode)
                                      ↓
                                    Read 48,000 bytes from WASM memory
                                      ↓
                                    Convert each bit to a pixel:
                                      1 → light gray (e-ink white: #F5F5F0)
                                      0 → dark (e-ink black: #0F0F0F)
                                      ↓
                                    Put on Canvas via ctx.putImageData()
```

### E-ink visual effects (optional polish)

To make it feel like a real e-ink display:
- **FULL_REFRESH**: Flash the canvas black for 150ms, then show the image (simulates the real e-ink full refresh)
- **FAST_REFRESH**: Just show the image immediately
- **CSS filter**: Apply slight warmth/contrast to mimic e-ink paper tone

### Grayscale mode

The firmware also supports 2-bit grayscale (4 levels) for cover images. We handle this with two buffers (LSB + MSB) that combine to give values 0-3, mapped to 4 shades of gray.

---

## Domain 6: Input HAL

### What problem does this solve?

The real device has 7 physical buttons. The browser has a keyboard and mouse. We need to translate keyboard presses into fake button presses.

### The X4's button layout

```
Physical device:
  Top:    [Prev Page]  [Power]  [Next Page]
  Bottom: [Back]  [OK]  [◄]  [►]
```

### Our keyboard mapping

| Keyboard Key | Virtual Button | Index |
|-------------|----------------|-------|
| `Escape` | Back | 0 |
| `Enter` | OK / Confirm | 1 |
| `Arrow Left` | Left | 2 |
| `Arrow Right` | Right | 3 |
| `Arrow Up` / `Page Up` | Prev Page (side) | 4 |
| `Arrow Down` / `Page Down` | Next Page (side) | 5 |
| `P` | Power | 6 |

### How it works

```
1. User presses Enter on keyboard
        ↓
2. JavaScript keydown event fires
        ↓
3. JS calls into WASM: Module._setButtonState(1, true)
        ↓
4. Our HalGPIO_Web stores: buttonState[1] = true
        ↓
5. Firmware calls gpio.update() in main loop
        ↓
6. Our update() computes: wasPressed[1] = true (edge detection)
        ↓
7. Firmware calls gpio.wasPressed(BTN_CONFIRM) → returns true
        ↓
8. Activity handles the "OK" press as if a real button was pushed
```

### Additional stubs

| Firmware call | Our response | Why |
|---------------|-------------|-----|
| `getBatteryPercentage()` | Return `100` | Always "full battery" |
| `isUsbConnected()` | Return `false` | Not relevant in browser |
| `startDeepSleep()` | Show overlay "Device sleeping" | Can't really sleep |
| `getWakeupReason()` | Return `AfterFlash` | Simulates fresh boot |

---

## Domain 7: Web Shell

### What problem does this solve?

We need a webpage that hosts the emulator. This is the only part written in HTML/CSS/JavaScript (everything else is C++ compiled to WASM).

### What's on the page?

```
┌──────────────────────────────────────────────────────────┐
│                                                           │
│   ┌────────────────────┐    ┌──────────────────────────┐ │
│   │                    │    │  CrossPoint Emulator      │ │
│   │   ┌────────────┐   │    │                          │ │
│   │   │            │   │    │  ┌────────────────────┐  │ │
│   │   │  Canvas    │   │    │  │                    │  │ │
│   │   │  480x800   │   │    │  │  Drop EPUB files   │  │ │
│   │   │  (the      │   │    │  │  here to load      │  │ │
│   │   │  "screen") │   │    │  │                    │  │ │
│   │   │            │   │    │  └────────────────────┘  │ │
│   │   │            │   │    │                          │ │
│   │   └────────────┘   │    │  Keyboard Shortcuts:     │ │
│   │                    │    │  Esc = Back              │ │
│   │  [Bk] [OK] [◄] [►]│    │  Enter = OK              │ │
│   │  [PgUp] [PgDn] [P] │    │  Arrows = Navigate       │ │
│   │                    │    │  PgUp/PgDn = Page turn   │ │
│   └────────────────────┘    │  P = Power               │ │
│      Device frame            └──────────────────────────┘ │
│                                  Control panel            │
└──────────────────────────────────────────────────────────┘
```

### The 3 files

**`index.html`** -- The page structure:
- A `<canvas>` element (480x800) for the e-ink display
- Virtual button elements users can click
- A drag-and-drop zone for loading books
- A panel showing keyboard shortcuts
- Loads the WASM module

**`style.css`** -- Visual styling:
- Dark background (the emulator "desk")
- Device frame styling (rounded corners, shadow)
- E-ink appearance filter on the canvas (slight warmth, contrast reduction)
- Button styling

**`shell.js`** -- The JavaScript bridge:
- `renderFramebuffer()` -- converts the 48KB buffer to Canvas pixels
- `renderGrayscale()` -- handles 2-bit grayscale mode
- Keyboard event listeners → `setButtonState()` calls into WASM
- Virtual button click handlers → `setButtonState()` calls into WASM
- Drag-and-drop handler → writes files into Emscripten's virtual filesystem

---

## Build & Run Flow

Once all 7 domains are implemented, here's how it works end-to-end:

```
Step 1: BUILD
  emcmake cmake emulator/
  emmake make
    ↓
  Emscripten compiles:
    - Firmware src/*.cpp (unchanged)
    - Firmware lib/*.cpp (unchanged)
    - emulator/shims/*.cpp (our fakes)
    - emulator/hal_web/*.cpp (our HAL)
    ↓
  Produces: crosspoint-emulator.js + crosspoint-emulator.wasm

Step 2: SERVE
  python3 -m http.server 8080
    ↓
  Open http://localhost:8080

Step 3: RUN
  Browser loads index.html
    → loads shell.js (bridge code)
    → loads crosspoint-emulator.js (Emscripten runtime)
    → loads crosspoint-emulator.wasm (compiled firmware)
    ↓
  WASM module starts:
    → calls setup() (same as real device boot)
    → initializes display, fonts, storage
    → enters BootActivity → shows splash screen on Canvas
    → transitions to HomeActivity → shows home menu
    ↓
  User interacts:
    → presses keyboard keys → virtual buttons
    → drops EPUB file → loaded into virtual filesystem
    → navigates menus, opens book, reads pages
    → everything renders on the Canvas exactly as on real device
```

---

## Implementation Order

| Order | Domain | Milestone | Dependencies |
|:-----:|--------|-----------|-------------|
| 1 | Build System | `emcc` compiles without errors | None |
| 2 | Arduino Shim | `String`, `Serial`, `millis` work | Domain 1 |
| 3 | Filesystem Shim | `FsFile` can open/read/write files | Domain 1 |
| 4 | Threading Shim | FreeRTOS tasks run cooperatively | Domain 2 (needs `delay()`) |
| 5 | Display HAL | Splash screen appears on Canvas | Domain 1, 2 |
| 6 | Input HAL | Can navigate menus with keyboard | Domain 2 |
| 7 | Web Shell | Complete emulator page with buttons & file loading | Domain 5, 6 |

Domains 2, 3, and 4 are the "shim layer" -- they make the firmware think it's running on real hardware. Domains 5 and 6 are the "HAL replacements" -- they connect to browser APIs instead of hardware. Domain 7 is the user-facing webpage.

---

## Summary: Nothing in the firmware changes

```
FIRMWARE (existing code, ZERO changes)
├── src/main.cpp           ← compiles as-is
├── src/activities/*.cpp   ← compile as-is
├── lib/GfxRenderer/*.cpp  ← compiles as-is
├── lib/Epub/*.cpp         ← compiles as-is
├── lib/hal/HalDisplay.h   ← header used, .cpp REPLACED
├── lib/hal/HalGPIO.h      ← header used, .cpp REPLACED
└── lib/hal/HalStorage.h   ← header used, .cpp REPLACED

EMULATOR (all new code, lives in emulator/ directory)
├── shims/Arduino.h        ← Domain 2: Fake Arduino
├── shims/SdFat.h          ← Domain 3: Fake filesystem
├── shims/freertos/*.h     ← Domain 4: Fake threading
├── hal_web/HalDisplay_Web ← Domain 5: Canvas rendering
├── hal_web/HalGPIO_Web    ← Domain 6: Keyboard input
├── hal_web/HalStorage_Web ← Domain 3: Browser filesystem
├── web/index.html         ← Domain 7: The webpage
├── web/shell.js           ← Domain 7: JS bridge
└── CMakeLists.txt         ← Domain 1: Build recipe
```

The firmware code goes in one side, the emulator code goes in the other, Emscripten stitches them together, and out comes a `.wasm` file that runs in any browser.
