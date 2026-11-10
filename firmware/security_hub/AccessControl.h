#pragma once
#include <Arduino.h>
#include <Keypad.h>
#include <MFRC522.h>
#include <functional>
#include "Buzzer.h"
#include "Display.h"
#include "EventSink.h"

// Reads RFID cards and keypad input, enforces the lockout after repeated failures.
class AccessControl {
 public:
  struct Handlers {
    std::function<void(const String& who, const char* method)> onGranted;
    std::function<void()> onArmRequest;
    std::function<void()> onTestRequest;
    std::function<void()> onSetupRequest;
    EventSink onEvent;
  };

  AccessControl(Display& display, Buzzer& buzzer, Handlers handlers);
  void begin();
  void update();

 private:
  void pollCard();
  void pollKeypad();
  void grant(const String& who, const char* method);
  void deny(const String& reason);
  bool isLocked();
  static String uidToString(const MFRC522::Uid& uid);

  Display& display_;
  Buzzer& buzzer_;
  Handlers handlers_;
  MFRC522 rfid_;
  Keypad keypad_;
  String pinBuffer_;
  uint8_t failedAttempts_ = 0;
  uint32_t lockedUntil_ = 0;
  uint32_t armPendingUntil_ = 0;  // arm key pressed, waiting for card/PIN
};
