#include "NodeMonitor.h"
#include "Format.h"

void NodeMonitor::begin(uint8_t pin) {
  rf_.enableReceive(digitalPinToInterrupt(pin));
}

bool NodeMonitor::poll(RfPacket::SensorData& out) {
  if (!rf_.available()) {
    return false;
  }
  const uint32_t raw = rf_.getReceivedValue();
  const unsigned int bits = rf_.getReceivedBitlength();
  rf_.resetAvailable();
  return bits == RfPacket::kBitLength && ingest(raw, out);
}

bool NodeMonitor::ingest(uint32_t raw, RfPacket::SensorData& out) {
  if (!RfPacket::decode(raw, out) || out.nodeId >= count()) {
    return false;
  }
  Node& node = nodes_[out.nodeId];
  if (!node.online) {
    node.online = true;
    sink_(format(Config::Msg::kNodeOnline, out.nodeId), false);
  }
  node.lastSeen = millis();
  node.data = out;
  return true;
}

void NodeMonitor::update() {
  const uint32_t now = millis();
  for (uint8_t id = 0; id < count(); ++id) {
    Node& node = nodes_[id];
    if (node.online && now - node.lastSeen > Config::Timing::kNodeTimeoutMs) {
      node.online = false;
      sink_(format(Config::Msg::kNodeOffline, id), true);
    }
  }
}
