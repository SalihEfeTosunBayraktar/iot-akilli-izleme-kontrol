#include "Buzzer.h"
#include "config.h"

void Buzzer::begin() {
  pinMode(pin_, OUTPUT);
  write(false);
}

void Buzzer::beep(uint16_t durationMs) {
  beepUntil_ = millis() + durationMs;
  write(true);
}

void Buzzer::setSiren(bool on) {
  siren_ = on;
  if (!on) {
    write(false);
  }
}

void Buzzer::update() {
  const uint32_t now = millis();
  if (siren_) {
    if (now - lastToggle_ >= Config::Timing::kSirenToggleMs) {
      lastToggle_ = now;
      sirenPhase_ = !sirenPhase_;
      write(sirenPhase_);
    }
    return;
  }
  if (beepUntil_ != 0 && static_cast<int32_t>(now - beepUntil_) >= 0) {
    beepUntil_ = 0;
    write(false);
  }
}

void Buzzer::write(bool on) {
  digitalWrite(pin_, on ? HIGH : LOW);
}
