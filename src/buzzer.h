#pragma once

#include <Arduino.h>

enum class Sound : uint8_t { Key, Success, Error };

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

  // Sound::Key is ignored while a success or error pattern plays, so a key
  // press cannot cut the result feedback short. Other sounds restart.
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
