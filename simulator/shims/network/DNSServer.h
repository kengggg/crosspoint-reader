#pragma once
// DNSServer stub for Emscripten web simulator.

#include "IPAddress.h"

namespace DNSReplyCode {
constexpr int NoError = 0;
constexpr int ServerFailure = 2;
}

class DNSServer {
 public:
  void start(int port, const char* domainName, IPAddress resolvedIP) {
    (void)port;
    (void)domainName;
    (void)resolvedIP;
  }
  void stop() {}
  void processNextRequest() {}
  void setTTL(int ttl) { (void)ttl; }
  void setErrorReplyCode(int code) { (void)code; }
};
