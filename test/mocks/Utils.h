#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>

#define PUB_KEY_SIZE 32
#define PRV_KEY_SIZE 64
#define SIGNATURE_SIZE 64
#define CIPHER_MAC_SIZE 16

namespace mesh {

class RNG {
public:
  virtual ~RNG() = default;
  virtual void random(uint8_t* dest, size_t sz) = 0;
};

class Utils {
public:
  static void sha256(uint8_t* hash, size_t hash_len, const uint8_t*, int) {
    std::memset(hash, 0, hash_len);
  }

  static int encryptThenMAC(const uint8_t*, uint8_t* dest, const uint8_t* src, int src_len) {
    int out_len = src_len + CIPHER_MAC_SIZE;
    std::memset(dest, 0xAA, CIPHER_MAC_SIZE);
    std::memcpy(dest + CIPHER_MAC_SIZE, src, src_len);
    return out_len;
  }

  static int MACThenDecrypt(const uint8_t*, uint8_t* dest, const uint8_t* src, int src_len) {
    if (src_len < CIPHER_MAC_SIZE) {
      return 0;
    }
    int out_len = src_len - CIPHER_MAC_SIZE;
    std::memcpy(dest, src + CIPHER_MAC_SIZE, out_len);
    return out_len;
  }

  static void toHex(char* dest, const uint8_t* src, size_t len) {
    static const char hex_chars[] = "0123456789ABCDEF";
    while (len > 0) {
      uint8_t b = *src++;
      *dest++ = hex_chars[b >> 4];
      *dest++ = hex_chars[b & 0x0F];
      len--;
    }
    *dest = 0;
  }

  static bool isHexChar(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
  }

  static bool fromHex(uint8_t* dest, int dest_size, const char* src_hex) {
    if ((int)std::strlen(src_hex) != dest_size * 2) return false;
    for (int i = 0; i < dest_size; i++) {
      auto nibble = [](char c) -> uint8_t {
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return c - '0';
      };
      dest[i] = (nibble(src_hex[i * 2]) << 4) | nibble(src_hex[i * 2 + 1]);
    }
    return true;
  }
};

}
