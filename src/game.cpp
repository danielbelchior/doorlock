#include "game.h"

#include "util.h"

Game::Game(Display& display, StatusLed& led, Buzzer& buzzer, UserStore& users)
    : display_(display), led_(led), buzzer_(buzzer), users_(users) {}

void Game::begin() {
  resetToPrompt();
}

bool Game::isIdle() const {
  return (state_ == State::EnterPin && input_.length() == 0) || state_ == State::Notice;
}

void Game::update() {
  switch (state_) {
    case State::Greeting:
      if (timeReached(deadlineMs_)) startChallenge();
      break;
    case State::Result:
    case State::Notice:
      if (timeReached(deadlineMs_)) resetToPrompt();
      break;
    case State::EnterPin:
      if (input_.length() > 0 && timeReached(lastActivityMs_ + PIN_ENTRY_TIMEOUT_MS)) resetToPrompt();
      break;
    case State::Challenge:
      if (timeReached(lastActivityMs_ + CHALLENGE_TIMEOUT_MS)) resetToPrompt();
      break;
    case State::Menu:
      if (timeReached(lastActivityMs_ + MENU_TIMEOUT_MS)) resetToPrompt();
      break;
    case State::Busy:
      break;
  }
}

void Game::handleKey(char key) {
  lastActivityMs_ = millis();

  switch (state_) {
    case State::EnterPin:
      if (key == '#') {
        if (input_.length() > 0) submitPin();
      } else if (key == 'D') {
        if (input_ == MENU_CODE) openMenu();
      } else if (key == '*') {
        input_ = "";
        renderPrompt();
      } else if (appendDigit(key, USER_PIN_LEN)) {
        renderPrompt();
      }
      break;

    case State::Menu:
      if (key == '*') {
        resetToPrompt();
      } else if (key == 'D') {
        if (menuChoice_) runMenuChoice();
      } else if (key >= '1' && key <= '3') {
        menuChoice_ = key;
        renderMenu();
      }
      break;

    case State::Challenge:
      if (key == '#') {
        if (input_.length() > 0) submitAnswer();
      } else if (key == '*') {
        input_ = "";
        renderChallenge();
      } else if (appendDigit(key, ANSWER_MAX_LEN)) {
        renderChallenge();
      }
      break;

    case State::Notice:
      // Dismiss the notice; a digit also starts typing the PIN.
      resetToPrompt();
      if (appendDigit(key, USER_PIN_LEN)) renderPrompt();
      break;

    case State::Result:
      resetToPrompt();
      break;

    case State::Greeting:
    case State::Busy:
      break;
  }
}

void Game::showNotice(const String& top, const String& bottom, uint32_t durationMs) {
  if (state_ == State::Busy) return;
  input_ = "";
  display_.show(top, bottom);
  enterTimedState(State::Notice, durationMs);
}

void Game::showBusy(const String& top, const String& bottom) {
  input_ = "";
  state_ = State::Busy;
  display_.show(top, bottom);
}

void Game::endBusy() {
  if (state_ == State::Busy) resetToPrompt();
}

void Game::resetToPrompt() {
  input_ = "";
  userName_[0] = '\0';
  state_ = State::EnterPin;
  lastActivityMs_ = millis();
  renderPrompt();
}

void Game::renderPrompt() {
  String stars;
  for (size_t i = 0; i < input_.length(); i++) stars += '*';
  display_.show("Digite a senha:", stars);
}

void Game::renderChallenge() {
  display_.show(question_ + " = ?", String("> ") + input_);
}

void Game::submitPin() {
  const int index = users_.findByPin(input_);
  input_ = "";

  if (index >= 0) {
    const uint8_t userIndex = static_cast<uint8_t>(index);
    strncpy(userName_, users_.at(userIndex).name, USER_NAME_LEN);
    userName_[USER_NAME_LEN] = '\0';
    display_.show("Bem-vindo(a):", userName_);
    led_.blink(LED_GREEN);
    buzzer_.play(Sound::Success);
    if (users_.challenge(userIndex)) {
      enterTimedState(State::Greeting, GREETING_MS);
    } else {
      enterTimedState(State::Result, RESULT_MS);
    }
  } else {
    display_.show("ERRO", "Senha invalida");
    led_.blink(LED_RED);
    buzzer_.play(Sound::Error);
    enterTimedState(State::Result, RESULT_MS);
  }
}

void Game::startChallenge() {
  long a;
  long b;
  switch (random(3)) {
    case 0:
      a = random(100, 500);
      b = random(100, 500);
      answer_ = a + b;
      question_ = String(a) + " + " + String(b);
      break;
    case 1:  // b < a keeps the result positive
      a = random(200, 900);
      b = random(100, a);
      answer_ = a - b;
      question_ = String(a) + " - " + String(b);
      break;
    default:
      a = random(12, 100);
      b = random(2, 10);
      answer_ = a * b;
      question_ = String(a) + " x " + String(b);
      break;
  }

  input_ = "";
  state_ = State::Challenge;
  lastActivityMs_ = millis();
  renderChallenge();
}

void Game::submitAnswer() {
  const long given = input_.toInt();
  input_ = "";

  if (given == answer_) {
    display_.show("Correto! :)", userName_);
    led_.blink(LED_GREEN);
    buzzer_.play(Sound::Success);
  } else {
    display_.show("Errado! :(", String("Era: ") + String(answer_));
    led_.blink(LED_RED);
    buzzer_.play(Sound::Error);
  }
  enterTimedState(State::Result, RESULT_MS);
}

void Game::openMenu() {
  input_ = "";
  menuChoice_ = 0;
  state_ = State::Menu;
  led_.blink(LED_GREEN, 1);
  renderMenu();
}

void Game::renderMenu() {
  String top = "1Hotspot 2IP  >";
  if (menuChoice_) top += menuChoice_;
  display_.show(top, "3Reiniciar *Sair");
}

void Game::runMenuChoice() {
  const MenuAction action = static_cast<MenuAction>(menuChoice_ - '0');
  resetToPrompt();
  if (menuHandler_) menuHandler_(action);
}

void Game::enterTimedState(State state, uint32_t durationMs) {
  state_ = state;
  deadlineMs_ = millis() + durationMs;
}

bool Game::appendDigit(char key, uint8_t maxLength) {
  if (key < '0' || key > '9' || input_.length() >= maxLength) return false;
  input_ += key;
  return true;
}
