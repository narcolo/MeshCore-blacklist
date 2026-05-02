#pragma once

#include <stdint.h>
#include <string.h>

class DisplayDriver {
  int _w, _h;
protected:
  DisplayDriver(int w, int h) { _w = w; _h = h; }
public:
  enum Color { DARK=0, LIGHT, RED, GREEN, BLUE, YELLOW, ORANGE }; // on b/w screen, colors will be !=0 synonym of light

  int width() const { return _w; }
  int height() const { return _h; }

  virtual bool isOn() = 0;
  virtual void turnOn() = 0;
  virtual void turnOff() = 0;
  virtual void clear() = 0;
  virtual void startFrame(Color bkg = DARK) = 0;
  virtual void setTextSize(int sz) = 0;
  virtual void setColor(Color c) = 0;
  virtual void setCursor(int x, int y) = 0;
  virtual void print(const char* str) = 0;
  virtual void printWordWrap(const char* str, int max_width) { print(str); }   // fallback to basic print() if no override
  virtual void fillRect(int x, int y, int w, int h) = 0;
  virtual void drawRect(int x, int y, int w, int h) = 0;
  virtual void drawXbm(int x, int y, const uint8_t* bits, int w, int h) = 0;
  virtual uint16_t getTextWidth(const char* str) = 0;
  virtual void drawTextCentered(int mid_x, int y, const char* str) {   // helper method (override to optimise)
    int w = getTextWidth(str);
    setCursor(mid_x - w/2, y);
    print(str);
  }
  virtual void drawTextRightAlign(int x_anch, int y, const char* str) {
    int w = getTextWidth(str);
    setCursor(x_anch - w, y);
    print(str);
  }
  virtual void drawTextLeftAlign(int x_anch, int y, const char* str) {
    setCursor(x_anch, y);
    print(str);
  }
  
  // convert UTF-8 characters to displayable block characters for compatibility
  virtual void translateUTF8ToBlocks(char* dest, const char* src, size_t dest_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] != 0 && j < dest_size - 1; i++) {
      unsigned char c = (unsigned char)src[i];
      if (c >= 32 && c <= 126) {
        dest[j++] = c;  // ASCII printable
      } else if (c >= 0xC2 && c <= 0xDF && src[i+1]) {
        // 2-byte UTF-8: decode codepoint and transliterate known diacritics
        uint16_t cp = ((c & 0x1F) << 6) | ((unsigned char)src[++i] & 0x3F);
        switch (cp) {
          case 0x0104: dest[j++] = 'A'; break; // Ą
          case 0x0105: dest[j++] = 'a'; break; // ą
          case 0x0106: dest[j++] = 'C'; break; // Ć
          case 0x0107: dest[j++] = 'c'; break; // ć
          case 0x0118: dest[j++] = 'E'; break; // Ę
          case 0x0119: dest[j++] = 'e'; break; // ę
          case 0x0141: dest[j++] = 'L'; break; // Ł
          case 0x0142: dest[j++] = 'l'; break; // ł
          case 0x0143: dest[j++] = 'N'; break; // Ń
          case 0x0144: dest[j++] = 'n'; break; // ń
          case 0x00D3: dest[j++] = 'O'; break; // Ó
          case 0x00F3: dest[j++] = 'o'; break; // ó
          case 0x015A: dest[j++] = 'S'; break; // Ś
          case 0x015B: dest[j++] = 's'; break; // ś
          case 0x0179: dest[j++] = 'Z'; break; // Ź
          case 0x017A: dest[j++] = 'z'; break; // ź
          case 0x017B: dest[j++] = 'Z'; break; // Ż
          case 0x017C: dest[j++] = 'z'; break; // ż
          default:     dest[j++] = '\xDB'; break; // CP437 full block █
        }
      } else if (c >= 0x80) {
        dest[j++] = '\xDB';  // CP437 full block █ (3/4-byte UTF-8)
        while (src[i+1] && ((unsigned char)src[i+1] & 0xC0) == 0x80)
          i++;  // skip continuation bytes
      }
    }
    dest[j] = 0;
  }
  
  // draw text with ellipsis if it exceeds max_width
  virtual void drawTextEllipsized(int x, int y, int max_width, const char* str) {
    char temp_str[256];  // reasonable buffer size
    size_t len = strlen(str);
    if (len >= sizeof(temp_str)) len = sizeof(temp_str) - 1;
    memcpy(temp_str, str, len);
    temp_str[len] = 0;
    
    if (getTextWidth(temp_str) <= max_width) {
      setCursor(x, y);
      print(temp_str);
      return;
    }
    
    // for variable-width fonts (GxEPD), add space after ellipsis
    // for fixed-width fonts (OLED), keep tight spacing to save precious characters
    const char* ellipsis;
    // use a simple heuristic: if 'i' and 'l' have different widths, it's variable-width
    int i_width = getTextWidth("i");
    int l_width = getTextWidth("l");
    if (i_width != l_width) {
      ellipsis = "... ";  // variable-width fonts: add space
    } else {
      ellipsis = "...";   // fixed-width fonts: no space
    }
    
    int ellipsis_width = getTextWidth(ellipsis);
    int str_len = strlen(temp_str);
    
    while (str_len > 0 && getTextWidth(temp_str) > max_width - ellipsis_width) {
      temp_str[--str_len] = 0;
    }
    strcat(temp_str, ellipsis);
    
    setCursor(x, y);
    print(temp_str);
  }
  
  virtual void endFrame() = 0;
};
