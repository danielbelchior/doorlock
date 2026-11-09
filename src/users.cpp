#include "users.h"

namespace {

// Same namespace and keys as v1 so stored users are kept after the upgrade.
constexpr const char* NVS_NAMESPACE = "keypad";
constexpr const char* KEY_COUNT = "count";
constexpr const char* KEY_USERS = "users";
// Added in v2.1. Missing (older firmware) means every user keeps the challenge.
constexpr const char* KEY_CHALLENGE = "challenge";

bool isValidName(const String& name) {
  if (name.length() < 1 || name.length() > USER_NAME_LEN) return false;
  for (size_t i = 0; i < name.length(); i++) {
    const uint8_t c = static_cast<uint8_t>(name[i]);
    if (c < 32 || c > 126) return false;  // the LCD only shows ASCII
  }
  return true;
}

bool isValidPin(const String& pin) {
  if (pin.length() < 1 || pin.length() > USER_PIN_LEN) return false;
  for (size_t i = 0; i < pin.length(); i++) {
    if (pin[i] < '0' || pin[i] > '9') return false;
  }
  return true;
}

void copyField(char* dest, size_t size, const String& src) {
  strncpy(dest, src.c_str(), size - 1);
  dest[size - 1] = '\0';
}

}  // namespace

const char* userResultMessage(UserResult result) {
  switch (result) {
    case UserResult::Ok:
      return "OK";
    case UserResult::InvalidName:
      return "Nome inválido: use 1 a 16 caracteres, sem acentos.";
    case UserResult::InvalidPin:
      return "Senha inválida: use 1 a 8 dígitos.";
    case UserResult::DuplicatePin:
      return "Essa senha já está em uso por outro usuário.";
    case UserResult::Full:
      return "A lista de usuários está cheia.";
    case UserResult::NotFound:
      return "Usuário não encontrado. Recarregue a página.";
    case UserResult::StorageError:
      return "Erro ao gravar na memória do dispositivo.";
  }
  return "Erro desconhecido.";
}

void UserStore::begin() {
  memset(users_, 0, sizeof(users_));
  memset(challenge_, 1, sizeof(challenge_));
  prefs_.begin(NVS_NAMESPACE, false);

  if (!prefs_.isKey(KEY_COUNT)) {
    seedDefaults();
    return;
  }

  const uint8_t stored = prefs_.getUChar(KEY_COUNT, 0);
  if (stored > 0) {
    if (stored > MAX_USERS || prefs_.getBytesLength(KEY_USERS) != sizeof(users_)) {
      Serial.println("[users] stored list does not match this firmware, restoring defaults");
      seedDefaults();
      return;
    }
    prefs_.getBytes(KEY_USERS, users_, sizeof(users_));
    if (prefs_.isKey(KEY_CHALLENGE) && prefs_.getBytesLength(KEY_CHALLENGE) == sizeof(challenge_)) {
      prefs_.getBytes(KEY_CHALLENGE, challenge_, sizeof(challenge_));
    }
  }
  count_ = stored;

  for (uint8_t i = 0; i < count_; i++) {
    users_[i].name[USER_NAME_LEN] = '\0';
    users_[i].pin[USER_PIN_LEN] = '\0';
  }
  Serial.printf("[users] %u user(s) loaded\n", count_);
}

int UserStore::findByPin(const String& pin) const {
  for (uint8_t i = 0; i < count_; i++) {
    if (strcmp(pin.c_str(), users_[i].pin) == 0) return i;
  }
  return -1;
}

UserResult UserStore::add(String name, String pin, bool withChallenge) {
  if (count_ >= MAX_USERS) return UserResult::Full;
  const UserResult check = validate(name, pin, -1);
  if (check != UserResult::Ok) return check;

  copyField(users_[count_].name, sizeof(users_[count_].name), name);
  copyField(users_[count_].pin, sizeof(users_[count_].pin), pin);
  challenge_[count_] = withChallenge ? 1 : 0;
  count_++;
  return save() ? UserResult::Ok : UserResult::StorageError;
}

UserResult UserStore::update(int index, String name, String pin, bool withChallenge) {
  if (index < 0 || index >= count_) return UserResult::NotFound;
  const UserResult check = validate(name, pin, index);
  if (check != UserResult::Ok) return check;

  memset(&users_[index], 0, sizeof(User));
  copyField(users_[index].name, sizeof(users_[index].name), name);
  copyField(users_[index].pin, sizeof(users_[index].pin), pin);
  challenge_[index] = withChallenge ? 1 : 0;
  return save() ? UserResult::Ok : UserResult::StorageError;
}

UserResult UserStore::remove(int index) {
  if (index < 0 || index >= count_) return UserResult::NotFound;

  for (int i = index; i < count_ - 1; i++) {
    users_[i] = users_[i + 1];
    challenge_[i] = challenge_[i + 1];
  }
  count_--;
  memset(&users_[count_], 0, sizeof(User));
  challenge_[count_] = 1;
  return save() ? UserResult::Ok : UserResult::StorageError;
}

UserResult UserStore::validate(String& name, String& pin, int ignoreIndex) const {
  name.trim();
  pin.trim();
  if (!isValidName(name)) return UserResult::InvalidName;
  if (!isValidPin(pin)) return UserResult::InvalidPin;

  const int existing = findByPin(pin);
  if (existing >= 0 && existing != ignoreIndex) return UserResult::DuplicatePin;
  return UserResult::Ok;
}

void UserStore::seedDefaults() {
  memset(users_, 0, sizeof(users_));
  memset(challenge_, 1, sizeof(challenge_));
  copyField(users_[0].name, sizeof(users_[0].name), "Daniel");
  copyField(users_[0].pin, sizeof(users_[0].pin), "1234");
  copyField(users_[1].name, sizeof(users_[1].name), "Silvia");
  copyField(users_[1].pin, sizeof(users_[1].pin), "5678");
  count_ = 2;
  save();
  Serial.println("[users] default users created");
}

bool UserStore::save() {
  // Blobs first, count last: a power cut in between leaves the old count,
  // which still matches valid entries.
  bool ok = prefs_.putBytes(KEY_USERS, users_, sizeof(users_)) == sizeof(users_);
  ok = prefs_.putBytes(KEY_CHALLENGE, challenge_, sizeof(challenge_)) == sizeof(challenge_) && ok;
  ok = prefs_.putUChar(KEY_COUNT, count_) == 1 && ok;
  if (!ok) Serial.println("[users] failed to save");
  return ok;
}
