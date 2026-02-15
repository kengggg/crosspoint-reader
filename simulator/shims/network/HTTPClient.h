#pragma once
// HTTPClient stub for Emscripten web simulator.

#include "WString.h"
#include "Stream.h"
#include "WiFiClient.h"

#define HTTP_CODE_OK 200
#define HTTP_CODE_MOVED_PERMANENTLY 301
#define HTTP_CODE_FOUND 302
#define HTTP_CODE_NOT_FOUND 404

class HTTPClient {
 public:
  bool begin(const String& url) {
    (void)url;
    return false;
  }
  bool begin(const char* url) {
    (void)url;
    return false;
  }
  bool begin(WiFiClient& client, const String& url) {
    (void)client;
    (void)url;
    return false;
  }
  void end() {}
  void setAuthorization(const char* user, const char* password) {
    (void)user;
    (void)password;
  }
  void setAuthorization(const char* auth) { (void)auth; }
  void addHeader(const String& name, const String& value) {
    (void)name;
    (void)value;
  }
  void setTimeout(uint16_t timeout) { (void)timeout; }
  void setFollowRedirects(int follow) { (void)follow; }
  void setRedirectLimit(int limit) { (void)limit; }
  void setConnectTimeout(int timeout) { (void)timeout; }

  int GET() { return -1; }
  int POST(const String& payload) {
    (void)payload;
    return -1;
  }
  int POST(const uint8_t* payload, size_t size) {
    (void)payload;
    (void)size;
    return -1;
  }
  int PUT(const String& payload) {
    (void)payload;
    return -1;
  }
  int sendRequest(const char* type, const String& payload) {
    (void)type;
    (void)payload;
    return -1;
  }

  String getString() { return String(""); }
  int getSize() { return 0; }
  Stream& getStream() {
    static WiFiClient dummy;
    return dummy;
  }
  WiFiClient* getStreamPtr() { return nullptr; }
  bool connected() { return false; }
  void writeToStream(Stream* stream) { (void)stream; }

  String header(const char* name) {
    (void)name;
    return String("");
  }
  bool hasHeader(const char* name) {
    (void)name;
    return false;
  }
  int headers() { return 0; }
  String headerName(int i) {
    (void)i;
    return String("");
  }
};

#define HTTPC_ERROR_CONNECTION_REFUSED (-1)
#define HTTPC_ERROR_SEND_HEADER_FAILED (-2)
#define HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
#define HTTPC_ERROR_NOT_CONNECTED (-4)
#define HTTPC_ERROR_CONNECTION_LOST (-5)
#define HTTPC_ERROR_NO_STREAM (-6)
#define HTTPC_ERROR_NO_HTTP_SERVER (-7)
#define HTTPC_ERROR_TOO_LESS_RAM (-8)
#define HTTPC_ERROR_ENCODING (-9)
#define HTTPC_ERROR_STREAM_WRITE (-10)
#define HTTPC_ERROR_READ_TIMEOUT (-11)

#define HTTPC_STRICT_FOLLOW_REDIRECTS 1
#define HTTPC_FORCE_FOLLOW_REDIRECTS 2
