#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Receives RfPacket frames from ESP32 sensor nodes over ESP-NOW. The radio callback runs in the
// Wi-Fi task, so it only queues the 4-byte packet; the main loop drains the queue.
class EspNowReceiver {
 public:
  bool begin();  // call once Wi-Fi is started (STA or AP+STA)
  bool pop(uint32_t& raw);

 private:
  static void onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int length);
  static QueueHandle_t queue_;
};
