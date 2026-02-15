# CrossPoint Reader — Web Simulator

A browser-based simulator for the CrossPoint Reader firmware, compiled to WebAssembly via Emscripten.

## How It Works

The simulator compiles the **exact same firmware code** that runs on the XTeink X4 hardware to WebAssembly. Instead of talking to real hardware (e-ink display, buttons, SD card), the firmware talks to browser-based replacements:

| Firmware HAL | Hardware (X4)        | Simulator (Browser)          |
| ------------ | -------------------- | ---------------------------- |
| HalDisplay   | E-ink panel          | HTML `<canvas>` element      |
| HalGPIO      | Physical buttons     | Keyboard / on-screen buttons |
| HalStorage   | MicroSD card         | In-memory filesystem (MEMFS) |
| FreeRTOS     | Real-time OS threads | Emscripten fibers             |

## Prerequisites

1. **Emscripten SDK** (emsdk) — the compiler toolchain that turns C++ into WebAssembly.

```bash
# Install emsdk (one time)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest

# Activate for current shell (run every time you open a new terminal)
source ./emsdk_env.sh
```

2. **CMake** (version 3.13+) and **Make** — usually pre-installed on macOS/Linux.

```bash
# Ubuntu/Debian
sudo apt install cmake make

# macOS
brew install cmake
```

## Building

From the `simulator/` directory:

```bash
# Build the simulator
./build.sh

# Or build and start a local web server
./build.sh serve
```

This produces files in `simulator/build/`:
- `crosspoint.html` — Open this in your browser
- `crosspoint.js` — Emscripten runtime
- `crosspoint.wasm` — Compiled firmware

## Running

1. Start a local web server (required — browsers block file:// WASM loading):

```bash
cd simulator/build
python3 -m http.server 8080
```

2. Open `http://localhost:8080/crosspoint.html` in your browser.

3. **Upload an EPUB**: Use the file picker to load a `.epub` file into the simulator's virtual filesystem.

4. **Navigate**: Use keyboard keys or click the on-screen buttons.

## Keyboard Controls

| Key           | Button        | Function        |
| ------------- | ------------- | --------------- |
| `Escape`      | Back          | Go back         |
| `Enter`       | Confirm       | Select/confirm  |
| `Arrow Left`  | Left          | Previous item   |
| `Arrow Right` | Right         | Next item       |
| `Arrow Up`    | Up/PageBack   | Page back       |
| `Arrow Down`  | Down/PageFwd  | Page forward    |
| `P`           | Power         | Sleep/wake      |

## Architecture

```
simulator/
├── CMakeLists.txt          # Build system
├── build.sh                # Build helper script
├── main_simulator.cpp       # Entry point (replaces Arduino main)
├── hal/                    # HAL implementations for web
│   ├── HalDisplay_Web.cpp  # Canvas-based display
│   ├── HalGPIO_Web.cpp     # Keyboard-based input
│   └── HalStorage_Web.cpp  # MEMFS-based storage
├── shell/                  # Web UI
│   ├── index.html
│   ├── shell.js
│   └── style.css
└── shims/                  # Compatibility headers
    ├── arduino/            # Arduino.h, Serial, String, SPI, Print, Stream
    ├── esp/                # esp_sleep, esp_task_wdt, etc.
    ├── freertos/           # Cooperative task scheduling via fibers
    ├── network/            # WiFi, WebServer, HTTPClient stubs
    ├── sdk/                # EInkDisplay, InputManager, BatteryMonitor, SDCardManager
    └── thirdparty/         # ArduinoJson, QRCode, MD5Builder, base64
```

**Key design principle**: Zero firmware modifications. All simulator code lives in `simulator/`. The firmware source files compile unmodified — our shim headers are found first via include path ordering.

## Limitations

- **Network features** are not available (WiFi, OPDS browser, file transfer, OTA updates, KOReader sync)
- **Files are not persisted** — uploaded books are lost on page refresh (MEMFS is in-memory only)
- **Single-threaded** — FreeRTOS tasks run cooperatively, not in parallel
- **No grayscale dithering** in the display (2-bit grayscale is approximated)

## Troubleshooting

**Build fails with "emcmake not found"**: Run `source <path-to-emsdk>/emsdk_env.sh` first.

**Build fails with missing header**: A firmware include is not shimmed yet. Add a stub header in the appropriate `shims/` subdirectory.

**Page is blank**: Check the browser console (F12) for errors. The most common issue is CORS — you must use a local web server, not `file://`.

**Display doesn't update**: The simulator renders the framebuffer to canvas whenever `displayBuffer()` is called by the firmware. If nothing appears, the firmware may not have reached a render call yet.
