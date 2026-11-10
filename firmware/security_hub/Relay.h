#pragma once
#include <Arduino.h>
#include "config.h"

// Relay output controlled from the web panel (lamp, fan, siren light...).
class Relay {
 public:
  explicit Relay(uint8_t pin) : pin_(pin) {}
  void begin() {
    // Active-low 5V modules need their input pulled to their own VCC to switch off; a 3.3V HIGH is
    // not enough, so release the pin (open drain) and let the module's pull-up do it.
    pinMode(pin_, Config::RelayOutput::kActiveLow ? OUTPUT_OPEN_DRAIN : OUTPUT);
    set(false);
  }
  void set(bool on) {
    on_ = on;
    digitalWrite(pin_, (on != Config::RelayOutput::kActiveLow) ? HIGH : LOW);
  }
  bool isOn() const { return on_; }

 private:
  uint8_t pin_;
  bool on_ = false;
};
