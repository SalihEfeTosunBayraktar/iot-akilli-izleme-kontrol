#pragma once
#include <Arduino.h>
#include <RCSwitch.h>
#include <RfPacket.h>
#include "EventSink.h"
#include "config.h"

// Receives sensor packets (433 MHz here, ESP-NOW via ingest) and tracks which nodes are alive.
class NodeMonitor {
 public:
  struct Node {
    bool online = false;
    uint32_t lastSeen = 0;
    RfPacket::SensorData data{};
  };

  explicit NodeMonitor(EventSink sink) : sink_(sink) {}
  void begin(uint8_t pin);
  bool poll(RfPacket::SensorData& out);  // true when a valid 433 MHz packet arrived
  bool ingest(uint32_t raw, RfPacket::SensorData& out);  // any transport: validates and records a packet
  void update();                         // marks silent nodes offline
  const Node& node(uint8_t id) const { return nodes_[id]; }
  static constexpr uint8_t count() { return Config::Security::kMaxNodes; }

 private:
  RCSwitch rf_;
  Node nodes_[Config::Security::kMaxNodes];
  EventSink sink_;
};
