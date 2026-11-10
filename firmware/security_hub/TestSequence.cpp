#include "TestSequence.h"
#include "Format.h"
#include "config.h"

using namespace Config;

void TestSequence::requestAuth() {
  if (!running()) {
    enter(Step::AwaitAuth, Timing::kTestAuthTimeoutMs);
  }
}

bool TestSequence::awaitingAuth() {
  if (step_ == Step::AwaitAuth && static_cast<int32_t>(millis() - stepEnds_) >= 0) {
    step_ = Step::Idle;
  }
  return step_ == Step::AwaitAuth;
}

void TestSequence::begin(const char* by) {
  if (running()) {
    return;
  }
  sink_(format(Msg::kTestStarted, by), false);
  door_.open();
  enter(Step::Servo, 0);
}

void TestSequence::update() {
  const uint32_t now = millis();
  const bool stepOver = static_cast<int32_t>(now - stepEnds_) >= 0;
  switch (step_) {
    case Step::AwaitAuth:
      awaitingAuth();
      break;
    case Step::Servo:
      if (!door_.isMoving()) {
        enter(Step::Gap, Timing::kTestGapMs);
      }
      break;
    case Step::Gap:
      if (stepOver) {
        relay_.set(true);
        enter(Step::Relay, Timing::kTestRelayMs);
      }
      break;
    case Step::Relay:
      if (stepOver) {
        relay_.set(false);
        ledFrame_ = 0;
        enter(Step::Leds, Timing::kTestLedStepMs);
      }
      break;
    case Step::Leds:
      if (stepOver) {
        const uint16_t frames = IndicatorLeds::count() * Timing::kTestLedPasses;
        if (ledFrame_ < frames) {
          leds_.showOnly(ledFrame_ % IndicatorLeds::count());
          ++ledFrame_;
          enter(Step::Leds, Timing::kTestLedStepMs);
        } else {
          leds_.showOnly(IndicatorLeds::kNone);
          step_ = Step::Idle;
          sink_(Msg::kTestDone, false);
        }
      }
      break;
    default:
      break;
  }
}

const char* TestSequence::stepLabel() const {
  switch (step_) {
    case Step::AwaitAuth: return LcdText::kTestWaitAuth;
    case Step::Servo: return LcdText::kTestServo;
    case Step::Gap: return LcdText::kTestGap;
    case Step::Relay: return LcdText::kTestRelay;
    case Step::Leds: return LcdText::kTestLeds;
    default: return "";
  }
}

void TestSequence::enter(Step step, uint32_t durationMs) {
  step_ = step;
  stepEnds_ = millis() + durationMs;
}
