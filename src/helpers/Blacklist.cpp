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
  mode = BLACKLIST_MODE_OFF;
  num_entries = 0;
  if (_fs->exists(BLACKLIST_FILENAME)) {
  #if defined(RP2040_PLATFORM)
    File file = _fs->open(BLACKLIST_FILENAME, "r");
  #else
    File file = _fs->open(BLACKLIST_FILENAME);
  #endif
    if (file) {
      if (file.read((uint8_t *)&mode, 1) == 1 && mode > BLACKLIST_MODE_INDIRECT) {
        mode = BLACKLIST_MODE_OFF;  // corrupt/unknown value, fall back to safe default
      }
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
    bool success = (file.write((uint8_t *)&mode, 1) == 1);
    for (int i = 0; success && i < num_entries; i++) {
      auto e = &entries[i];
      success = (file.write((uint8_t *)&e->len, 1) == 1);
      success = success && (file.write(e->prefix, 3) == 3);
      if (!success) break;  // write failed
    }
    file.close();
  }
}

bool Blacklist::setMode(uint8_t m) {
  if (m > BLACKLIST_MODE_INDIRECT) return false;
  mode = m;
  return true;
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

static bool isValidHexPrefix(const char* hex, int len) {
  if (len != 2 && len != 4 && len != 6) return false;
  for (int i = 0; i < len; i++) {
    if (!mesh::Utils::isHexChar(hex[i])) return false;
  }
  return true;
}

void Blacklist::handleCommand(const char* sub, char* reply) {
  if (memcmp(sub, "add ", 4) == 0) {
    const char* hex = sub + 4;
    int hex_len = strlen(hex);
    uint8_t prefix[3];
    if (!isValidHexPrefix(hex, hex_len) || !mesh::Utils::fromHex(prefix, hex_len / 2, hex)) {
      strcpy(reply, "Err - prefix must be 2/4/6 hex chars (1-3 bytes)");
    } else if (add(prefix, hex_len / 2)) {
      save(_fs);
      strcpy(reply, "OK");
    } else {
      strcpy(reply, "Err - blacklist full");
    }
  } else if (memcmp(sub, "remove ", 7) == 0) {
    const char* hex = sub + 7;
    int hex_len = strlen(hex);
    uint8_t prefix[3];
    if (!isValidHexPrefix(hex, hex_len) || !mesh::Utils::fromHex(prefix, hex_len / 2, hex)) {
      strcpy(reply, "Err - prefix must be 2/4/6 hex chars (1-3 bytes)");
    } else if (remove(prefix, hex_len / 2)) {
      save(_fs);
      strcpy(reply, "OK");
    } else {
      strcpy(reply, "Err - not found");
    }
  } else if (strcmp(sub, "list") == 0) {
    char* dp = reply;
    for (int i = 0; i < num_entries && dp - reply < 134; i++) {
      auto e = &entries[i];
      if (i > 0) *dp++ = ' ';
      char hex[8];
      mesh::Utils::toHex(hex, e->prefix, e->len);
      sprintf(dp, "%d:%s", (int)e->len, hex);
      while (*dp) dp++;  // find end of string
    }
    if (dp == reply) {  // no entries, need non-empty response
      strcpy(dp, "-none-");
      dp += 6;
    }
    *dp = 0;
  } else if (strcmp(sub, "clear") == 0) {
    clear();
    strcpy(reply, "OK");
  } else if (strcmp(sub, "mode") == 0) {
    strcpy(reply, mode == BLACKLIST_MODE_OFF ? "off" : (mode == BLACKLIST_MODE_DIRECT ? "direct" : "indirect"));
  } else if (memcmp(sub, "mode ", 5) == 0) {
    const char* arg = sub + 5;
    uint8_t m;
    if (strcmp(arg, "off") == 0) {
      m = BLACKLIST_MODE_OFF;
    } else if (strcmp(arg, "direct") == 0) {
      m = BLACKLIST_MODE_DIRECT;
    } else if (strcmp(arg, "indirect") == 0) {
      m = BLACKLIST_MODE_INDIRECT;
    } else {
      m = 0xFF;
    }
    if (m == 0xFF || !setMode(m)) {
      strcpy(reply, "Err - must be: off, direct, or indirect");
    } else {
      save(_fs);
      strcpy(reply, "OK");
    }
  } else {
    strcpy(reply, "Err - bad params");
  }
}
