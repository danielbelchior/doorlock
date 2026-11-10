#pragma once

#include <Arduino.h>

#include <functional>

#include "buzzer.h"
#include "config.h"
#include "display.h"
#include "led.h"
#include "users.h"

// Values match the menu digits shown on the display.
enum class MenuAction : uint8_t { Hotspot = 1, ShowIp = 2, Restart = 3 };

// Keypad state machine: PIN entry -> greeting -> math challenge -> result
// (users without the challenge stop at the greeting), plus a service menu
// opened with MENU_CODE followed by D.
class Game {
 public:
  using MenuHandler = std::function<void(MenuAction)>;

  Game(Display& display, StatusLed& led, Buzzer& buzzer, UserStore& users);

  // Called after the menu closes, so the handler may show a notice.
  void onMenuAction(MenuHandler handler) { menuHandler_ = handler; }

  void begin();
  void update();
  void handleKey(char key);

  // True when nothing is being typed, so a notice can be shown safely.
  bool isIdle() const;

  // True at an empty PIN prompt with no key for at least ms.
  bool restingFor(uint32_t ms) const;

  // Temporary message; any key or the timeout returns to the PIN prompt.
  void showNotice(const Screen& screen, uint32_t durationMs);

  // Message that stays until endBusy(); keys are ignored meanwhile.
  void showBusy(const Screen& screen);
  void endBusy();

 private:
  enum class State : uint8_t { EnterPin, Greeting, Challenge, Result, Notice, Busy, Menu };

  void resetToPrompt();
  void renderPrompt();
  void renderChallenge();
  void submitPin();
  void startChallenge();
  void submitAnswer();
  void openMenu();
  void renderMenu();
  void runMenuChoice();
  void enterTimedState(State state, uint32_t durationMs);
  bool appendDigit(char key, uint8_t maxLength);

  Display& display_;
  StatusLed& led_;
  Buzzer& buzzer_;
  UserStore& users_;
  MenuHandler menuHandler_;

  State state_ = State::EnterPin;
  String input_;
  String question_;
  long answer_ = 0;
  char menuChoice_ = 0;  // '1'..'3' once a digit is typed in the menu
  // Copied at login so the result screen survives a user being edited or
  // removed from the web UI mid-session.
  char userName_[USER_NAME_LEN + 1] = {};
  uint32_t deadlineMs_ = 0;
  uint32_t lastActivityMs_ = 0;
};
