#pragma once
// WebSocketsServer stub for Emscripten web emulator.

#include <cstdint>
#include <cstddef>

enum WStype_t {
  WStype_DISCONNECTED = 0,
  WStype_CONNECTED,
  WStype_TEXT,
  WStype_BIN,
  WStype_ERROR,
  WStype_FRAGMENT_TEXT_START,
  WStype_FRAGMENT_BIN_START,
  WStype_FRAGMENT,
  WStype_FRAGMENT_FIN,
  WStype_PING,
  WStype_PONG,
};

class WebSocketsServer {
 public:
  WebSocketsServer(uint16_t port) { (void)port; }

  typedef void (*WebSocketServerEvent)(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

  void begin() {}
  void close() {}
  void loop() {}
  void onEvent(WebSocketServerEvent handler) { (void)handler; }
  bool sendTXT(uint8_t num, const char* payload) {
    (void)num;
    (void)payload;
    return false;
  }
  bool sendBIN(uint8_t num, const uint8_t* payload, size_t length) {
    (void)num;
    (void)payload;
    (void)length;
    return false;
  }
  bool broadcastTXT(const char* payload) {
    (void)payload;
    return false;
  }
  void disconnect(uint8_t num) { (void)num; }
};
