#pragma once

#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "led.h"
#include "users.h"

// Keypad state machine: PIN entry -> greeting -> math challenge -> result.
class Game {
 public:
  Game(Display& display, StatusLed& led, UserStore& users);

  void begin();
  void update();
  void handleKey(char key);

  // True when nothing is being typed, so a notice can be shown safely.
  bool isIdle() const;

  // Temporary message; any key or the timeout returns to the PIN prompt.
  void showNotice(const String& top, const String& bottom, uint32_t durationMs);

  // Message that stays until endBusy(); keys are ignored meanwhile.
  void showBusy(const String& top, const String& bottom);
  void endBusy();

 private:
  enum class State : uint8_t { EnterPin, Greeting, Challenge, Result, Notice, Busy };

  void resetToPrompt();
  void renderPrompt();
  void renderChallenge();
  void submitPin();
  void startChallenge();
  void submitAnswer();
  void enterTimedState(State state, uint32_t durationMs);
  bool appendDigit(char key, uint8_t maxLength);

  Display& display_;
  StatusLed& led_;
  UserStore& users_;

  State state_ = State::EnterPin;
  String input_;
  String question_;
  long answer_ = 0;
  // Copied at login so the result screen survives a user being edited or
  // removed from the web UI mid-session.
  char userName_[USER_NAME_LEN + 1] = {};
  uint32_t deadlineMs_ = 0;
  uint32_t lastActivityMs_ = 0;
};
