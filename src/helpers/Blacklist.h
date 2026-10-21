#pragma once

#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>
#include <helpers/IdentityStore.h>

#ifndef MAX_BLACKLIST
  #define MAX_BLACKLIST   16
#endif

#define BLACKLIST_MODE_OFF        0
#define BLACKLIST_MODE_DIRECT     1   // only the last hop in the path is checked
#define BLACKLIST_MODE_INDIRECT   2   // every hop in the path is checked

struct BlacklistEntry {
  uint8_t len;        // 1-3
  uint8_t prefix[3];
};

// Self-contained: owns its own mode setting and its own persistence
// (/s_blacklist). Nothing outside this class/MyMesh needs to know how
// either is stored.
class Blacklist {
  FILESYSTEM* _fs;
  uint8_t mode;
  BlacklistEntry entries[MAX_BLACKLIST];
  int num_entries;

  int indexOf(const uint8_t* prefix, uint8_t len) const;

public:
  Blacklist() {
    mode = BLACKLIST_MODE_OFF;
    memset(entries, 0, sizeof(entries));
    num_entries = 0;
  }

  void load(FILESYSTEM* fs);
  void save(FILESYSTEM* fs);

  uint8_t getMode() const { return mode; }
  bool setMode(uint8_t m);   // false if not a valid BLACKLIST_MODE_* value

  bool add(const uint8_t* prefix, uint8_t len);     // false if already full
  bool remove(const uint8_t* prefix, uint8_t len);  // false if not found
  void clear();

  int getCount() const { return num_entries; }
  const BlacklistEntry* getEntry(int idx) const { return &entries[idx]; }

  // true if `hash` (hash_size bytes, as read from a packet path entry) matches any stored entry
  bool matches(const uint8_t* hash, uint8_t hash_size) const;

  // Handles "add <hex>", "remove <hex>", "list", "clear", "mode", "mode <off|direct|indirect>".
  // `sub` is the command text after "blacklist" and any leading space has been stripped.
  // Writes the response into `reply` and persists any mutation itself.
  void handleCommand(const char* sub, char* reply);
};
