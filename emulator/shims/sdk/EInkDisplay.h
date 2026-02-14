#pragma once
// EInkDisplay stub for Emscripten web emulator.
// Provides the interface expected by HalDisplay.h.
// The actual rendering is done in HalDisplay_Web.cpp via the framebuffer.

#include <cstdint>
#include <cstring>

class EInkDisplay {
 public:
  // Display dimensions matching XTeink X4 hardware
  static constexpr uint16_t DISPLAY_WIDTH = 480;
  static constexpr uint16_t DISPLAY_HEIGHT = 800;
  static constexpr uint16_t DISPLAY_WIDTH_BYTES = DISPLAY_WIDTH / 8;
  static constexpr uint32_t BUFFER_SIZE = DISPLAY_WIDTH_BYTES * DISPLAY_HEIGHT;

  enum RefreshMode { FULL_REFRESH, HALF_REFRESH, FAST_REFRESH };

  // Constructor accepts pin numbers (ignored in emulator)
  EInkDisplay(int sclk, int mosi, int cs, int dc, int rst, int busy)
      : frameBuffer_(new uint8_t[BUFFER_SIZE]) {
    (void)sclk;
    (void)mosi;
    (void)cs;
    (void)dc;
    (void)rst;
    (void)busy;
    memset(frameBuffer_, 0xFF, BUFFER_SIZE);  // White screen
  }

  ~EInkDisplay() { delete[] frameBuffer_; }

  void begin() {}

  void clearScreen(uint8_t color = 0xFF) const { memset(frameBuffer_, color, BUFFER_SIZE); }

  void drawImage(const uint8_t* imageData, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                 bool fromProgmem = false) const {
    (void)fromProgmem;
    uint16_t wBytes = w / 8;
    for (uint16_t row = 0; row < h && (y + row) < DISPLAY_HEIGHT; row++) {
      for (uint16_t col = 0; col < wBytes && (x / 8 + col) < DISPLAY_WIDTH_BYTES; col++) {
        frameBuffer_[(y + row) * DISPLAY_WIDTH_BYTES + x / 8 + col] = imageData[row * wBytes + col];
      }
    }
  }

  // displayBuffer and refreshDisplay trigger rendering to the canvas (via JS callback)
  void displayBuffer(RefreshMode mode = FAST_REFRESH, bool turnOffScreen = false);
  void refreshDisplay(RefreshMode mode = FAST_REFRESH, bool turnOffScreen = false);

  void deepSleep() {}

  uint8_t* getFrameBuffer() const { return frameBuffer_; }

  // Grayscale stubs - copy into internal buffers for later display
  void copyGrayscaleBuffers(const uint8_t* lsbBuffer, const uint8_t* msbBuffer) {
    if (lsbBuffer) memcpy(grayscaleLsb_, lsbBuffer, BUFFER_SIZE);
    if (msbBuffer) memcpy(grayscaleMsb_, msbBuffer, BUFFER_SIZE);
  }
  void copyGrayscaleLsbBuffers(const uint8_t* lsbBuffer) {
    if (lsbBuffer) memcpy(grayscaleLsb_, lsbBuffer, BUFFER_SIZE);
  }
  void copyGrayscaleMsbBuffers(const uint8_t* msbBuffer) {
    if (msbBuffer) memcpy(grayscaleMsb_, msbBuffer, BUFFER_SIZE);
  }
  void cleanupGrayscaleBuffers(const uint8_t* bwBuffer) {
    if (bwBuffer) memcpy(frameBuffer_, bwBuffer, BUFFER_SIZE);
  }
  void displayGrayBuffer(bool turnOffScreen = false);

 private:
  uint8_t* frameBuffer_;
  uint8_t grayscaleLsb_[BUFFER_SIZE] = {};
  uint8_t grayscaleMsb_[BUFFER_SIZE] = {};
};
