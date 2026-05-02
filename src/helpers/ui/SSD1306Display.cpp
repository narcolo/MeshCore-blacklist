#include "SSD1306Display.h"

// Polish diacritic glyphs: column-major, 5 bytes, LSB=top row, bit7=spacing row below letter.
// Designed from the Adafruit 5x7 CP437 font with diacritic marks added.
static const struct { uint16_t cp; uint8_t bmp[5]; } POLISH_GLYPHS[] = {
  // lowercase
  { 0x00F3, {0x38, 0x46, 0x45, 0x44, 0x38} }, // ó  (o + acute at rows 0-1 of col1-2)
  { 0x0105, {0x20, 0x54, 0x54, 0xD4, 0x78} }, // ą  (a + ogonek pixel in spacing row at col3)
  { 0x0107, {0x38, 0x46, 0x45, 0x44, 0x20} }, // ć  (c + acute at rows 0-1 of col1-2)
  { 0x0119, {0x38, 0x54, 0x54, 0xD4, 0x18} }, // ę  (e + ogonek pixel in spacing row at col3)
  { 0x0142, {0x00, 0x49, 0x7F, 0x48, 0x00} }, // ł  (l + horizontal stroke at row3 on col1+col3)
  { 0x0144, {0x7C, 0x0A, 0x05, 0x04, 0x78} }, // ń  (n + acute at rows 0-1 of col1-2)
  { 0x015B, {0x48, 0x56, 0x55, 0x54, 0x20} }, // ś  (s + acute at rows 0-1 of col1-2)
  { 0x017A, {0x44, 0x66, 0x55, 0x4C, 0x44} }, // ź  (z + acute at rows 0-1 of col1-2)
  { 0x017C, {0x44, 0x64, 0x55, 0x4C, 0x44} }, // ż  (z + dot at row0 of col2)
  // uppercase
  { 0x00D3, {0x3E, 0x41, 0xC1, 0xC1, 0x3E} }, // Ó  (O + 2-px acute in spacing row at col2-3)
  { 0x0104, {0x7C, 0x12, 0x11, 0x12, 0xFC} }, // Ą  (A + ogonek pixel in spacing row at col4)
  { 0x0106, {0x3E, 0x41, 0xC1, 0xC1, 0x22} }, // Ć  (C + 2-px acute in spacing row at col2-3)
  { 0x0118, {0x7F, 0x49, 0x49, 0x49, 0xC1} }, // Ę  (E + ogonek pixel in spacing row at col4)
  { 0x0141, {0x7F, 0x48, 0x40, 0x40, 0x40} }, // Ł  (L + stroke pixel at row3 of col1)
  { 0x0143, {0x7F, 0x04, 0x88, 0x90, 0x7F} }, // Ń  (N + 2-px acute in spacing row at col2-3)
  { 0x015A, {0x46, 0x49, 0xC9, 0xC9, 0x31} }, // Ś  (S + 2-px acute in spacing row at col2-3)
  { 0x0179, {0x61, 0x51, 0xC9, 0xC5, 0x43} }, // Ź  (Z + 2-px acute in spacing row at col2-3)
  { 0x017B, {0x61, 0x51, 0xC9, 0x45, 0x43} }, // Ż  (Z + dot in spacing row at col2)
};
static const int POLISH_GLYPH_COUNT = (int)(sizeof(POLISH_GLYPHS) / sizeof(POLISH_GLYPHS[0]));

const uint8_t* SSD1306Display::findPolishGlyph(uint16_t cp) {
  for (int i = 0; i < POLISH_GLYPH_COUNT; i++) {
    if (POLISH_GLYPHS[i].cp == cp) return POLISH_GLYPHS[i].bmp;
  }
  return NULL;
}

