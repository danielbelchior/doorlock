#pragma once

#include <Adafruit_NeoPixel.h>

#include "config.h"

constexpr uint32_t ledColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0) {
  return (static_cast<uint32_t>(w) << 24) | (static_cast<uint32_t>(r) << 16) |
         (static_cast<uint32_t>(g) << 8) | b;
}

constexpr uint32_t LED_OFF = 0;
// Full green in a "yellow" looks green on the SK6812, so red dominates.
constexpr uint32_t LED_YELLOW = ledColor(255, 160, 0);
constexpr uint32_t LED_GREEN = ledColor(0, 255, 0);
constexpr uint32_t LED_RED = ledColor(255, 0, 0);

// Single SK6812 pixel driven without blocking: call update() every loop.
class StatusLed {
 public:
  StatusLed();

  void begin();
  void update();

  // Short yellow flash. Ignored while a blink is running so a key press
  // cannot cut the green/red result feedback short.
  void keyFlash();
  void blink(uint32_t color, uint8_t times = LED_BLINK_COUNT);

 private:
  enum class Mode : uint8_t { Idle, KeyFlash, Blink };

  void set(uint32_t color);

  Adafruit_NeoPixel pixel_;
  Mode mode_;
  uint32_t color_;
  uint8_t phasesLeft_;
  uint32_t nextMs_;
};
