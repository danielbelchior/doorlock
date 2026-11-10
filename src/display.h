#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include "config.h"

enum class Layout : uint8_t { Message, Pin, Menu };
enum class Icon : uint8_t { None, Ok, Error };
enum class NetStatus : uint8_t { Offline, Online, Hotspot };

// One screen. The display always draws a header (title, clock, network icon),
// then a body that depends on the layout, then the footer if there is one.
//   Message: `text` in the largest font that fits, `detail` below it, `icon`
//            on the left.
//   Pin:     one filled dot per character of `text`; the digits never show.
//   Menu:    one row per '\n'-separated item of `text`, numbered from 1;
//            item `selected` (1-based, 0 = none) is highlighted.
// `footer` holds key hints as "<key> <label>" separated by '|', for example
// "# Entrar|* Apagar". Text must be ASCII: the fonts have no accents.
struct Screen {
  Layout layout = Layout::Message;
  Icon icon = Icon::None;
  String title;
  String text;
  String detail;
  String footer;
  uint8_t selected = 0;

  bool operator==(const Screen& other) const;
  bool operator!=(const Screen& other) const { return !(*this == other); }
};

Screen messageScreen(const String& title, const String& text, const String& detail = String(),
                     Icon icon = Icon::None);

// 0.96" 128x64 SSD1306 OLED on I2C, drawn with U8g2 into a full frame buffer.
// The frame is sent only when something visible changed. The header uses rows
// 0-15 only, so it fits the yellow band of two-color modules.
class Display {
 public:
  Display();

  void begin();
  void show(const Screen& screen);
  void setNetwork(NetStatus status);
  void setClock(const String& hhmm);  // empty hides the clock
  void setAsleep(bool asleep);        // panel off; the content is kept
  bool asleep() const { return asleep_; }

 private:
  void render();
  void drawHeader();
  uint8_t drawNetworkIcon();
  void drawMessage(uint8_t bottom);
  void drawPin(uint8_t bottom);
  void drawMenu();
  void drawFooter();
  void drawIcon(Icon icon, uint8_t centerY);
  const uint8_t* fitFont(const uint8_t* const* fonts, uint8_t count, const String& text,
                         uint8_t width);

  // For a 1.3" SH1106 module use U8G2_SH1106_128X64_NONAME_F_HW_I2C instead.
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled_;
  Screen screen_;
  NetStatus network_ = NetStatus::Offline;
  String clock_;
  bool asleep_ = false;
  bool started_ = false;
};
