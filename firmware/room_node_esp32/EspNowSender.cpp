#include "EspNowSender.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include "config.h"

volatile int8_t EspNowSender::lastStatus_ = -1;

bool EspNowSender::begin(const uint8_t (&hubMac)[6]) {
  memcpy(hubMac_, hubMac, sizeof(hubMac_));
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK || esp_now_register_send_cb(onSent) != ESP_OK) {
    return false;
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, hubMac_, sizeof(hubMac_));
  peer.channel = 0;  // follow whatever channel the radio is on
  peer.encrypt = false;
  failures_ = Config::EspNow::kFailuresBeforeRescan;  // the first send searches for the channel
  return esp_now_add_peer(&peer) == ESP_OK;
}

bool EspNowSender::send(uint32_t raw) {
  if (failures_ < Config::EspNow::kFailuresBeforeRescan && sendOnce(raw)) {
    failures_ = 0;
    return true;
  }
  if (++failures_ < Config::EspNow::kFailuresBeforeRescan) {
    return false;
  }
  const bool found = findChannel(raw);
  failures_ = found ? 0 : Config::EspNow::kFailuresBeforeRescan;
  return found;
}

bool EspNowSender::findChannel(uint32_t raw) {
  for (uint8_t ch = 1; ch <= Config::EspNow::kMaxChannel; ++ch) {
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    if (sendOnce(raw)) {
      channel_ = ch;
      return true;
    }
  }
  channel_ = 0;
  return false;
}

bool EspNowSender::sendOnce(uint32_t raw) {
  lastStatus_ = -1;
  if (esp_now_send(hubMac_, reinterpret_cast<const uint8_t*>(&raw), sizeof(raw)) != ESP_OK) {
    return false;
  }
  const uint32_t start = millis();
  while (lastStatus_ < 0 && millis() - start < Config::Timing::kAckWaitMs) {
    delay(1);
  }
  return lastStatus_ == 1;
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
void EspNowSender::onSent(const wifi_tx_info_t*, esp_now_send_status_t status) {
#else
void EspNowSender::onSent(const uint8_t*, esp_now_send_status_t status) {
#endif
  lastStatus_ = status == ESP_NOW_SEND_SUCCESS ? 1 : 0;
}
