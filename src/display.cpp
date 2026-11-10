#include "display.h"

namespace {

constexpr uint8_t WIDTH = 128;
constexpr uint8_t HEIGHT = 64;
constexpr uint8_t HEADER_BASELINE = 10;
constexpr uint8_t HEADER_LINE_Y = 13;
// Row 16 is the first blue row on two-color modules; nothing may straddle it.
constexpr uint8_t BODY_TOP = 16;
constexpr uint8_t FOOTER_TOP = 55;
constexpr uint8_t FOOTER_BASELINE = 62;
constexpr uint8_t ICON_RADIUS = 10;
constexpr uint8_t ICON_AREA = 28;  // text starts here when there is an icon
constexpr uint8_t MENU_ROW_HEIGHT = 12;
constexpr uint8_t PIN_DOT_PITCH = 13;

const uint8_t* const TEXT_FONTS[] = {u8g2_font_helvB14_tr, u8g2_font_helvB12_tr,
                                     u8g2_font_helvB10_tr, u8g2_font_helvB08_tr};
const uint8_t* const DETAIL_FONTS[] = {u8g2_font_helvR10_tr, u8g2_font_helvR08_tr};
const uint8_t* const TITLE_FONT = u8g2_font_helvB08_tr;
const uint8_t* const MENU_FONT = u8g2_font_helvR08_tr;
const uint8_t* const SMALL_FONT = u8g2_font_5x8_tr;
constexpr uint8_t TEXT_FONT_COUNT = sizeof(TEXT_FONTS) / sizeof(TEXT_FONTS[0]);
constexpr uint8_t DETAIL_FONT_COUNT = sizeof(DETAIL_FONTS) / sizeof(DETAIL_FONTS[0]);

}  // namespace

bool Screen::operator==(const Screen& other) const {
  return layout == other.layout && icon == other.icon && selected == other.selected &&
         title == other.title && text == other.text && detail == other.detail &&
         footer == other.footer;
}

Screen messageScreen(const String& title, const String& text, const String& detail, Icon icon) {
  Screen screen;
  screen.icon = icon;
  screen.title = title;
  screen.text = text;
  screen.detail = detail;
  return screen;
}

Display::Display()
    : oled_(DISPLAY_ROTATE_180 ? U8G2_R2 : U8G2_R0, U8X8_PIN_NONE, PIN_I2C_SCL, PIN_I2C_SDA) {}

void Display::begin() {
  oled_.setI2CAddress(DISPLAY_I2C_ADDRESS << 1);  // U8g2 expects the 8-bit form
  oled_.setBusClock(DISPLAY_I2C_CLOCK_HZ);
  oled_.begin();  // also starts Wire on PIN_I2C_SDA / PIN_I2C_SCL
  oled_.setContrast(DISPLAY_CONTRAST);
  oled_.setFontMode(1);
  started_ = true;
  render();
}

void Display::show(const Screen& screen) {
  if (screen == screen_) return;
  screen_ = screen;
  render();
}

void Display::setNetwork(NetStatus status) {
  if (status == network_) return;
  network_ = status;
  render();
}

void Display::setClock(const String& hhmm) {
  if (hhmm == clock_) return;
  clock_ = hhmm;
  render();
}

void Display::setAsleep(bool asleep) {
  if (asleep == asleep_) return;
  asleep_ = asleep;
  if (started_) oled_.setPowerSave(asleep ? 1 : 0);
}

void Display::render() {
  if (!started_) return;
  const bool hasFooter = screen_.footer.length() > 0;
  const uint8_t bottom = hasFooter ? FOOTER_TOP - 1 : HEIGHT;

  oled_.clearBuffer();
  oled_.setDrawColor(1);
  drawHeader();
  switch (screen_.layout) {
    case Layout::Message:
      drawMessage(bottom);
      break;
    case Layout::Pin:
      drawPin(bottom);
      break;
    case Layout::Menu:
      drawMenu();
      break;
  }
  if (hasFooter) drawFooter();
  oled_.sendBuffer();
}

