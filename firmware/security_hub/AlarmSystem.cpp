#include "AlarmSystem.h"
#include "Format.h"
#include "config.h"

void AlarmSystem::arm(const char* by) {
  if (state_ != AlarmState::Disarmed) {
    return;
  }
  state_ = AlarmState::Arming;
  armingStarted_ = millis();
  armedBy_ = by;
  sink_(format(Config::Msg::kArming, Config::Timing::kArmDelayMs / 1000, by), false);
}

void AlarmSystem::disarm(const char* by) {
  if (state_ == AlarmState::Disarmed) {
    return;
  }
  state_ = AlarmState::Disarmed;
  refreshSiren();
  sink_(format(Config::Msg::kDisarmed, by), true);
}

void AlarmSystem::onMotion() {
  if (state_ != AlarmState::Armed) {
    return;
  }
  state_ = AlarmState::Triggered;
  refreshSiren();
  sink_(Config::Msg::kIntrusion, true);
}

void AlarmSystem::onGasLevel(uint8_t level) {
  if (!gasAlarm_ && level >= Config::Security::kGasAlarmLevel) {
    gasAlarm_ = true;
    refreshSiren();
    sink_(format(Config::Msg::kGas, level), true);
  } else if (gasAlarm_ && level <= Config::Security::kGasClearLevel) {
    gasAlarm_ = false;
    refreshSiren();
    sink_(Config::Msg::kGasClear, true);
  }
}

void AlarmSystem::update() {
  if (state_ == AlarmState::Arming && millis() - armingStarted_ >= Config::Timing::kArmDelayMs) {
    state_ = AlarmState::Armed;
    sink_(format(Config::Msg::kArmed, armedBy_.c_str()), true);
  }
}

const char* AlarmSystem::stateCode() const {
  switch (state_) {
    case AlarmState::Arming: return "arming";
    case AlarmState::Armed: return "armed";
    case AlarmState::Triggered: return "triggered";
    default: return "disarmed";
  }
}

uint32_t AlarmSystem::armingRemainingMs() const {
  if (state_ != AlarmState::Arming) {
    return 0;
  }
  const uint32_t elapsed = millis() - armingStarted_;
  return elapsed >= Config::Timing::kArmDelayMs ? 0 : Config::Timing::kArmDelayMs - elapsed;
}

void AlarmSystem::refreshSiren() {
  buzzer_.setSiren(state_ == AlarmState::Triggered || gasAlarm_);
}
