#pragma once
#include <Arduino.h>
#include <functional>
#include "StatusReport.h"
#include "config.h"

// Periodically uploads the status JSON to the PC dashboard; backs off while the PC is offline.
// The reply carries queued remote commands; their results ride along with the next upload.
class TelemetryClient {
 public:
  using CommandHandler = std::function<bool(const String& command)>;  // false = unknown/refused

  TelemetryClient(const StatusReport& status, CommandHandler onCommand) : status_(status), onCommand_(onCommand) {}
  void update();
  void setUrl(const String& url) { url_ = url; }  // from DeviceSettings, editable in the setup portal

 private:
  struct Ack {
    uint32_t id;
    bool ok;
  };

  bool post(const String& body, String& reply);
  String withAcks(const String& status) const;
  void runCommands(const String& reply);

  const StatusReport& status_;
  CommandHandler onCommand_;
  uint32_t nextAttempt_ = 0;
  String url_;
  Ack acks_[Config::Telemetry::kMaxCommandsPerReply];
  uint8_t ackCount_ = 0;
};
