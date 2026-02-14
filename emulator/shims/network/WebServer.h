#pragma once
// WebServer stub for Emscripten web emulator.

#include "WString.h"
#include "WiFiClient.h"
#include <functional>

class WebServer {
 public:
  WebServer(int port = 80) { (void)port; }

  void begin() {}
  void close() {}
  void stop() {}
  void handleClient() {}

  typedef std::function<void()> THandlerFunction;
  void on(const char* uri, THandlerFunction handler) {
    (void)uri;
    (void)handler;
  }
  void on(const char* uri, int method, THandlerFunction handler) {
    (void)uri;
    (void)method;
    (void)handler;
  }
  void on(const char* uri, int method, THandlerFunction handler, THandlerFunction uploadHandler) {
    (void)uri;
    (void)method;
    (void)handler;
    (void)uploadHandler;
  }
  void onNotFound(THandlerFunction handler) { (void)handler; }

  void send(int code, const char* contentType = nullptr, const String& content = String()) {
    (void)code;
    (void)contentType;
    (void)content;
  }
  void send(int code, const char* contentType, const char* content) {
    (void)code;
    (void)contentType;
    (void)content;
  }
  void sendHeader(const String& name, const String& value, bool first = false) {
    (void)name;
    (void)value;
    (void)first;
  }

  String arg(const String& name) {
    (void)name;
    return String();
  }
  String arg(int i) {
    (void)i;
    return String();
  }
  bool hasArg(const String& name) {
    (void)name;
    return false;
  }
  int args() { return 0; }
  String argName(int i) {
    (void)i;
    return String();
  }

  String uri() { return String("/"); }
  int method() { return 0; }
  WiFiClient client() { return WiFiClient(); }

  String hostHeader() { return String(); }

  // Upload handler
  struct HTTPUpload {
    int status = 0;
    String filename;
    String name;
    String type;
    size_t totalSize = 0;
    size_t currentSize = 0;
    uint8_t buf[1024] = {};
  };
  HTTPUpload& upload() {
    static HTTPUpload u;
    return u;
  }

  // HTTP methods
  enum { HTTP_GET = 1, HTTP_POST = 2, HTTP_PUT = 3, HTTP_DELETE = 4, HTTP_ANY = 255 };
};

#define UPLOAD_FILE_START 0
#define UPLOAD_FILE_WRITE 1
#define UPLOAD_FILE_END 2
#define UPLOAD_FILE_ABORTED 3

#define HTTP_UPLOAD_BUFLEN 1024

// Global HTTP method constants (ESP32 WebServer defines these globally)
#define HTTP_GET 1
#define HTTP_POST 2
#define HTTP_PUT 3
#define HTTP_DELETE 4
#define HTTP_ANY 255
