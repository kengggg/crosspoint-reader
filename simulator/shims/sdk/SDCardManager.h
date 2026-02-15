#pragma once
// SDCardManager stub for Emscripten web simulator.
// The actual filesystem operations are implemented in HalStorage_Web.cpp.
// This stub provides the class/types needed by HalStorage.h.
// We also define FsFile and related types here since the firmware expects them
// from the SdFat library via SDCardManager.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "WString.h"
#include "Print.h"
#include "Stream.h"

// File open flags (matching SdFat)
typedef uint8_t oflag_t;
#define O_RDONLY 0x00
#define O_WRONLY 0x01
#define O_RDWR 0x02
#define O_APPEND 0x04
#define O_CREAT 0x08
#define O_TRUNC 0x10
#define O_AT_END 0x20
#define O_EXCL 0x40
#define FILE_WRITE (O_RDWR | O_CREAT | O_AT_END)
#define FILE_READ O_RDONLY

// Seek positions
#define SeekSet 0
#define SeekCur 1
#define SeekEnd 2

// FsFile implementation backed by C stdio.
// In the real SdFat library, FsFile inherits from Stream (which inherits from Print).
// This is needed because firmware code passes FsFile to functions expecting Print&/Stream&.
class FsFile : public Stream {
 public:
  FsFile() = default;
  ~FsFile() override { close(); }

  // Move semantics
  FsFile(FsFile&& other) noexcept : fp_(other.fp_), isDir_(other.isDir_) {
    strncpy(name_, other.name_, sizeof(name_));
    strncpy(path_, other.path_, sizeof(path_));
    other.fp_ = nullptr;
    other.isDir_ = false;
  }
  FsFile& operator=(FsFile&& other) noexcept {
    if (this != &other) {
      close();
      fp_ = other.fp_;
      isDir_ = other.isDir_;
      strncpy(name_, other.name_, sizeof(name_));
      strncpy(path_, other.path_, sizeof(path_));
      other.fp_ = nullptr;
      other.isDir_ = false;
    }
    return *this;
  }

  // No copy
  FsFile(const FsFile&) = delete;
  FsFile& operator=(const FsFile&) = delete;

  operator bool() const { return fp_ != nullptr || isDir_; }

  bool open(const char* path, oflag_t flags = O_RDONLY);
  bool openNext(FsFile& dirFile, oflag_t flags = O_RDONLY);

  void close() {
    if (fp_) {
      fclose(fp_);
      fp_ = nullptr;
    }
    isDir_ = false;
  }

  // Stream interface overrides
  int available() override {
    if (!fp_) return 0;
    long cur = ftell(fp_);
    fseek(fp_, 0, SEEK_END);
    long end = ftell(fp_);
    fseek(fp_, cur, SEEK_SET);
    return static_cast<int>(end - cur);
  }
  int read() override {
    if (!fp_) return -1;
    return fgetc(fp_);
  }
  int peek() override {
    if (!fp_) return -1;
    int c = fgetc(fp_);
    if (c != EOF) ungetc(c, fp_);
    return c;
  }

  // Print interface override
  size_t write(uint8_t c) override {
    if (!fp_) return 0;
    return fwrite(&c, 1, 1, fp_);
  }

  // Additional read/write overloads used by firmware
  int read(void* buf, size_t count) {
    if (!fp_) return -1;
    return static_cast<int>(fread(buf, 1, count, fp_));
  }

  size_t write(const void* buf, size_t count) {
    if (!fp_) return 0;
    return fwrite(buf, 1, count, fp_);
  }

  bool seek(uint64_t pos) {
    if (!fp_) return false;
    return fseek(fp_, static_cast<long>(pos), SEEK_SET) == 0;
  }
  bool seekSet(uint64_t pos) { return seek(pos); }
  bool seekCur(int64_t offset) {
    if (!fp_) return false;
    return fseek(fp_, static_cast<long>(offset), SEEK_CUR) == 0;
  }
  bool seekEnd(int64_t offset = 0) {
    if (!fp_) return false;
    return fseek(fp_, static_cast<long>(offset), SEEK_END) == 0;
  }

  uint64_t curPosition() const {
    if (!fp_) return 0;
    return static_cast<uint64_t>(ftell(fp_));
  }
  uint64_t position() const { return curPosition(); }

  uint64_t fileSize() const {
    if (!fp_) return 0;
    long cur = ftell(fp_);
    fseek(fp_, 0, SEEK_END);
    long sz = ftell(fp_);
    fseek(fp_, cur, SEEK_SET);
    return static_cast<uint64_t>(sz);
  }
  uint64_t size() const { return fileSize(); }

  bool isDirectory() const { return isDir_; }
  bool isDir() const { return isDir_; }
  bool isOpen() const { return fp_ != nullptr || isDir_; }

  // Get name (filename only, not full path)
  void getName(char* buf, size_t size) const { strncpy(buf, name_, size); }
  const char* name() const { return name_; }

  void flush() {
    if (fp_) fflush(fp_);
  }

  // Arduino-style directory iteration (used by MyLibraryActivity, SleepActivity, etc.)
  FsFile openNextFile();
  void rewindDirectory();

  // Internal helpers for simulator
  void setPath(const char* path);
  void setIsDir(bool dir) { isDir_ = dir; }
  FILE* getFp() const { return fp_; }
  void setFp(FILE* fp) { fp_ = fp; }
  const char* getPath() const { return path_; }

 private:
  FILE* fp_ = nullptr;
  bool isDir_ = false;
  char name_[256] = {};
  char path_[512] = {};
};

// SDCardManager - delegates to the simulator filesystem.
// HalStorage_Web.cpp provides the real implementations.
class SDCardManager {
 public:
  static SDCardManager& getInstance() {
    static SDCardManager instance;
    return instance;
  }

  bool begin();
  bool ready() const { return initialized_; }
  std::vector<String> listFiles(const char* path = "/", int maxFiles = 200);
  String readFile(const char* path);
  bool readFileToStream(const char* path, Print& out, size_t chunkSize = 256);
  size_t readFileToBuffer(const char* path, char* buffer, size_t bufferSize, size_t maxBytes = 0);
  bool writeFile(const char* path, const String& content);
  bool ensureDirectoryExists(const char* path);
  FsFile open(const char* path, oflag_t oflag = O_RDONLY);
  bool mkdir(const char* path, bool pFlag = true);
  bool exists(const char* path);
  bool remove(const char* path);
  bool rmdir(const char* path);
  bool openFileForRead(const char* moduleName, const char* path, FsFile& file);
  bool openFileForWrite(const char* moduleName, const char* path, FsFile& file);
  bool removeDir(const char* path);

 private:
  SDCardManager() = default;
  bool initialized_ = false;
};

#define SdMan SDCardManager::getInstance()
