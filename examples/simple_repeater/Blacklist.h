#pragma once

#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>
#include <helpers/IdentityStore.h>

#ifndef MAX_BLACKLIST
  #define MAX_BLACKLIST   16
#endif

struct BlacklistEntry {
  uint8_t len;        // 1-3
  uint8_t prefix[3];
};

class Blacklist {
  FILESYSTEM* _fs;
  BlacklistEntry entries[MAX_BLACKLIST];
  int num_entries;

  int indexOf(const uint8_t* prefix, uint8_t len) const;

public:
  Blacklist() {
    memset(entries, 0, sizeof(entries));
    num_entries = 0;
  }

  void load(FILESYSTEM* fs);
  void save(FILESYSTEM* fs);

  bool add(const uint8_t* prefix, uint8_t len);     // false if already full
  bool remove(const uint8_t* prefix, uint8_t len);  // false if not found
  void clear();

  int getCount() const { return num_entries; }
  const BlacklistEntry* getEntry(int idx) const { return &entries[idx]; }

  // true if `hash` (hash_size bytes, as read from a packet path entry) matches any stored entry
  bool matches(const uint8_t* hash, uint8_t hash_size) const;
};
