#pragma once
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <functional>
#include "DeviceSettings.h"
#include "EventLog.h"
#include "StatusReport.h"

// AWS IoT Core link: publishes telemetry and new events over MQTT/TLS and applies the
// fan/valve commands the cloud writes to this device's shadow.
class CloudLink {
 public:
  struct Actuators {
    std::function<void(bool on)> setFan;
    std::function<void(bool open)> setValve;
    std::function<bool()> fanOn;
    std::function<bool()> valveOpen;
  };

  CloudLink(const StatusReport& status, const EventLog& log, Actuators actuators);
  void begin(const DeviceSettings& settings);  // endpoint/certs from NVS; stays idle if incomplete
  void update();
  bool connected() { return mqtt_.connected(); }

 private:
  bool connect();
  void publishNewEvents();
  void onMessage(char* topic, uint8_t* payload, unsigned int length);
  void reportState();
  String topic(const char* suffix) const;

  const StatusReport& status_;
  const EventLog& log_;
  Actuators actuators_;
  WiFiClientSecure net_;
  PubSubClient mqtt_;
  uint32_t nextConnect_ = 0;
  uint32_t nextTelemetry_ = 0;
  uint32_t publishedEvents_ = 0;  // survives reconnects so nothing is sent twice
  bool configured_ = false;
  String thing_;
  String prefix_;
};
