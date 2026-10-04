#include "Blacklist.h"

#define BLACKLIST_FILENAME  "/s_blacklist"

static File openWrite(FILESYSTEM* _fs, const char* filename) {
  #if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
    _fs->remove(filename);
    return _fs->open(filename, FILE_O_WRITE);
  #elif defined(RP2040_PLATFORM)
    return _fs->open(filename, "w");
  #else
    return _fs->open(filename, "w", true);
  #endif
}

void Blacklist::load(FILESYSTEM* fs) {
  _fs = fs;
  num_entries = 0;
  if (_fs->exists(BLACKLIST_FILENAME)) {
  #if defined(RP2040_PLATFORM)
    File file = _fs->open(BLACKLIST_FILENAME, "r");
  #else
    File file = _fs->open(BLACKLIST_FILENAME);
  #endif
    if (file) {
      while (num_entries < MAX_BLACKLIST) {
        BlacklistEntry e;
        bool success = (file.read((uint8_t *)&e.len, 1) == 1);
        success = success && (file.read(e.prefix, 3) == 3);
        if (!success) break;  // EOF

        entries[num_entries++] = e;
      }
      file.close();
    }
  }
}

void Blacklist::save(FILESYSTEM* fs) {
  _fs = fs;
  File file = openWrite(_fs, BLACKLIST_FILENAME);
  if (file) {
    for (int i = 0; i < num_entries; i++) {
      auto e = &entries[i];
      bool success = (file.write((uint8_t *)&e->len, 1) == 1);
      success = success && (file.write(e->prefix, 3) == 3);
      if (!success) break;  // write failed
    }
    file.close();
  }
}

int Blacklist::indexOf(const uint8_t* prefix, uint8_t len) const {
  for (int i = 0; i < num_entries; i++) {
    if (entries[i].len == len && memcmp(entries[i].prefix, prefix, len) == 0) return i;
  }
  return -1;
}

bool Blacklist::add(const uint8_t* prefix, uint8_t len) {
  if (indexOf(prefix, len) >= 0) return true;  // already present, idempotent
  if (num_entries >= MAX_BLACKLIST) return false;  // full

  BlacklistEntry e;
  memset(&e, 0, sizeof(e));
  e.len = len;
  memcpy(e.prefix, prefix, len);
  entries[num_entries++] = e;
  return true;
}

bool Blacklist::remove(const uint8_t* prefix, uint8_t len) {
  int idx = indexOf(prefix, len);
  if (idx < 0) return false;  // not found

  num_entries--;
  while (idx < num_entries) {
    entries[idx] = entries[idx + 1];
    idx++;
  }
  return true;
}

void Blacklist::clear() {
  if (_fs && _fs->exists(BLACKLIST_FILENAME)) {
    _fs->remove(BLACKLIST_FILENAME);
  }
  memset(entries, 0, sizeof(entries));
  num_entries = 0;
}

bool Blacklist::matches(const uint8_t* hash, uint8_t hash_size) const {
  for (int i = 0; i < num_entries; i++) {
    auto e = &entries[i];
    if (e->len > hash_size) continue;  // packet's hop hash is too short to resolve this entry
    if (memcmp(e->prefix, hash, e->len) == 0) return true;
  }
  return false;
}
