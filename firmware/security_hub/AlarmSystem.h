#pragma once
#include <Arduino.h>
#include "Buzzer.h"
#include "EventSink.h"

enum class AlarmState : uint8_t { Disarmed, Arming, Armed, Triggered };

// Arm/disarm state machine plus an always-on gas alarm.
class AlarmSystem {
 public:
  AlarmSystem(Buzzer& buzzer, EventSink sink) : buzzer_(buzzer), sink_(sink) {}
  void arm(const char* by);
  void disarm(const char* by);
  void onMotion();
  void onGasLevel(uint8_t level);
  void update();

  AlarmState state() const { return state_; }
  bool gasAlarm() const { return gasAlarm_; }
  const char* stateCode() const;
  uint32_t armingRemainingMs() const;

 private:
  void refreshSiren();

  Buzzer& buzzer_;
  EventSink sink_;
  AlarmState state_ = AlarmState::Disarmed;
  bool gasAlarm_ = false;
  uint32_t armingStarted_ = 0;
  String armedBy_;
};
