// Stub implementations for network classes that are too heavy to compile
// in the simulator (deep ESP32/ArduinoJson dependencies).
// These classes are non-functional in the web simulator.

#include "network/CrossPointWebServer.h"
#include "network/HttpDownloader.h"
#include "network/OtaUpdater.h"
#include "KOReaderSyncClient.h"

#include <cstdio>

// --- CrossPointWebServer stubs ---

CrossPointWebServer::CrossPointWebServer() {}
CrossPointWebServer::~CrossPointWebServer() { stop(); }

void CrossPointWebServer::begin() {
  printf("[SIM] CrossPointWebServer::begin() - not available in simulator\n");
}

void CrossPointWebServer::stop() {
  running = false;
}

void CrossPointWebServer::handleClient() {}

CrossPointWebServer::WsUploadStatus CrossPointWebServer::getWsUploadStatus() const {
  return WsUploadStatus{};
}

void CrossPointWebServer::onWebSocketEvent(uint8_t, WStype_t, uint8_t*, size_t) {}
void CrossPointWebServer::wsEventCallback(uint8_t, WStype_t, uint8_t*, size_t) {}
void CrossPointWebServer::scanFiles(const char*, const std::function<void(FileInfo)>&) const {}
String CrossPointWebServer::formatFileSize(size_t) const { return String("0 B"); }
bool CrossPointWebServer::isEpubFile(const String&) const { return false; }

void CrossPointWebServer::handleRoot() const {}
void CrossPointWebServer::handleNotFound() const {}
void CrossPointWebServer::handleStatus() const {}
void CrossPointWebServer::handleFileList() const {}
void CrossPointWebServer::handleFileListData() const {}
void CrossPointWebServer::handleDownload() const {}
void CrossPointWebServer::handleUpload(UploadState&) const {}
void CrossPointWebServer::handleUploadPost(UploadState&) const {}
void CrossPointWebServer::handleCreateFolder() const {}
void CrossPointWebServer::handleRename() const {}
void CrossPointWebServer::handleMove() const {}
void CrossPointWebServer::handleDelete() const {}
void CrossPointWebServer::handleSettingsPage() const {}
void CrossPointWebServer::handleGetSettings() const {}
void CrossPointWebServer::handlePostSettings() {}

// --- HttpDownloader stubs ---

bool HttpDownloader::fetchUrl(const std::string& url, std::string& outContent) {
  (void)url;
  outContent.clear();
  printf("[SIM] HttpDownloader::fetchUrl() - not available in simulator\n");
  return false;
}

bool HttpDownloader::fetchUrl(const std::string& url, Stream& stream) {
  (void)url;
  (void)stream;
  printf("[SIM] HttpDownloader::fetchUrl(stream) - not available in simulator\n");
  return false;
}

HttpDownloader::DownloadError HttpDownloader::downloadToFile(
    const std::string& url, const std::string& destPath, ProgressCallback progress) {
  (void)url;
  (void)destPath;
  (void)progress;
  printf("[SIM] HttpDownloader::downloadToFile() - not available in simulator\n");
  return HTTP_ERROR;
}

// --- OtaUpdater stubs ---

OtaUpdater::OtaUpdaterError OtaUpdater::checkForUpdate() {
  printf("[SIM] OtaUpdater::checkForUpdate() - not available in simulator\n");
  return INTERNAL_UPDATE_ERROR;
}

bool OtaUpdater::isUpdateNewer() const { return false; }

const std::string& OtaUpdater::getLatestVersion() const { return latestVersion; }

OtaUpdater::OtaUpdaterError OtaUpdater::installUpdate() {
  printf("[SIM] OtaUpdater::installUpdate() - not available in simulator\n");
  return INTERNAL_UPDATE_ERROR;
}

// --- KOReaderSyncClient stubs ---

KOReaderSyncClient::Error KOReaderSyncClient::authenticate() {
  printf("[SIM] KOReaderSyncClient::authenticate() - not available in simulator\n");
  return NETWORK_ERROR;
}

KOReaderSyncClient::Error KOReaderSyncClient::getProgress(
    const std::string& documentHash, KOReaderProgress& outProgress) {
  (void)documentHash;
  (void)outProgress;
  return NETWORK_ERROR;
}

KOReaderSyncClient::Error KOReaderSyncClient::updateProgress(const KOReaderProgress& progress) {
  (void)progress;
  return NETWORK_ERROR;
}

const char* KOReaderSyncClient::errorString(Error error) {
  switch (error) {
    case OK: return "OK";
    case NO_CREDENTIALS: return "No credentials";
    case NETWORK_ERROR: return "Network not available (simulator)";
    case AUTH_FAILED: return "Auth failed";
    case SERVER_ERROR: return "Server error";
    case JSON_ERROR: return "JSON error";
    case NOT_FOUND: return "Not found";
    default: return "Unknown error";
  }
}
