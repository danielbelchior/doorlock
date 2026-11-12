#include "buzzer.h"

#include "config.h"
#include "util.h"

namespace {

// The MH-FMD switches the buzzer on when I/O is LOW, so silence holds the pin
// HIGH.
constexpr uint8_t BUZZER_ON_LEVEL = BUZZER_ACTIVE_LOW ? LOW : HIGH;
constexpr uint8_t BUZZER_OFF_LEVEL = BUZZER_ACTIVE_LOW ? HIGH : LOW;

using Step = Buzzer::Step;

// Building blocks: short, medium and long beeps separated by a gap. Every
// sound must keep a rhythm no other sound uses.
constexpr Step CLICK = {true, BUZZER_KEY_MS};
constexpr Step S = {true, 60};
constexpr Step M = {true, 250};
constexpr Step L = {true, 600};
constexpr Step G = {false, 80};

const Step KEY_STEPS[] = {CLICK};
const Step INVALID_STEPS[] = {CLICK, {false, 60}, CLICK};
const Step WELCOME_STEPS[] = {S, G, S};
const Step ACCESS_DENIED_STEPS[] = {L};
const Step CORRECT_STEPS[] = {S, G, S, G, S, G, M};
const Step WRONG_STEPS[] = {M, G, M};
const Step TIMEOUT_STEPS[] = {M, G, S};
const Step MENU_OPEN_STEPS[] = {S, G, S, G, S};
const Step SHOW_IP_STEPS[] = {S, G, M};
const Step HOTSPOT_STEPS[] = {S, G, M, G, S};
// Must end before the restart, 1 s after the request.
const Step RESTART_STEPS[] = {S, G, S, G, L};
const Step READY_STEPS[] = {S, G, S, G, S, G, S};
const Step WIFI_CONNECTED_STEPS[] = {S, G, S, G, M};
const Step USER_ADDED_STEPS[] = {S, G, M, G, M};
const Step USER_UPDATED_STEPS[] = {M, G, S, G, M};
const Step USER_REMOVED_STEPS[] = {M, G, M, G, S};
const Step WIFI_SAVED_STEPS[] = {M, G, S, G, S};
const Step REJECTED_STEPS[] = {M, G, M, G, M};
// Must end before the reboot, 1.5 s after the upload.
const Step UPDATE_DONE_STEPS[] = {S, G, S, G, S, G, S, G, S};
const Step UPDATE_FAILED_STEPS[] = {L, G, L};

struct Pattern {
  const Step* steps;
  uint8_t count;
};

template <size_t N>
constexpr Pattern pattern(const Step (&steps)[N]) {
  return {steps, static_cast<uint8_t>(N)};
}

Pattern patternFor(Sound sound) {
  switch (sound) {
    case Sound::Key:
      return pattern(KEY_STEPS);
    case Sound::Invalid:
      return pattern(INVALID_STEPS);
    case Sound::Welcome:
      return pattern(WELCOME_STEPS);
    case Sound::AccessDenied:
      return pattern(ACCESS_DENIED_STEPS);
    case Sound::Correct:
      return pattern(CORRECT_STEPS);
    case Sound::Wrong:
      return pattern(WRONG_STEPS);
    case Sound::Timeout:
      return pattern(TIMEOUT_STEPS);
    case Sound::MenuOpen:
      return pattern(MENU_OPEN_STEPS);
    case Sound::ShowIp:
      return pattern(SHOW_IP_STEPS);
    case Sound::Hotspot:
      return pattern(HOTSPOT_STEPS);
    case Sound::Restart:
      return pattern(RESTART_STEPS);
    case Sound::Ready:
      return pattern(READY_STEPS);
    case Sound::WifiConnected:
      return pattern(WIFI_CONNECTED_STEPS);
    case Sound::UserAdded:
      return pattern(USER_ADDED_STEPS);
    case Sound::UserUpdated:
      return pattern(USER_UPDATED_STEPS);
    case Sound::UserRemoved:
      return pattern(USER_REMOVED_STEPS);
    case Sound::WifiSaved:
      return pattern(WIFI_SAVED_STEPS);
    case Sound::Rejected:
      return pattern(REJECTED_STEPS);
    case Sound::UpdateDone:
      return pattern(UPDATE_DONE_STEPS);
    case Sound::UpdateFailed:
      return pattern(UPDATE_FAILED_STEPS);
  }
  return pattern(KEY_STEPS);
}

bool isClick(Sound sound) {
  return sound == Sound::Key || sound == Sound::Invalid;
}

}  // namespace

void Buzzer::begin() {
  // Write the idle level before switching to output, so the buzzer does not
  // chirp at boot.
  digitalWrite(PIN_BUZZER, BUZZER_OFF_LEVEL);
  pinMode(PIN_BUZZER, OUTPUT);
  set(false);
}

void Buzzer::play(Sound sound) {
  if (isClick(sound) && pattern_) return;

  const Pattern p = patternFor(sound);
  steps_ = p.steps;
  stepCount_ = p.count;
  pattern_ = !isClick(sound);
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
