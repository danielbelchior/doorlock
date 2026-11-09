#include "display.h"

Display::Display() : lcd_(DISPLAY_I2C_ADDRESS, DISPLAY_COLS, DISPLAY_ROWS) {}

void Display::begin() {
  lcd_.init();
  lcd_.backlight();
  lcd_.clear();
}

void Display::show(const String& top, const String& bottom) {
  writeLine(0, top);
  writeLine(1, bottom);
}

void Display::writeLine(uint8_t row, const String& text) {
  String padded = text.substring(0, DISPLAY_COLS);
  while (padded.length() < DISPLAY_COLS) padded += ' ';
  if (padded == lines_[row]) return;

  lines_[row] = padded;
  lcd_.setCursor(0, row);
  lcd_.print(padded);
}
