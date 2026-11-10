#include "EspNowReceiver.h"
#include <RfPacket.h>
#include "config.h"

QueueHandle_t EspNowReceiver::queue_ = nullptr;

bool EspNowReceiver::begin() {
  queue_ = xQueueCreate(Config::EspNow::kQueueLength, sizeof(uint32_t));
  if (queue_ == nullptr || esp_now_init() != ESP_OK) {
    return false;
  }
  return esp_now_register_recv_cb(onReceive) == ESP_OK;
}

bool EspNowReceiver::pop(uint32_t& raw) {
  return queue_ != nullptr && xQueueReceive(queue_, &raw, 0) == pdTRUE;
}

void EspNowReceiver::onReceive(const esp_now_recv_info_t*, const uint8_t* data, int length) {
  if (length != sizeof(uint32_t)) {
    return;
  }
  uint32_t raw;
  memcpy(&raw, data, sizeof(raw));
  xQueueSend(queue_, &raw, 0);  // drop when full; the node resends every few seconds anyway
}
