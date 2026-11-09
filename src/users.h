#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "config.h"

// Stored as a raw blob; the layout must stay identical to v1 so existing
// users survive the firmware upgrade. Per-user options live in separate
// keys (see UserStore) for the same reason.
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
  // Whether the user must answer the math challenge after logging in.
  bool challenge(uint8_t index) const { return challenge_[index] != 0; }
  int findByPin(const String& pin) const;

  UserResult add(String name, String pin, bool withChallenge);
  UserResult update(int index, String name, String pin, bool withChallenge);
  UserResult remove(int index);

 private:
  UserResult validate(String& name, String& pin, int ignoreIndex) const;
  void seedDefaults();
  bool save();

  Preferences prefs_;
  User users_[MAX_USERS];
  uint8_t challenge_[MAX_USERS];  // 1 = math challenge on, parallel to users_
  uint8_t count_ = 0;
};