void SSD1306Display::drawPolishChar(const uint8_t* bitmap) {
  int x = display.getCursorX();
  int y = display.getCursorY();
  for (int col = 0; col < 5; col++) {
    uint8_t colbits = bitmap[col];
    for (int row = 0; row < 8; row++) {
      if (colbits & (1 << row)) {
        display.fillRect(x + col * _textsize, y + row * _textsize, _textsize, _textsize, _color);
      }
    }
  }
  display.setCursor(x + 6 * _textsize, y);
}

bool SSD1306Display::i2c_probe(TwoWire& wire, uint8_t addr) {
  wire.beginTransmission(addr);
  uint8_t error = wire.endTransmission();
  return (error == 0);
}

bool SSD1306Display::begin() {
  if (!_isOn) {
    if (_peripher_power) _peripher_power->claim();
    _isOn = true;
  }
  #ifdef DISPLAY_ROTATION
  display.setRotation(DISPLAY_ROTATION);
  #endif
  return display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS, true, false) && i2c_probe(Wire, DISPLAY_ADDRESS);
}

void SSD1306Display::turnOn() {
  if (!_isOn) {
    if (_peripher_power) _peripher_power->claim();
    _isOn = true;  // set before begin() to prevent double claim
    if (_peripher_power) begin();  // re-init display after power was cut
  }
  display.ssd1306_command(SSD1306_DISPLAYON);
}

void SSD1306Display::turnOff() {
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  if (_isOn) {
    if (_peripher_power) {
#if PIN_OLED_RESET >= 0
      digitalWrite(PIN_OLED_RESET, LOW);
#endif
      _peripher_power->release();
    }
    _isOn = false;
  }
}

void SSD1306Display::clear() {
  display.clearDisplay();
  display.display();
}

void SSD1306Display::startFrame(Color bkg) {
  display.clearDisplay();  // TODO: apply 'bkg'
  _color = SSD1306_WHITE;
  display.setTextColor(_color);
  display.setTextSize(1);
  _textsize = 1;
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
}

void SSD1306Display::setTextSize(int sz) {
  _textsize = sz;
  display.setTextSize(sz);
}

void SSD1306Display::setColor(Color c) {
  _color = (c != 0) ? SSD1306_WHITE : SSD1306_BLACK;
  display.setTextColor(_color);
}

void SSD1306Display::setCursor(int x, int y) {
  display.setCursor(x, y);
}

void SSD1306Display::translateUTF8ToBlocks(char* dest, const char* src, size_t dest_size) {
  // Passthrough: SSD1306Display::print() handles UTF-8 Polish chars natively
  strncpy(dest, src, dest_size - 1);
  dest[dest_size - 1] = 0;
}

void SSD1306Display::print(const char* str) {
  const char* s = str;
  while (*s) {
    unsigned char c = (unsigned char)*s;
    if (c >= 0xC2 && c <= 0xDF && s[1]) {
      uint16_t cp = ((uint16_t)(c & 0x1F) << 6) | ((unsigned char)s[1] & 0x3F);
      s += 2;
      const uint8_t* bmp = findPolishGlyph(cp);
      if (bmp) {
        drawPolishChar(bmp);
      } else {
        display.write('\xDB');  // CP437 full block for unrecognised
      }
    } else {
      display.write(c);
      s++;
    }
  }
}

void SSD1306Display::fillRect(int x, int y, int w, int h) {
  display.fillRect(x, y, w, h, _color);
}

void SSD1306Display::drawRect(int x, int y, int w, int h) {
  display.drawRect(x, y, w, h, _color);
}

void SSD1306Display::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  display.drawBitmap(x, y, bits, w, h, SSD1306_WHITE);
}

uint16_t SSD1306Display::getTextWidth(const char* str) {
  // Count UTF-8 characters (not bytes) — font is fixed-width 6*textsize px per char
  int char_count = 0;
  for (const char* p = str; *p; p++) {
    if (((unsigned char)*p & 0xC0) != 0x80)  // skip UTF-8 continuation bytes
      char_count++;
  }
  return (uint16_t)(char_count * 6 * _textsize);
}

void SSD1306Display::endFrame() {
  display.display();
}
