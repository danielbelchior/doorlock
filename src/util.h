#pragma once

#include <Arduino.h>

// Overflow-safe deadline check (millis() wraps after ~49 days).
inline bool timeReached(uint32_t deadlineMs) {
  return static_cast<int32_t>(millis() - deadlineMs) >= 0;
}
