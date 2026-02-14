// HalDisplay implementation for Emscripten web emulator.
// This file replaces lib/hal/HalDisplay.cpp.
// All methods delegate to the EInkDisplay stub, which manages the framebuffer.
// Display updates trigger a JS callback to render the framebuffer to <canvas>.

#include <HalDisplay.h>
#include <emscripten.h>

HalDisplay::HalDisplay() : einkDisplay(0, 0, 0, 0, 0, 0) {}

HalDisplay::~HalDisplay() {}

void HalDisplay::begin() { einkDisplay.begin(); }

void HalDisplay::clearScreen(uint8_t color) const { einkDisplay.clearScreen(color); }

void HalDisplay::drawImage(const uint8_t* imageData, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                           bool fromProgmem) const {
  einkDisplay.drawImage(imageData, x, y, w, h, fromProgmem);
}

// Convert our refresh mode to EInkDisplay's
static EInkDisplay::RefreshMode convertRefreshMode(HalDisplay::RefreshMode mode) {
  switch (mode) {
    case HalDisplay::FULL_REFRESH:
      return EInkDisplay::FULL_REFRESH;
    case HalDisplay::HALF_REFRESH:
      return EInkDisplay::HALF_REFRESH;
    case HalDisplay::FAST_REFRESH:
    default:
      return EInkDisplay::FAST_REFRESH;
  }
}

void HalDisplay::displayBuffer(HalDisplay::RefreshMode mode, bool turnOffScreen) {
  einkDisplay.displayBuffer(convertRefreshMode(mode), turnOffScreen);
}

void HalDisplay::refreshDisplay(HalDisplay::RefreshMode mode, bool turnOffScreen) {
  einkDisplay.refreshDisplay(convertRefreshMode(mode), turnOffScreen);
}

void HalDisplay::deepSleep() { einkDisplay.deepSleep(); }

uint8_t* HalDisplay::getFrameBuffer() const { return einkDisplay.getFrameBuffer(); }

void HalDisplay::copyGrayscaleBuffers(const uint8_t* lsbBuffer, const uint8_t* msbBuffer) {
  einkDisplay.copyGrayscaleBuffers(lsbBuffer, msbBuffer);
}
void HalDisplay::copyGrayscaleLsbBuffers(const uint8_t* lsbBuffer) {
  einkDisplay.copyGrayscaleLsbBuffers(lsbBuffer);
}
void HalDisplay::copyGrayscaleMsbBuffers(const uint8_t* msbBuffer) {
  einkDisplay.copyGrayscaleMsbBuffers(msbBuffer);
}
void HalDisplay::cleanupGrayscaleBuffers(const uint8_t* bwBuffer) {
  einkDisplay.cleanupGrayscaleBuffers(bwBuffer);
}
void HalDisplay::displayGrayBuffer(bool turnOffScreen) {
  einkDisplay.displayGrayBuffer(turnOffScreen);
}

// --- EInkDisplay display methods: push framebuffer to JS canvas ---

// JS function defined in shell.js that renders the framebuffer to <canvas>
EM_JS(void, js_render_framebuffer, (const uint8_t* buf, int width, int height, int mode), {
  if (typeof Module.renderFramebuffer === 'function') {
    Module.renderFramebuffer(buf, width, height, mode);
  }
});

EM_JS(void, js_render_grayscale, (const uint8_t* lsb, const uint8_t* msb, int width, int height), {
  if (typeof Module.renderGrayscale === 'function') {
    Module.renderGrayscale(lsb, msb, width, height);
  }
});

void EInkDisplay::displayBuffer(RefreshMode mode, bool turnOffScreen) {
  (void)turnOffScreen;
  js_render_framebuffer(frameBuffer_, DISPLAY_WIDTH, DISPLAY_HEIGHT, static_cast<int>(mode));
}

void EInkDisplay::refreshDisplay(RefreshMode mode, bool turnOffScreen) {
  (void)turnOffScreen;
  js_render_framebuffer(frameBuffer_, DISPLAY_WIDTH, DISPLAY_HEIGHT, static_cast<int>(mode));
}

void EInkDisplay::displayGrayBuffer(bool turnOffScreen) {
  (void)turnOffScreen;
  js_render_grayscale(grayscaleLsb_, grayscaleMsb_, DISPLAY_WIDTH, DISPLAY_HEIGHT);
}
