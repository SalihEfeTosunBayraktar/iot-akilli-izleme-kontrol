#pragma once
#include <Arduino.h>
#include <DHT.h>

// Pins, timing and thresholds of the ESP32 D1 Mini (WROOM-32) room node (replaces the Nano + 433 MHz pair).
// Powered from the MB102 5V rail into VCC; the 3.3V rail feeds the DHT11. The board has double pin rows,
// so it sits beside the breadboard and is wired with female-male jumpers.
namespace Config {

constexpr uint8_t kNodeId = 1;  // 0 is the Nano node; each node needs a unique id (0..3)

// Wi-Fi station MAC of the hub; the hub logs it at boot ("ESP-NOW alıcı hazır, merkez MAC: ...").
inline constexpr uint8_t kHubMac[6] = {0x10, 0x20, 0xBA, 0x4C, 0x5D, 0xC8};

namespace Pin {
constexpr uint8_t kPir = 26;        // inner row, antenna end side (3rd pin)
constexpr uint8_t kDht = 27;        // outer row, 2nd pin
constexpr uint8_t kGas = 34;        // ADC1, input only; MQ-2 AO through a 4.7k/10k divider (outer row, 6th pin)
constexpr uint8_t kLedGas = 16;     // red
constexpr uint8_t kLedMotion = 17;  // yellow
constexpr uint8_t kLedTemp = 18;    // blue
constexpr uint8_t kStatusLed = 2;   // on-board blue LED of the D1 Mini, blinks on every delivered packet
}  // namespace Pin

constexpr uint8_t kDhtType = DHT11;

namespace Timing {
constexpr uint32_t kSendIntervalMs = 2000;
constexpr uint32_t kGasWarmupMs = 60000;  // MQ-2 reads high while its heater warms up
constexpr uint32_t kAckWaitMs = 50;       // wait for the MAC-layer delivery report
constexpr uint16_t kStatusBlinkMs = 30;
}  // namespace Timing

namespace Gas {
constexpr uint8_t kLocalAlarmLevel = 100;    // 0..255, same scale as the hub and the Nano node
constexpr float kDividerRatio = 14.7f / 10.0f;  // (4.7k + 10k) / 10k: ADC voltage -> sensor voltage
constexpr uint16_t kSensorFullScaleMv = 5000;   // MQ-2 module output range at 5 V supply
}  // namespace Gas

namespace Indicators {
constexpr uint8_t kTempHighC = 30;  // blue LED turns on at or above this temperature
}

namespace Text {
constexpr const char* kInitFailed = "ESP-NOW init failed";
}

namespace EspNow {
constexpr uint8_t kMaxChannel = 13;
constexpr uint8_t kFailuresBeforeRescan = 3;  // the hub may have moved to another router channel
}

}  // namespace Config
