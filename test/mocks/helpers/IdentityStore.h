#pragma once

// Minimal in-memory filesystem stand-in, just enough for Blacklist's
// load()/save()/exists()/remove() calls in the native unit-test build.
// Real firmware uses fs::FS / Adafruit_LittleFS via the FILESYSTEM macro
// (see the real src/helpers/IdentityStore.h); this mock replaces that
// header so tests don't need a real flash filesystem.

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <map>
#include <string>
#include <vector>

class MockFile {
  std::vector<uint8_t>* _data = nullptr;

public:
  size_t pos = 0;

  MockFile() = default;
  MockFile(std::vector<uint8_t>* data) : _data(data) { }

  operator bool() const { return _data != nullptr; }

  size_t read(uint8_t* dest, size_t len) {
    if (!_data) return 0;
    size_t avail = _data->size() > pos ? _data->size() - pos : 0;
    size_t n = len < avail ? len : avail;
    std::memcpy(dest, _data->data() + pos, n);
    pos += n;
    return n;
  }

  size_t write(const uint8_t* src, size_t len) {
    if (!_data) return 0;
    _data->insert(_data->end(), src, src + len);
    return len;
  }

  void close() { _data = nullptr; }
};
using File = MockFile;

class MockFS {
  std::map<std::string, std::vector<uint8_t>> _files;

public:
  bool exists(const char* path) { return _files.count(path) > 0; }

  bool remove(const char* path) { return _files.erase(path) > 0; }

  void mkdir(const char*) { }

  // read-mode open, as used on most platforms (and native) for Blacklist::load()
  File open(const char* path) {
    auto it = _files.find(path);
    if (it == _files.end()) return File();
    return File(&it->second);
  }

  // write-mode open, as used by Blacklist::save()'s native branch: open(path, "w", true)
  File open(const char* path, const char* mode, bool /*create*/ = false) {
    auto& v = _files[path];
    if (std::strcmp(mode, "w") == 0) v.clear();
    return File(&v);
  }
};
#define FILESYSTEM MockFS
