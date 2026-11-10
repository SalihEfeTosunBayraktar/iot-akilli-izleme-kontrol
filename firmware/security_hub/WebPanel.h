#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "AlarmSystem.h"
#include "Door.h"
#include "EventSink.h"
#include "Relay.h"
#include "StatusReport.h"
#include "TestSequence.h"

// Password-protected dashboard: live status JSON plus arm/disarm/door/relay commands.
class WebPanel {
 public:
  struct Deps {
    AlarmSystem& alarm;
    Relay& relay;
    Door& door;
    const StatusReport& status;
    TestSequence& test;
    std::function<void()> onSetupRequest;
    EventSink sink;
  };

  explicit WebPanel(Deps deps) : d_(deps), server_(80) {}
  void begin();
  void handle() { server_.handleClient(); }
  WebServer& server() { return server_; }  // shared with the setup portal (one server on port 80)

 private:
  bool authorized();
  void sendStatus();
  void route(const char* path, HTTPMethod method, std::function<void()> action);

  Deps d_;
  WebServer server_;
};
