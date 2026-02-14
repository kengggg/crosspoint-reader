#pragma once
// mDNS stub for Emscripten web emulator.

class MDNSResponder {
 public:
  bool begin(const char* hostName) {
    (void)hostName;
    return false;
  }
  void end() {}
  void addService(const char* service, const char* proto, uint16_t port) {
    (void)service;
    (void)proto;
    (void)port;
  }
};

extern MDNSResponder MDNS;