void Display::drawHeader() {
  oled_.setFont(TITLE_FONT);
  oled_.drawStr(0, HEADER_BASELINE, screen_.title.c_str());

  const uint8_t iconWidth = drawNetworkIcon();
  if (clock_.length() > 0) {
    oled_.setFont(SMALL_FONT);
    const uint8_t x = WIDTH - iconWidth - 4 - oled_.getStrWidth(clock_.c_str());
    oled_.drawStr(x, HEADER_BASELINE - 1, clock_.c_str());
  }
  oled_.drawHLine(0, HEADER_LINE_Y, WIDTH);
}

// Draws at the right edge of the header and returns the width used.
uint8_t Display::drawNetworkIcon() {
  if (network_ == NetStatus::Hotspot) {
    constexpr uint8_t w = 15;
    oled_.drawRBox(WIDTH - w, 1, w, 10, 2);
    oled_.setDrawColor(0);
    oled_.setFont(SMALL_FONT);
    oled_.drawStr(WIDTH - w + 3, 9, "AP");
    oled_.setDrawColor(1);
    return w;
  }

  // Three signal bars, filled when connected, outlined when not.
  constexpr uint8_t barWidth = 3;
  constexpr uint8_t w = 3 * barWidth + 2;
  for (uint8_t i = 0; i < 3; i++) {
    const uint8_t h = 3 + 3 * i;
    const uint8_t x = WIDTH - w + i * (barWidth + 1);
    const uint8_t y = 11 - h;
    if (network_ == NetStatus::Online) {
      oled_.drawBox(x, y, barWidth, h);
    } else {
      oled_.drawFrame(x, y, barWidth, h);
    }
  }
  if (network_ == NetStatus::Offline) {
    oled_.drawLine(WIDTH - w - 1, 1, WIDTH - 1, 11);
  }
  return w;
}

const uint8_t* Display::fitFont(const uint8_t* const* fonts, uint8_t count, const String& text,
                                uint8_t width) {
  for (uint8_t i = 0; i < count; i++) {
    oled_.setFont(fonts[i]);
    if (oled_.getStrWidth(text.c_str()) <= width) return fonts[i];
  }
  return fonts[count - 1];  // still too wide: the end is clipped
}

void Display::drawMessage(uint8_t bottom) {
  const bool hasIcon = screen_.icon != Icon::None;
  const bool hasDetail = screen_.detail.length() > 0;
  const uint8_t left = hasIcon ? ICON_AREA : 0;
  const uint8_t width = WIDTH - left;
  constexpr uint8_t gap = 5;

  const uint8_t* textFont = fitFont(TEXT_FONTS, TEXT_FONT_COUNT, screen_.text, width);
  oled_.setFont(textFont);
  const int textAscent = oled_.getAscent();

  const uint8_t* detailFont = nullptr;
  int detailAscent = 0;
  if (hasDetail) {
    detailFont = fitFont(DETAIL_FONTS, DETAIL_FONT_COUNT, screen_.detail, width);
    detailAscent = oled_.getAscent();
  }

  const int blockHeight = textAscent + (hasDetail ? gap + detailAscent : 0);
  const int top = BODY_TOP + (bottom - BODY_TOP - blockHeight) / 2;
  const int textBaseline = top + textAscent;

  oled_.setFont(textFont);
  const int textX = hasIcon ? left : (WIDTH - oled_.getStrWidth(screen_.text.c_str())) / 2;
  oled_.drawStr(textX, textBaseline, screen_.text.c_str());

  if (hasDetail) {
    oled_.setFont(detailFont);
    const int detailX = hasIcon ? left : (WIDTH - oled_.getStrWidth(screen_.detail.c_str())) / 2;
    oled_.drawStr(detailX, textBaseline + gap + detailAscent, screen_.detail.c_str());
  }

  if (hasIcon) drawIcon(screen_.icon, top + blockHeight / 2);
}

