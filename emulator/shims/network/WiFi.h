#pragma once
// WiFi stub for Emscripten web emulator.
// Network features are not available in the web emulator.

#include "IPAddress.h"
#include "WString.h"
#include "esp_wifi.h"

#define WIFI_OFF 0
#define WIFI_STA 1
#define WIFI_AP 2
#define WIFI_AP_STA 3

#define WL_CONNECTED 3
#define WL_DISCONNECTED 6
#define WL_IDLE_STATUS 0
#define WL_NO_SSID_AVAIL 1
#define WL_CONNECT_FAILED 4
#define WL_CONNECTION_LOST 5

#define WIFI_SCAN_RUNNING (-1)
#define WIFI_SCAN_FAILED (-2)
#define WIFI_AUTH_OPEN 0
#define WIFI_AUTH_WEP 1
#define WIFI_AUTH_WPA_PSK 2
#define WIFI_AUTH_WPA2_PSK 3

typedef int wl_status_t;

class WiFiClass {
 public:
  bool mode(int m) {
    (void)m;
    return true;
  }
  int begin(const char* ssid, const char* passphrase = nullptr) {
    (void)ssid;
    (void)passphrase;
    return WL_DISCONNECTED;
  }
  void disconnect(bool wifiOff = false) { (void)wifiOff; }
  wl_status_t status() { return WL_DISCONNECTED; }
  IPAddress localIP() { return IPAddress(0, 0, 0, 0); }
  IPAddress softAPIP() { return IPAddress(0, 0, 0, 0); }
  String SSID() { return String(""); }
  String psk() { return String(""); }
  bool softAP(const char* ssid, const char* pass = nullptr, int channel = 1, int ssid_hidden = 0,
              int max_connection = 4) {
    (void)ssid; (void)pass; (void)channel; (void)ssid_hidden; (void)max_connection;
    return false;
  }
  void softAPdisconnect(bool wifiOff = false) { (void)wifiOff; }
  int8_t scanNetworks(bool async = false) {
    (void)async;
    return 0;
  }
  int16_t scanComplete() { return 0; }
  void scanDelete() {}
  String SSID(uint8_t networkItem) {
    (void)networkItem;
    return String("");
  }
  int32_t RSSI() { return -100; }
  int32_t RSSI(uint8_t networkItem) {
    (void)networkItem;
    return -100;
  }
  uint8_t encryptionType(uint8_t networkItem) {
    (void)networkItem;
    return WIFI_AUTH_OPEN;
  }
  String macAddress() { return String("00:00:00:00:00:00"); }
  String macAddress(uint8_t* mac) {
    (void)mac;
    return String("00:00:00:00:00:00");
  }
  bool isConnected() { return false; }
  void setHostname(const char* hostname) { (void)hostname; }
  void setAutoReconnect(bool autoReconnect) { (void)autoReconnect; }
  String BSSIDstr(uint8_t networkItem = 0) {
    (void)networkItem;
    return String("");
  }
  int32_t channel() { return 0; }
  wifi_mode_t getMode() { return WIFI_MODE_NULL; }
  int softAPgetStationNum() { return 0; }
  void setSleep(bool enable) { (void)enable; }
};

extern WiFiClass WiFi;
