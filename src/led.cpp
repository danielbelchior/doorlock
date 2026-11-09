#include "led.h"

#include "util.h"

StatusLed::StatusLed()
    : pixel_(1, PIN_LED, NEO_GRBW + NEO_KHZ800),
      mode_(Mode::Idle),
      color_(LED_OFF),
      phasesLeft_(0),
      nextMs_(0) {}

void StatusLed::begin() {
  pixel_.begin();
  pixel_.setBrightness(LED_BRIGHTNESS);
  set(LED_OFF);
}

void StatusLed::keyFlash() {
  if (mode_ == Mode::Blink) return;
  mode_ = Mode::KeyFlash;
  set(LED_YELLOW);
  nextMs_ = millis() + LED_KEY_FLASH_MS;
}

void StatusLed::blink(uint32_t color, uint8_t times) {
  if (times == 0) return;
  mode_ = Mode::Blink;
  color_ = color;
  phasesLeft_ = times * 2;  // one "on" and one "off" phase per blink
  set(color);
  nextMs_ = millis() + LED_BLINK_MS;
}

void StatusLed::update() {
  if (mode_ == Mode::Idle || !timeReached(nextMs_)) return;

  if (mode_ == Mode::Blink && --phasesLeft_ > 0) {
    set(phasesLeft_ % 2 == 0 ? color_ : LED_OFF);
    nextMs_ = millis() + LED_BLINK_MS;
    return;
  }

  set(LED_OFF);
  mode_ = Mode::Idle;
}

void StatusLed::set(uint32_t color) {
  pixel_.setPixelColor(0, color);
  pixel_.show();
}
