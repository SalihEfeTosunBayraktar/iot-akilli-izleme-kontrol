#pragma once
#include <Arduino.h>
#include <esp_idf_version.h>
#include <esp_now.h>

// Sends RfPacket frames to the hub over ESP-NOW. The hub sits on its router's Wi-Fi channel, so the
// node finds that channel itself: it tries each channel until the hub acknowledges a frame.
class EspNowSender {
 public:
  bool begin(const uint8_t (&hubMac)[6]);
  bool send(uint32_t raw);  // true when the hub acknowledged the frame
  uint8_t channel() const { return channel_; }

 private:
  bool sendOnce(uint32_t raw);
  bool findChannel(uint32_t raw);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
  static void onSent(const wifi_tx_info_t* info, esp_now_send_status_t status);
#else
  static void onSent(const uint8_t* mac, esp_now_send_status_t status);
#endif

  uint8_t hubMac_[6] = {};
  uint8_t channel_ = 0;
  uint8_t failures_ = 0;
  static volatile int8_t lastStatus_;  // -1 pending, 0 failed, 1 delivered
};
