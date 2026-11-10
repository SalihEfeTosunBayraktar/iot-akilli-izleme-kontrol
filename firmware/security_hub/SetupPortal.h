#pragma once
#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "DeviceSettings.h"
#include "EventSink.h"

// Setup access point with a captive portal for changing Wi-Fi/AWS settings from a phone.
// Shares the panel's WebServer: AP clients use /setup freely, LAN clients need the panel login.
class SetupPortal {
 public:
  SetupPortal(WebServer& server, DeviceSettings& settings, EventSink sink)
      : server_(server), settings_(settings), sink_(sink) {}

  // Must run before WebPanel::begin() so the captive "/" redirect wins over the panel page.
  void begin(bool staConnected, bool requested);
  void update();
  void requestSetupMode(const char* by);  // persists the request and restarts
  bool apActive() const { return active_; }
  const String& apSuffix() const { return apSuffix_; }

 private:
  void addRoutes();
  bool startAp();
  void stopAp();
  void holdOpen();  // AP stays on until settings are saved
  void retryStation(uint32_t now);
  bool fromAp();
  bool allowed();
  void redirectToSetup();
  void sendScan();
  void save();
  void reply(int code, bool ok, const String& message);
  void scheduleRestart();

  WebServer& server_;
  DeviceSettings& settings_;
  EventSink sink_;
  DNSServer dns_;
  String apSuffix_;
  bool active_ = false;
  bool indefinite_ = false;
  bool staAttempting_ = false;
  uint32_t closeAt_ = 0;
  uint32_t nextStaAttempt_ = 0;
  uint32_t staAttemptEnds_ = 0;
  bool restartPending_ = false;
  uint32_t restartAt_ = 0;
};
