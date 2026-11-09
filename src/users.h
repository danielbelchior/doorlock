#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "config.h"

// Stored as a raw blob; the layout must stay identical to v1 so existing
// users survive the firmware upgrade.
struct User {
  char name[USER_NAME_LEN + 1];
  char pin[USER_PIN_LEN + 1];
};

enum class UserResult : uint8_t {
  Ok,
  InvalidName,
  InvalidPin,
  DuplicatePin,
  Full,
  NotFound,
  StorageError,
};

// Human readable (Portuguese) message for the web UI.
const char* userResultMessage(UserResult result);

class UserStore {
 public:
  void begin();

  uint8_t count() const { return count_; }
  const User& at(uint8_t index) const { return users_[index]; }
  int findByPin(const String& pin) const;

  UserResult add(String name, String pin);
  UserResult update(int index, String name, String pin);
  UserResult remove(int index);

 private:
  UserResult validate(String& name, String& pin, int ignoreIndex) const;
  void seedDefaults();
  bool save();

  Preferences prefs_;
  User users_[MAX_USERS];
  uint8_t count_ = 0;
};
