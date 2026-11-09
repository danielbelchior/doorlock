#include "buzzer.h"

#include "config.h"
#include "util.h"

namespace {

// The MH-FMD switches the buzzer on when I/O is LOW, so silence holds the pin
// HIGH.
constexpr uint8_t BUZZER_ON_LEVEL = BUZZER_ACTIVE_LOW ? LOW : HIGH;
constexpr uint8_t BUZZER_OFF_LEVEL = BUZZER_ACTIVE_LOW ? HIGH : LOW;

// Patterns as {buzzer on, duration in ms}.
const Buzzer::Step KEY_STEPS[] = {{true, BUZZER_KEY_MS}};
const Buzzer::Step SUCCESS_STEPS[] = {{true, 70}, {false, 70}, {true, 70}};
const Buzzer::Step ERROR_STEPS[] = {{true, 600}};

}  // namespace

void Buzzer::begin() {
  // Write the idle level before switching to output, so the buzzer does not
  // chirp at boot.
  digitalWrite(PIN_BUZZER, BUZZER_OFF_LEVEL);
  pinMode(PIN_BUZZER, OUTPUT);
  set(false);
}

void Buzzer::play(Sound sound) {
  if (sound == Sound::Key && pattern_) return;

  switch (sound) {
    case Sound::Key:
      steps_ = KEY_STEPS;
      stepCount_ = sizeof(KEY_STEPS) / sizeof(KEY_STEPS[0]);
      break;
    case Sound::Success:
      steps_ = SUCCESS_STEPS;
      stepCount_ = sizeof(SUCCESS_STEPS) / sizeof(SUCCESS_STEPS[0]);
      break;
    case Sound::Error:
      steps_ = ERROR_STEPS;
      stepCount_ = sizeof(ERROR_STEPS) / sizeof(ERROR_STEPS[0]);
      break;
  }
  pattern_ = sound != Sound::Key;
  stepIndex_ = 0;
  startStep(steps_[0]);
}

void Buzzer::stop() {
  steps_ = nullptr;
  pattern_ = false;
  set(false);
}

void Buzzer::update() {
  if (!steps_ || !timeReached(nextMs_)) return;

  if (++stepIndex_ < stepCount_) {
    startStep(steps_[stepIndex_]);
    return;
  }
  stop();
}

void Buzzer::startStep(const Step& step) {
  nextMs_ = millis() + step.ms;
  set(step.on);
}

void Buzzer::set(bool on) {
  digitalWrite(PIN_BUZZER, on ? BUZZER_ON_LEVEL : BUZZER_OFF_LEVEL);
}
