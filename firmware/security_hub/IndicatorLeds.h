#pragma once
#include <Arduino.h>
#include "NodeMonitor.h"
#include "config.h"

// Status LEDs: gas alarm, motion seen by any online node, temperature at or above the threshold.
class IndicatorLeds {
 public:
  static constexpr uint8_t kNone = 0xFF;
  static constexpr uint8_t count() { return sizeof(kPins); }

  void begin() {
    for (uint8_t pin : kPins) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
    }
  }

  void update(const NodeMonitor& nodes, bool gasAlarm) {
    bool motion = false;
    bool hot = false;
    for (uint8_t id = 0; id < NodeMonitor::count(); ++id) {
      const NodeMonitor::Node& node = nodes.node(id);
      if (!node.online) {
        continue;
      }
      motion = motion || node.data.motion;
      hot = hot || node.data.temperature >= Config::Indicators::kTempHighC;
    }
    digitalWrite(Config::Pin::kLedGas, gasAlarm ? HIGH : LOW);
    digitalWrite(Config::Pin::kLedMotion, motion ? HIGH : LOW);
    digitalWrite(Config::Pin::kLedTemp, hot ? HIGH : LOW);
  }

  // Light a single LED by index (kNone turns all off); used by the self-test chase.
  void showOnly(uint8_t index) {
    for (uint8_t i = 0; i < count(); ++i) {
      digitalWrite(kPins[i], i == index ? HIGH : LOW);
    }
  }

 private:
  static constexpr uint8_t kPins[] = {Config::Pin::kLedGas, Config::Pin::kLedMotion, Config::Pin::kLedTemp};
};
