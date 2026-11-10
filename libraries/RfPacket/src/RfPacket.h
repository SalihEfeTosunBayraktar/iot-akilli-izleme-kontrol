#pragma once
#include <stdint.h>

// One 433 MHz rc-switch code carries a whole sensor reading.
// Layout (MSB -> LSB): magic(3) node(2) motion(1) gas(8) temp(7) hum(7) checksum(4)
namespace RfPacket {

struct SensorData {
  uint8_t nodeId;       // 0..3
  bool motion;
  uint8_t gasLevel;     // 0..255 (10-bit ADC >> 2)
  uint8_t temperature;  // 0..127 C
  uint8_t humidity;     // 0..100 %
};

constexpr unsigned int kBitLength = 32;
constexpr uint32_t kMagic = 0b101;

inline uint8_t checksum(uint32_t body) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < 7; ++i) {
    sum ^= (body >> (i * 4)) & 0x0F;
  }
  return sum;
}

inline uint32_t encode(const SensorData& d) {
  const uint32_t body = (kMagic << 25)
      | (static_cast<uint32_t>(d.nodeId & 0x03) << 23)
      | (static_cast<uint32_t>(d.motion ? 1 : 0) << 22)
      | (static_cast<uint32_t>(d.gasLevel) << 14)
      | (static_cast<uint32_t>(d.temperature & 0x7F) << 7)
      | static_cast<uint32_t>(d.humidity & 0x7F);
  return (body << 4) | checksum(body);
}

inline bool decode(uint32_t raw, SensorData& out) {
  const uint32_t body = raw >> 4;
  if ((raw & 0x0F) != checksum(body) || (body >> 25) != kMagic) {
    return false;
  }
  out.nodeId = (body >> 23) & 0x03;
  out.motion = (body >> 22) & 0x01;
  out.gasLevel = (body >> 14) & 0xFF;
  out.temperature = (body >> 7) & 0x7F;
  out.humidity = body & 0x7F;
  return true;
}

}  // namespace RfPacket
