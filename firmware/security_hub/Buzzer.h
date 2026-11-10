#pragma once
#include <Arduino.h>

// Non-blocking active buzzer: short beeps for feedback, a pulsing siren for alarms.
class Buzzer {
 public:
  explicit Buzzer(uint8_t pin) : pin_(pin) {}
  void begin();
  void beep(uint16_t durationMs);
  void setSiren(bool on);
  void update();

 private:
  void write(bool on);

  uint8_t pin_;
  uint32_t beepUntil_ = 0;
  bool siren_ = false;
  bool sirenPhase_ = false;
  uint32_t lastToggle_ = 0;
};
