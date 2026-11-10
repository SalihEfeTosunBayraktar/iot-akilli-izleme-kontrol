#pragma once
#include <Arduino.h>
#include <DHT.h>

// Pins, timing and thresholds of the room sensor node (Arduino Nano).
namespace Config {

constexpr uint8_t kNodeId = 0;  // 0..3, unique per node

namespace Pin {
constexpr uint8_t kPir = 2;
constexpr uint8_t kDht = 4;
constexpr uint8_t kBuzzer = 5;
constexpr uint8_t kRfTransmitter = 10;
constexpr uint8_t kStatusLed = LED_BUILTIN;
constexpr uint8_t kGas = A0;
constexpr uint8_t kLedGas = 6;     // red
constexpr uint8_t kLedMotion = 7;  // yellow
constexpr uint8_t kLedTemp = 8;    // blue
}  // namespace Pin

namespace Indicators {
constexpr uint8_t kTempHighC = 30;  // blue LED turns on at or above this temperature
}

constexpr uint8_t kDhtType = DHT11;

namespace Timing {
constexpr uint32_t kSendIntervalMs = 2000;
constexpr uint32_t kGasWarmupMs = 60000;  // MQ-2 reads high while its heater warms up
constexpr uint16_t kLedBlinkMs = 30;
}  // namespace Timing

namespace Rf {
constexpr int kRepeatTransmit = 4;
}

namespace Gas {
constexpr uint8_t kLocalAlarmLevel = 100;  // 0..255, same scale as the hub
}

}  // namespace Config
