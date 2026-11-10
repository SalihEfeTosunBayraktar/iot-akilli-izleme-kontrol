#include "Door.h"
#include "config.h"

void Door::begin() {
  servo_.setPeriodHertz(50);
  servo_.attach(pin_);
  moveTo(Config::DoorServo::kClosedAngle);
}

void Door::open() {
  if (held_) {
    return;
  }
  moveTo(Config::DoorServo::kOpenAngle);
  closeAt_ = millis() + Config::Timing::kDoorOpenMs;
}

void Door::hold(bool open) {
  held_ = open;
  closeAt_ = 0;
  moveTo(open ? Config::DoorServo::kOpenAngle : Config::DoorServo::kClosedAngle);
}

void Door::update() {
  const uint32_t now = millis();
  if (closeAt_ != 0 && static_cast<int32_t>(now - closeAt_) >= 0) {
    closeAt_ = 0;
    moveTo(Config::DoorServo::kClosedAngle);
  }
  if (releaseAt_ != 0 && static_cast<int32_t>(now - releaseAt_) >= 0) {
    releaseAt_ = 0;
    servo_.release();
  }
}

void Door::moveTo(uint8_t angle) {
  servo_.write(angle);
  releaseAt_ = millis() + Config::Timing::kServoSettleMs;
}
