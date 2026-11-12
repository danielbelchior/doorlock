#pragma once

#include <Arduino.h>

// One sound per outcome. Patterns are in buzzer.cpp.
enum class Sound : uint8_t {
  Key,            // any key
  Invalid,        // a key that does nothing here
  Welcome,        // known PIN
  AccessDenied,   // unknown PIN
  Correct,        // right challenge answer
  Wrong,          // wrong challenge answer
  Timeout,        // input or menu abandoned
  MenuOpen,
  ShowIp,
  Hotspot,
  Restart,
  Ready,          // boot finished
  WifiConnected,
  UserAdded,
  UserUpdated,
  UserRemoved,
  WifiSaved,
  Rejected,       // a web request failed validation or could not be done
  UpdateDone,
  UpdateFailed,
};

// Active buzzer (MH-FMD module). It has its own oscillator and always plays
// the same pitch, so sounds differ only in their on/off rhythm. Driven with
// digitalWrite(), no library needed. Non-blocking: call update() every loop.
class Buzzer {
 public:
  struct Step {
    bool on;
    uint16_t ms;
  };

  void begin();
  void update();

  // Key and Invalid are ignored while any other pattern plays, so a key
  // press cannot cut result feedback short. Other sounds restart.
  void play(Sound sound);
  void stop();

 private:
  void startStep(const Step& step);
  void set(bool on);

  const Step* steps_ = nullptr;
  uint8_t stepCount_ = 0;
  uint8_t stepIndex_ = 0;
  bool pattern_ = false;
  uint32_t nextMs_ = 0;
};