void Display::drawIcon(Icon icon, uint8_t centerY) {
  constexpr uint8_t cx = ICON_RADIUS;
  oled_.drawDisc(cx, centerY, ICON_RADIUS);
  oled_.setDrawColor(0);
  for (uint8_t t = 0; t < 2; t++) {  // two passes make 2 px strokes
    if (icon == Icon::Ok) {
      oled_.drawLine(cx - 5, centerY + t, cx - 2, centerY + 3 + t);
      oled_.drawLine(cx - 2, centerY + 3 + t, cx + 5, centerY - 4 + t);
    } else {
      oled_.drawLine(cx - 4 + t, centerY - 4, cx + 4 + t, centerY + 4);
      oled_.drawLine(cx + 4 + t, centerY - 4, cx - 4 + t, centerY + 4);
    }
  }
  oled_.setDrawColor(1);
}

void Display::drawPin(uint8_t bottom) {
  const uint8_t cy = BODY_TOP + (bottom - BODY_TOP) / 2;
  const uint8_t typed =
      screen_.text.length() < USER_PIN_LEN ? screen_.text.length() : USER_PIN_LEN;
  const uint8_t x0 = (WIDTH - (USER_PIN_LEN - 1) * PIN_DOT_PITCH) / 2;
  for (uint8_t i = 0; i < USER_PIN_LEN; i++) {
    const uint8_t x = x0 + i * PIN_DOT_PITCH;
    if (i < typed) {
      oled_.drawDisc(x, cy, 4);
    } else {
      oled_.drawDisc(x, cy, 1);
    }
  }
}

void Display::drawMenu() {
  oled_.setFont(MENU_FONT);
  uint8_t row = 0;
  int start = 0;
  while (start <= static_cast<int>(screen_.text.length())) {
    int end = screen_.text.indexOf('\n', start);
    if (end < 0) end = screen_.text.length();
    const String item = screen_.text.substring(start, end);
    const uint8_t y = BODY_TOP + 1 + row * MENU_ROW_HEIGHT;
    const bool selected = screen_.selected == row + 1;

    if (selected) {
      oled_.drawRBox(0, y, WIDTH, MENU_ROW_HEIGHT, 2);
      oled_.setDrawColor(0);
    }
    const char number[2] = {static_cast<char>('1' + row), '\0'};
    oled_.setFont(TITLE_FONT);
    oled_.drawStr(4, y + 10, number);
    oled_.setFont(MENU_FONT);
    oled_.drawStr(16, y + 10, item.c_str());
    oled_.setDrawColor(1);

    row++;
    start = end + 1;
  }
}

void Display::drawFooter() {
  constexpr uint8_t keyWidth = 7;
  constexpr uint8_t keyGap = 2;
  constexpr uint8_t maxHints = 4;
  String labels[maxHints];
  char keys[maxHints];
  uint8_t count = 0;
  uint16_t total = 0;

  oled_.setFont(SMALL_FONT);
  int start = 0;
  while (count < maxHints && start < static_cast<int>(screen_.footer.length())) {
    int end = screen_.footer.indexOf('|', start);
    if (end < 0) end = screen_.footer.length();
    keys[count] = screen_.footer[start];
    labels[count] = screen_.footer.substring(start + 2, end);
    total += keyWidth + keyGap + oled_.getStrWidth(labels[count].c_str());
    count++;
    start = end + 1;
  }
  if (count == 0) return;

  const int space = total < WIDTH ? (WIDTH - total) / (count + 1) : 0;
  int x = space;
  for (uint8_t i = 0; i < count; i++) {
    oled_.drawRBox(x, FOOTER_TOP, keyWidth, HEIGHT - FOOTER_TOP, 1);
    oled_.setDrawColor(0);
    const char key[2] = {keys[i], '\0'};
    oled_.drawStr(x + 1, FOOTER_BASELINE, key);
    oled_.setDrawColor(1);
    x += keyWidth + keyGap;
    oled_.drawStr(x, FOOTER_BASELINE, labels[i].c_str());
    x += oled_.getStrWidth(labels[i].c_str()) + space;
  }
}
