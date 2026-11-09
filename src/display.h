#pragma once

#include <LiquidCrystal_I2C.h>

#include "config.h"

// 16x2 LCD that only rewrites lines that changed, so updates do not flicker.
class Display {
 public:
  Display();

  void begin();
  void show(const String& top, const String& bottom = String());

 private:
  void writeLine(uint8_t row, const String& text);

  LiquidCrystal_I2C lcd_;
  String lines_[DISPLAY_ROWS];
};
