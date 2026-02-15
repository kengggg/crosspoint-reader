#pragma once
// ArduinoJson stub for Emscripten web simulator.
// Only used by network features (OTA, WebServer, KOReaderSync) which are non-functional
// in the simulator. This provides just enough types to compile.

#include "WString.h"
#include <cstddef>

namespace ArduinoJson {
namespace detail {
template <typename T>
struct is_string : std::false_type {};
}
}

class JsonVariant {
 public:
  JsonVariant() = default;
  template <typename T>
  T as() const { return T(); }
  bool isNull() const { return true; }
  operator bool() const { return false; }
  const char* operator|(const char* def) const { return def; }
  int operator|(int def) const { return def; }
  JsonVariant operator[](const char*) const { return {}; }
  JsonVariant operator[](int) const { return {}; }
};

class JsonObject {
 public:
  JsonVariant operator[](const char*) const { return {}; }
  bool isNull() const { return true; }
  void clear() {}
  int size() const { return 0; }
};

class JsonArray {
 public:
  JsonVariant operator[](int) const { return {}; }
  bool isNull() const { return true; }
  int size() const { return 0; }
  JsonVariant* begin() { return nullptr; }
  JsonVariant* end() { return nullptr; }
};

class JsonDocument {
 public:
  JsonDocument() = default;
  explicit JsonDocument(size_t capacity) { (void)capacity; }

  template <typename T>
  T as() const { return T(); }
  JsonVariant operator[](const char*) const { return {}; }
  JsonObject to() { return {}; }
  template <typename T>
  T to() { return T(); }

  bool isNull() const { return true; }
  void clear() {}
  size_t memoryUsage() const { return 0; }

  // Filter
  JsonDocument& filter(const JsonDocument&) { return *this; }
};

// Deserialization
enum DeserializationError_t {
  DeserializationError_Ok = 0,
  DeserializationError_EmptyInput,
  DeserializationError_IncompleteInput,
  DeserializationError_InvalidInput,
  DeserializationError_NoMemory,
  DeserializationError_TooDeep,
};

class DeserializationError {
 public:
  DeserializationError(DeserializationError_t err = DeserializationError_Ok) : err_(err) {}
  operator bool() const { return err_ != DeserializationError_Ok; }
  const char* c_str() const { return "error"; }

 private:
  DeserializationError_t err_;
};

template <typename TDocument>
DeserializationError deserializeJson(TDocument& doc, const char* json) {
  (void)doc;
  (void)json;
  return DeserializationError(DeserializationError_InvalidInput);
}

template <typename TDocument>
DeserializationError deserializeJson(TDocument& doc, const String& json) {
  (void)doc;
  (void)json;
  return DeserializationError(DeserializationError_InvalidInput);
}

template <typename TDocument, typename TFilter>
DeserializationError deserializeJson(TDocument& doc, const char* json, int, TFilter filter) {
  (void)doc;
  (void)json;
  (void)filter;
  return DeserializationError(DeserializationError_InvalidInput);
}

template <typename TDocument>
size_t serializeJson(const TDocument& doc, char* output, size_t size) {
  (void)doc;
  (void)output;
  (void)size;
  return 0;
}

template <typename TDocument>
size_t serializeJson(const TDocument& doc, String& output) {
  (void)doc;
  (void)output;
  return 0;
}
