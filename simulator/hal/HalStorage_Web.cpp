// HalStorage implementation for Emscripten web simulator.
// This file replaces lib/hal/HalStorage.cpp.
// Uses Emscripten's MEMFS virtual filesystem for all file operations.

#include <HalStorage.h>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

HalStorage HalStorage::instance;

// --- FsFile implementation ---

// Directory iteration state (stored per-file for openNextFile)
#include <map>
static std::map<std::string, DIR*> g_dir_handles;

FsFile FsFile::openNextFile() {
  if (!isDir_) return FsFile();

  auto it = g_dir_handles.find(std::string(path_));
  if (it == g_dir_handles.end()) {
    DIR* d = opendir(path_);
    if (!d) return FsFile();
    g_dir_handles[std::string(path_)] = d;
    it = g_dir_handles.find(std::string(path_));
  }

  struct dirent* entry;
  while ((entry = readdir(it->second)) != nullptr) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", path_, entry->d_name);

    FsFile f;
    struct stat st;
    if (stat(fullPath, &st) == 0 && S_ISDIR(st.st_mode)) {
      f.setPath(fullPath);
      f.setIsDir(true);
    } else {
      f.open(fullPath, O_RDONLY);
    }
    return f;
  }

  // End of directory - close and remove handle
  closedir(it->second);
  g_dir_handles.erase(it);
  return FsFile();
}

void FsFile::rewindDirectory() {
  if (!isDir_) return;
  auto it = g_dir_handles.find(std::string(path_));
  if (it != g_dir_handles.end()) {
    closedir(it->second);
    g_dir_handles.erase(it);
  }
}

void FsFile::setPath(const char* p) {
  strncpy(path_, p, sizeof(path_) - 1);
  path_[sizeof(path_) - 1] = '\0';
  // Extract filename from path
  const char* slash = strrchr(p, '/');
  strncpy(name_, slash ? slash + 1 : p, sizeof(name_) - 1);
  name_[sizeof(name_) - 1] = '\0';
}

bool FsFile::open(const char* path, oflag_t flags) {
  close();
  setPath(path);

  // Check if it's a directory
  struct stat st;
  if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
    isDir_ = true;
    return true;
  }

  const char* mode;
  if ((flags & O_RDWR) && (flags & O_CREAT)) {
    mode = (flags & O_TRUNC) ? "w+b" : "r+b";
    // If file doesn't exist and O_CREAT, create it
    FILE* test = fopen(path, "rb");
    if (!test) {
      mode = "w+b";
    } else {
      fclose(test);
    }
  } else if (flags & O_WRONLY) {
    if (flags & O_APPEND) {
      mode = "ab";
    } else if (flags & O_TRUNC) {
      mode = "wb";
    } else {
      mode = "wb";
    }
  } else {
    mode = "rb";
  }

  fp_ = fopen(path, mode);
  if (!fp_ && (flags & O_CREAT)) {
    fp_ = fopen(path, "w+b");
  }
  return fp_ != nullptr;
}

bool FsFile::openNext(FsFile& dirFile, oflag_t flags) {
  (void)flags;
  if (!dirFile.isDir_) return false;

  // Use a static DIR* per directory file for iteration
  static DIR* dir = nullptr;
  static char dirPath[512] = {};

  // If directory path changed, reopen
  if (strcmp(dirPath, dirFile.path_) != 0) {
    if (dir) closedir(dir);
    dir = opendir(dirFile.path_);
    strncpy(dirPath, dirFile.path_, sizeof(dirPath) - 1);
  }

  if (!dir) return false;

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", dirFile.path_, entry->d_name);

    struct stat st;
    if (stat(fullPath, &st) == 0) {
      close();
      setPath(fullPath);
      if (S_ISDIR(st.st_mode)) {
        isDir_ = true;
      } else {
        fp_ = fopen(fullPath, "rb");
      }
      return true;
    }
  }

  closedir(dir);
  dir = nullptr;
  dirPath[0] = '\0';
  return false;
}

// --- SDCardManager implementation ---

static void mkdirRecursive(const char* path) {
  char tmp[512];
  strncpy(tmp, path, sizeof(tmp) - 1);
  tmp[sizeof(tmp) - 1] = '\0';
  for (char* p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      ::mkdir(tmp, 0755);
      *p = '/';
    }
  }
  ::mkdir(tmp, 0755);
}

bool SDCardManager::begin() {
  // Create essential directories in MEMFS
  mkdirRecursive("/sd");
  mkdirRecursive("/sd/books");
  mkdirRecursive("/sd/.crosspoint");
  mkdirRecursive("/sd/.crosspoint/cache");
  initialized_ = true;
  printf("[SIM] SDCardManager initialized (MEMFS)\n");
  return true;
}

std::vector<String> SDCardManager::listFiles(const char* path, int maxFiles) {
  std::vector<String> files;
  DIR* dir = opendir(path);
  if (!dir) return files;

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr && static_cast<int>(files.size()) < maxFiles) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
    files.push_back(String(entry->d_name));
  }
  closedir(dir);
  return files;
}

String SDCardManager::readFile(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f) return String("");

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (size <= 0 || size > 1024 * 1024) {  // Cap at 1MB for safety
    fclose(f);
    return String("");
  }

  char* buf = new char[size + 1];
  fread(buf, 1, size, f);
  buf[size] = '\0';
  fclose(f);

  String result(buf);
  delete[] buf;
  return result;
}

