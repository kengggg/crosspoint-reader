#!/usr/bin/env bash
# Build script for CrossPoint Reader Web Simulator.
#
# Prerequisites:
#   - Emscripten SDK (emsdk) installed and activated
#     Install: https://emscripten.org/docs/getting_started/downloads.html
#     $ git clone https://github.com/emscripten-core/emsdk.git
#     $ cd emsdk && ./emsdk install latest && ./emsdk activate latest
#     $ source ./emsdk_env.sh
#
# Usage:
#   ./build.sh          # Build the simulator
#   ./build.sh clean    # Clean build directory
#   ./build.sh serve    # Build and start a local web server
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }

# Check for Emscripten — try to auto-detect if not already in PATH
check_emscripten() {
  if ! command -v emcmake &> /dev/null; then
    # Try common emsdk locations relative to this repo
    local search_paths=(
      "${SCRIPT_DIR}/../../emsdk"
      "${SCRIPT_DIR}/../emsdk"
      "${HOME}/emsdk"
      "/opt/emsdk"
    )
    for p in "${search_paths[@]}"; do
      if [ -f "$p/emsdk_env.sh" ]; then
        warn "emcmake not in PATH, sourcing $p/emsdk_env.sh"
        source "$p/emsdk_env.sh" 2>/dev/null
        break
      fi
    done
  fi

  if ! command -v emcmake &> /dev/null; then
    error "Emscripten not found! Please install and activate emsdk first.

  Install instructions:
    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk
    ./emsdk install latest
    ./emsdk activate latest
    source ./emsdk_env.sh

  Then either:
    a) Run 'source <path-to-emsdk>/emsdk_env.sh' before ./build.sh
    b) Place emsdk alongside the crosspoint-reader repo (../emsdk/)"
  fi
  info "Emscripten found: $(emcc --version | head -1)"
}

# Clean build
clean() {
  info "Cleaning build directory..."
  rm -rf "${BUILD_DIR}"
  info "Done."
}

# Build
build() {
  check_emscripten

  info "Configuring with CMake (Emscripten toolchain)..."
  mkdir -p "${BUILD_DIR}"
  cd "${BUILD_DIR}"

  emcmake cmake "${SCRIPT_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    2>&1 | tail -5

  info "Building..."
  emmake make -j$(nproc 2>/dev/null || echo 4) 2>&1

  info "Build complete!"
  info ""
  info "Output files in: ${BUILD_DIR}/"
  info "  index.html       — Main HTML page"
  info "  crosspoint.js    — Emscripten glue code"
  info "  crosspoint.wasm  — WebAssembly binary"
  info "  shell.js         — Simulator UI logic"
  info "  style.css        — Simulator styles"
}

# Serve locally
serve() {
  build

  info ""
  info "Starting local web server at http://localhost:8080"
  info "Press Ctrl+C to stop."
  info ""

  cd "${BUILD_DIR}"
  if command -v python3 &> /dev/null; then
    python3 -m http.server 8080
  elif command -v python &> /dev/null; then
    python -m SimpleHTTPServer 8080
  else
    error "Python not found. Please open ${BUILD_DIR}/crosspoint.html in a web server."
  fi
}

# Main
case "${1:-build}" in
  clean)
    clean
    ;;
  serve)
    serve
    ;;
  build|*)
    build
    ;;
esac
