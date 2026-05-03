#include <Arduino.h>
#include <Wire.h>
#include "FaketecHandheldBoard.h"

void FaketecHandheldBoard::begin() {
  NRF52BoardDCDC::begin();

  pinMode(PIN_VBAT_READ, INPUT);

  // Joystick and back button — active-low, internal pull-up
  pinMode(PIN_USER_BTN,   INPUT_PULLUP);
  pinMode(JOYSTICK_UP,    INPUT_PULLUP);
  pinMode(JOYSTICK_DOWN,  INPUT_PULLUP);
  pinMode(JOYSTICK_LEFT,  INPUT_PULLUP);
  pinMode(JOYSTICK_RIGHT, INPUT_PULLUP);
  pinMode(PIN_BACK_BTN,   INPUT_PULLUP);

#if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
  Wire.setPins(PIN_BOARD_SDA, PIN_BOARD_SCL);
#endif
  Wire.begin();

  // Power up SX1262 + SSD1306 via shared load switch
  pinMode(SX126X_POWER_EN, OUTPUT);
  digitalWrite(SX126X_POWER_EN, HIGH);
  delay(150);  // allow radio and display to stabilize on cold battery boot
}