bool SDCardManager::readFileToStream(const char* path, Print& out, size_t chunkSize) {
  FILE* f = fopen(path, "rb");
  if (!f) return false;

  uint8_t* buf = new uint8_t[chunkSize];
  size_t bytesRead;
  while ((bytesRead = fread(buf, 1, chunkSize, f)) > 0) {
    out.write(buf, bytesRead);
  }
  delete[] buf;
  fclose(f);
  return true;
}

size_t SDCardManager::readFileToBuffer(const char* path, char* buffer, size_t bufferSize, size_t maxBytes) {
  FILE* f = fopen(path, "rb");
  if (!f) return 0;

  size_t toRead = bufferSize - 1;
  if (maxBytes > 0 && maxBytes < toRead) toRead = maxBytes;

  size_t bytesRead = fread(buffer, 1, toRead, f);
  buffer[bytesRead] = '\0';
  fclose(f);
  return bytesRead;
}

bool SDCardManager::writeFile(const char* path, const String& content) {
  FILE* f = fopen(path, "wb");
  if (!f) return false;

  fwrite(content.c_str(), 1, content.length(), f);
  fclose(f);
  return true;
}

bool SDCardManager::ensureDirectoryExists(const char* path) {
  mkdirRecursive(path);
  return true;
}

FsFile SDCardManager::open(const char* path, oflag_t oflag) {
  FsFile file;
  file.open(path, oflag);
  return file;
}

bool SDCardManager::mkdir(const char* path, bool pFlag) {
  if (pFlag) {
    mkdirRecursive(path);
  } else {
    ::mkdir(path, 0755);
  }
  return true;
}

bool SDCardManager::exists(const char* path) {
  struct stat st;
  return stat(path, &st) == 0;
}

bool SDCardManager::remove(const char* path) {
  return ::remove(path) == 0;
}

bool SDCardManager::rmdir(const char* path) {
  return ::rmdir(path) == 0;
}

bool SDCardManager::openFileForRead(const char* moduleName, const char* path, FsFile& file) {
  (void)moduleName;
  return file.open(path, O_RDONLY);
}

bool SDCardManager::openFileForWrite(const char* moduleName, const char* path, FsFile& file) {
  (void)moduleName;
  // Ensure parent directory exists
  char parentDir[512];
  strncpy(parentDir, path, sizeof(parentDir) - 1);
  parentDir[sizeof(parentDir) - 1] = '\0';
  char* lastSlash = strrchr(parentDir, '/');
  if (lastSlash) {
    *lastSlash = '\0';
    mkdirRecursive(parentDir);
  }
  return file.open(path, O_WRONLY | O_CREAT | O_TRUNC);
}

bool SDCardManager::removeDir(const char* path) {
  return rmdir(path);
}

// --- HalStorage delegates to SDCardManager ---

#define SDCard SDCardManager::getInstance()

HalStorage::HalStorage() {}

bool HalStorage::begin() { return SDCard.begin(); }

bool HalStorage::ready() const { return SDCard.ready(); }

std::vector<String> HalStorage::listFiles(const char* path, int maxFiles) {
  return SDCard.listFiles(path, maxFiles);
}

String HalStorage::readFile(const char* path) { return SDCard.readFile(path); }

bool HalStorage::readFileToStream(const char* path, Print& out, size_t chunkSize) {
  return SDCard.readFileToStream(path, out, chunkSize);
}

size_t HalStorage::readFileToBuffer(const char* path, char* buffer, size_t bufferSize, size_t maxBytes) {
  return SDCard.readFileToBuffer(path, buffer, bufferSize, maxBytes);
}

bool HalStorage::writeFile(const char* path, const String& content) {
  return SDCard.writeFile(path, content);
}

bool HalStorage::ensureDirectoryExists(const char* path) {
  return SDCard.ensureDirectoryExists(path);
}

FsFile HalStorage::open(const char* path, const oflag_t oflag) {
  return SDCard.open(path, oflag);
}

bool HalStorage::mkdir(const char* path, const bool pFlag) {
  return SDCard.mkdir(path, pFlag);
}

bool HalStorage::exists(const char* path) { return SDCard.exists(path); }

bool HalStorage::remove(const char* path) { return SDCard.remove(path); }

bool HalStorage::rmdir(const char* path) { return SDCard.rmdir(path); }

bool HalStorage::openFileForRead(const char* moduleName, const char* path, FsFile& file) {
  return SDCard.openFileForRead(moduleName, path, file);
}

bool HalStorage::openFileForRead(const char* moduleName, const std::string& path, FsFile& file) {
  return openFileForRead(moduleName, path.c_str(), file);
}

bool HalStorage::openFileForRead(const char* moduleName, const String& path, FsFile& file) {
  return openFileForRead(moduleName, path.c_str(), file);
}

bool HalStorage::openFileForWrite(const char* moduleName, const char* path, FsFile& file) {
  return SDCard.openFileForWrite(moduleName, path, file);
}

bool HalStorage::openFileForWrite(const char* moduleName, const std::string& path, FsFile& file) {
  return openFileForWrite(moduleName, path.c_str(), file);
}

bool HalStorage::openFileForWrite(const char* moduleName, const String& path, FsFile& file) {
  return openFileForWrite(moduleName, path.c_str(), file);
}

bool HalStorage::removeDir(const char* path) { return SDCard.removeDir(path); }
