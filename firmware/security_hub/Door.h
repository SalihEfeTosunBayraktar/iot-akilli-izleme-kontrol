#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>

// Servo-driven door latch that closes itself after a timeout.
class Door {
 public:
  explicit Door(uint8_t pin) : pin_(pin) {}
  void begin();
  void open();
  void hold(bool open);  // keep open/closed until told otherwise (cloud valve command)
  void update();
  bool isOpen() const { return held_ || closeAt_ != 0; }
  bool isMoving() const { return closeAt_ != 0 || releaseAt_ != 0; }  // open, or still settling

 private:
  void moveTo(uint8_t angle);

  uint8_t pin_;
  Servo servo_;
  uint32_t closeAt_ = 0;
  uint32_t releaseAt_ = 0;
  bool held_ = false;
};
