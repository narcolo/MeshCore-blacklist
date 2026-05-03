#pragma once

#include <MeshCore.h>
#include <Arduino.h>
#include <helpers/NRF52Board.h>

// LoRa (HT-RA62 / SX1262) — same physical wiring as ProMicro variant
#define P_LORA_NSS      13
#define P_LORA_DIO_1    11
#define P_LORA_RESET    10
#define P_LORA_BUSY     16
#define P_LORA_MISO     15
#define P_LORA_SCLK     12
#define P_LORA_MOSI     14
#define SX126X_POWER_EN 21   // shared load switch: powers radio + display
#define SX126X_RXEN      2
#define SX126X_TXEN     RADIOLIB_NC
#define SX126X_DIO2_AS_RF_SWITCH  true
#define SX126X_DIO3_TCXO_VOLTAGE (1.8f)

// Battery ADC — 1M/1M voltage divider → scale x2; calibrate against multimeter
#define PIN_VBAT_READ   17
#define ADC_MULTIPLIER  (2.0f)

// I2C pins defined in platformio.ini: PIN_BOARD_SDA=8, PIN_BOARD_SCL=7

// 5-way joystick + back button
// IMPORTANT: verify all pin numbers with multimeter before flashing
#define PIN_USER_BTN    6    // center / confirm
#define JOYSTICK_UP     5
#define JOYSTICK_DOWN   3
#define JOYSTICK_LEFT   4
#define JOYSTICK_RIGHT  9
#define PIN_BACK_BTN   20

class FaketecHandheldBoard : public NRF52BoardDCDC {
  float adc_mult = ADC_MULTIPLIER;

public:
  FaketecHandheldBoard() : NRF52Board("Faketec_OTA") {}
  void begin();

  uint16_t getBattMilliVolts() override {
    analogReadResolution(12);
    uint32_t raw = 0;
    for (int i = 0; i < 8; i++) raw += analogRead(PIN_VBAT_READ);
    return (uint16_t)(adc_mult * (raw / 8));
  }

  bool setAdcMultiplier(float m) override {
    adc_mult = (m == 0.0f) ? ADC_MULTIPLIER : m;
    return true;
  }
  float getAdcMultiplier() const override {
    return (adc_mult == 0.0f) ? ADC_MULTIPLIER : adc_mult;
  }

  const char* getManufacturerName() const override {
    return "Faketec Handheld DIY";
  }

  void powerOff() override { sd_power_system_off(); }
};
