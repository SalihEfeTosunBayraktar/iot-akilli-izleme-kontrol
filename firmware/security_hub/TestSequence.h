#pragma once
#include <Arduino.h>
#include "Door.h"
#include "EventSink.h"
#include "IndicatorLeds.h"
#include "Relay.h"

// Module self-test: servo cycle, pause, relay on/off, LED chase. Runs without blocking the loop.
class TestSequence {
 public:
  TestSequence(Door& door, Relay& relay, IndicatorLeds& leds, EventSink sink)
      : door_(door), relay_(relay), leds_(leds), sink_(sink) {}

  void requestAuth();                // keypad test key: wait for card/PIN
  bool awaitingAuth();               // true while the request is still valid
  void begin(const char* by);
  void update();
  bool running() const { return step_ != Step::Idle && step_ != Step::AwaitAuth; }
  const char* stepLabel() const;

 private:
  enum class Step : uint8_t { Idle, AwaitAuth, Servo, Gap, Relay, Leds };
  void enter(Step step, uint32_t durationMs);

  Door& door_;
  Relay& relay_;
  IndicatorLeds& leds_;
  EventSink sink_;
  Step step_ = Step::Idle;
  uint32_t stepEnds_ = 0;
  uint16_t ledFrame_ = 0;
};
