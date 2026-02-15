#pragma once
// Arduino String class shim for Emscripten web simulator
// Provides the subset of Arduino's String API used by CrossPoint firmware.

#include <cstdlib>
#include <cstring>
#include <string>

class String {
 public:
  String() = default;
  String(const char* s) : str_(s ? s : "") {}
  String(const std::string& s) : str_(s) {}
  String(const String& s) : str_(s.str_) {}
  String(String&& s) noexcept : str_(std::move(s.str_)) {}
  String(char c) : str_(1, c) {}
  String(int val) : str_(std::to_string(val)) {}
  String(unsigned int val) : str_(std::to_string(val)) {}
  String(long val) : str_(std::to_string(val)) {}
  String(unsigned long val) : str_(std::to_string(val)) {}
  String(float val, unsigned int decimalPlaces = 2) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.*f", decimalPlaces, val);
    str_ = buf;
  }
  String(double val, unsigned int decimalPlaces = 2) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.*f", decimalPlaces, val);
    str_ = buf;
  }

  String& operator=(const String& rhs) {
    str_ = rhs.str_;
    return *this;
  }
  String& operator=(String&& rhs) noexcept {
    str_ = std::move(rhs.str_);
    return *this;
  }
  String& operator=(const char* s) {
    str_ = s ? s : "";
    return *this;
  }

  const char* c_str() const { return str_.c_str(); }
  unsigned int length() const { return static_cast<unsigned int>(str_.length()); }
  bool isEmpty() const { return str_.empty(); }

  bool equals(const String& s) const { return str_ == s.str_; }
  bool equals(const char* s) const { return s ? str_ == s : str_.empty(); }
  bool equalsIgnoreCase(const String& s) const {
    if (str_.size() != s.str_.size()) return false;
    for (size_t i = 0; i < str_.size(); i++) {
      if (tolower(str_[i]) != tolower(s.str_[i])) return false;
    }
    return true;
  }

  bool operator==(const String& rhs) const { return str_ == rhs.str_; }
  bool operator==(const char* rhs) const { return rhs ? str_ == rhs : str_.empty(); }
  bool operator!=(const String& rhs) const { return str_ != rhs.str_; }
  bool operator!=(const char* rhs) const { return !(*this == rhs); }
  bool operator<(const String& rhs) const { return str_ < rhs.str_; }

  String operator+(const String& rhs) const { return String(str_ + rhs.str_); }
  String operator+(const char* rhs) const { return String(str_ + (rhs ? rhs : "")); }
  String operator+(char c) const { return String(str_ + c); }
  friend String operator+(const char* lhs, const String& rhs) { return String(std::string(lhs ? lhs : "") + rhs.str_); }

  String& operator+=(const String& rhs) {
    str_ += rhs.str_;
    return *this;
  }
  String& operator+=(const char* rhs) {
    if (rhs) str_ += rhs;
    return *this;
  }
  String& operator+=(char c) {
    str_ += c;
    return *this;
  }

  char operator[](unsigned int index) const { return str_[index]; }
  char& operator[](unsigned int index) { return str_[index]; }
  char charAt(unsigned int index) const { return str_[index]; }

  bool startsWith(const String& prefix) const {
    return str_.compare(0, prefix.str_.size(), prefix.str_) == 0;
  }
  bool startsWith(const char* prefix) const {
    if (!prefix) return true;
    size_t len = strlen(prefix);
    return str_.compare(0, len, prefix) == 0;
  }
  bool endsWith(const String& suffix) const {
    if (suffix.str_.size() > str_.size()) return false;
    return str_.compare(str_.size() - suffix.str_.size(), suffix.str_.size(), suffix.str_) == 0;
  }
  bool endsWith(const char* suffix) const {
    if (!suffix) return true;
    size_t len = strlen(suffix);
    if (len > str_.size()) return false;
    return str_.compare(str_.size() - len, len, suffix) == 0;
  }

  String substring(unsigned int from) const { return String(str_.substr(from)); }
  String substring(unsigned int from, unsigned int to) const {
    if (from >= str_.size()) return String();
    return String(str_.substr(from, to - from));
  }

  int indexOf(char c) const {
    auto pos = str_.find(c);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }
  int indexOf(char c, unsigned int from) const {
    auto pos = str_.find(c, from);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }
  int indexOf(const String& s) const {
    auto pos = str_.find(s.str_);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }
  int indexOf(const String& s, unsigned int from) const {
    auto pos = str_.find(s.str_, from);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }
  int lastIndexOf(char c) const {
    auto pos = str_.rfind(c);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }
  int lastIndexOf(const String& s) const {
    auto pos = str_.rfind(s.str_);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
  }

  void replace(const String& from, const String& to) {
    size_t pos = 0;
    while ((pos = str_.find(from.str_, pos)) != std::string::npos) {
      str_.replace(pos, from.str_.length(), to.str_);
      pos += to.str_.length();
    }
  }

  void trim() {
    auto start = str_.find_first_not_of(" \t\r\n");
    auto end = str_.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) {
      str_.clear();
    } else {
      str_ = str_.substr(start, end - start + 1);
    }
  }

  void toLowerCase() {
    for (auto& c : str_) c = tolower(c);
  }
  void toUpperCase() {
    for (auto& c : str_) c = toupper(c);
  }

  long toInt() const { return atol(str_.c_str()); }
  float toFloat() const { return atof(str_.c_str()); }
  double toDouble() const { return atof(str_.c_str()); }

  void remove(unsigned int index) { str_.erase(index); }
  void remove(unsigned int index, unsigned int count) { str_.erase(index, count); }

  // Implicit conversion to std::string
  operator std::string() const { return str_; }

 private:
  std::string str_;
};
