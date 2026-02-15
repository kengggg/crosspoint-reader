// CrossPoint Reader Web Simulator — Shell JavaScript
// Handles display rendering, keyboard input, and file upload.

'use strict';

// --- Display Rendering ---

const canvas = document.getElementById('display');
const ctx = canvas.getContext('2d');
const statusText = document.getElementById('status-text');

// Render 1-bit framebuffer to canvas.
// The framebuffer is in physical panel orientation (800x480 landscape).
// We rotate 90° CCW to display as portrait (480x800) on the canvas,
// matching how the real X4 panel is physically mounted.
Module.renderFramebuffer = function(bufPtr, width, height, mode) {
  const widthBytes = width / 8;
  const bufSize = widthBytes * height;
  const buf = Module.HEAPU8.subarray(bufPtr, bufPtr + bufSize);

  // Canvas is portrait: 480 wide x 800 tall
  const canvasW = canvas.width;   // 480
  const canvasH = canvas.height;  // 800
  const imageData = ctx.createImageData(canvasW, canvasH);
  const pixels = imageData.data;

  // 90° CCW rotation: canvas(cx, cy) = buffer(cy, width-1-cx)
  for (let bufY = 0; bufY < height; bufY++) {
    for (let xByte = 0; xByte < widthBytes; xByte++) {
      const byte = buf[bufY * widthBytes + xByte];
      for (let bit = 0; bit < 8; bit++) {
        const bufX = xByte * 8 + bit;
        // Rotate 90° CCW: bufX,bufY → canvasX=bufY, canvasY=width-1-bufX
        const cx = bufY;
        const cy = (width - 1) - bufX;
        const idx = (cy * canvasW + cx) * 4;
        // E-ink: 1 = white, 0 = black (MSB first)
        const isWhite = (byte >> (7 - bit)) & 1;
        const gray = isWhite ? 245 : 10;
        pixels[idx + 0] = gray;
        pixels[idx + 1] = gray;
        pixels[idx + 2] = gray;
        pixels[idx + 3] = 255;
      }
    }
  }

  ctx.putImageData(imageData, 0, 0);

  // Flash effect for full refresh
  if (mode === 0) { // FULL_REFRESH
    canvas.style.filter = 'invert(1)';
    setTimeout(() => { canvas.style.filter = ''; }, 80);
  }
};

// Render 2-bit grayscale framebuffer (LSB + MSB planes)
// Same 90° CCW rotation as renderFramebuffer.
Module.renderGrayscale = function(lsbPtr, msbPtr, width, height) {
  const bufSize = (width / 8) * height;
  const lsb = Module.HEAPU8.subarray(lsbPtr, lsbPtr + bufSize);
  const msb = Module.HEAPU8.subarray(msbPtr, msbPtr + bufSize);

  const canvasW = canvas.width;
  const canvasH = canvas.height;
  const imageData = ctx.createImageData(canvasW, canvasH);
  const pixels = imageData.data;
  const widthBytes = width / 8;

  for (let bufY = 0; bufY < height; bufY++) {
    for (let xByte = 0; xByte < widthBytes; xByte++) {
      const lsbByte = lsb[bufY * widthBytes + xByte];
      const msbByte = msb[bufY * widthBytes + xByte];
      for (let bit = 0; bit < 8; bit++) {
        const bufX = xByte * 8 + bit;
        const cx = bufY;
        const cy = (width - 1) - bufX;
        const idx = (cy * canvasW + cx) * 4;
        const lsbBit = (lsbByte >> (7 - bit)) & 1;
        const msbBit = (msbByte >> (7 - bit)) & 1;
        const level = (msbBit << 1) | lsbBit;
        const grayValues = [10, 85, 170, 245];
        const gray = grayValues[level];
        pixels[idx + 0] = gray;
        pixels[idx + 1] = gray;
        pixels[idx + 2] = gray;
        pixels[idx + 3] = 255;
      }
    }
  }

  ctx.putImageData(imageData, 0, 0);
};

// Deep sleep handler
Module.onDeepSleep = function() {
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  statusText.textContent = 'Device sleeping (press any key to wake)';
};

// --- Keyboard Input ---
// Maps keyboard keys to X4 button indices:
// BTN_BACK=0, BTN_CONFIRM=1, BTN_LEFT=2, BTN_RIGHT=3, BTN_UP=4, BTN_DOWN=5, BTN_POWER=6

const KEY_MAP = {
  'Escape': 0,     // Back
  'Backspace': 0,   // Back (alternate)
  'Enter': 1,       // Confirm
  ' ': 1,           // Confirm (alternate)
  'ArrowLeft': 2,   // Left
  'ArrowRight': 3,  // Right
  'ArrowUp': 4,     // Up / Page Back
  'ArrowDown': 5,   // Down / Page Forward
  'p': 6,           // Power
  'P': 6,           // Power
};

document.addEventListener('keydown', function(e) {
  const btn = KEY_MAP[e.key];
  if (btn !== undefined) {
    e.preventDefault();
    Module._simulator_button_down(btn);
    // Highlight button in UI
    const btnEl = document.querySelector(`.hw-btn[data-btn="${btn}"]`);
    if (btnEl) btnEl.classList.add('pressed');
  }
});

document.addEventListener('keyup', function(e) {
  const btn = KEY_MAP[e.key];
  if (btn !== undefined) {
    e.preventDefault();
    Module._simulator_button_up(btn);
    const btnEl = document.querySelector(`.hw-btn[data-btn="${btn}"]`);
    if (btnEl) btnEl.classList.remove('pressed');
  }
});

// Mouse/touch support for on-screen buttons
document.querySelectorAll('.hw-btn').forEach(btn => {
  const idx = parseInt(btn.dataset.btn);

  btn.addEventListener('mousedown', (e) => {
    e.preventDefault();
    Module._simulator_button_down(idx);
    btn.classList.add('pressed');
  });
  btn.addEventListener('mouseup', (e) => {
    e.preventDefault();
    Module._simulator_button_up(idx);
    btn.classList.remove('pressed');
  });
  btn.addEventListener('mouseleave', () => {
    Module._simulator_button_up(idx);
    btn.classList.remove('pressed');
  });

  // Touch support
  btn.addEventListener('touchstart', (e) => {
    e.preventDefault();
    Module._simulator_button_down(idx);
    btn.classList.add('pressed');
  });
  btn.addEventListener('touchend', (e) => {
    e.preventDefault();
    Module._simulator_button_up(idx);
    btn.classList.remove('pressed');
  });
});

// --- File Upload ---
// Writes uploaded EPUB files into the MEMFS virtual filesystem

document.getElementById('epub-file').addEventListener('change', function(e) {
  const file = e.target.files[0];
  if (!file) return;

  statusText.textContent = `Uploading: ${file.name}...`;
  const reader = new FileReader();

  reader.onload = function(event) {
    const data = new Uint8Array(event.target.result);
    const path = '/sd/books/' + file.name;

    // Ensure directory exists
    try { FS.mkdir('/sd'); } catch(e) {}
    try { FS.mkdir('/sd/books'); } catch(e) {}

    // Write file to MEMFS
    FS.writeFile(path, data);
    statusText.textContent = `Uploaded: ${file.name} (${(data.length / 1024).toFixed(1)} KB) — navigate to My Library to open it`;
  };

  reader.onerror = function() {
    statusText.textContent = 'Upload failed!';
  };

  reader.readAsArrayBuffer(file);
});

// --- Init status ---
Module.onRuntimeInitialized = function() {
  statusText.textContent = 'Simulator running';
};
