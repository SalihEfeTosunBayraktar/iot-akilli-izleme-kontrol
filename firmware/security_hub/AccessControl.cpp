#include "AccessControl.h"
#include <SPI.h>
#include "Format.h"
#include "SecretsLoader.h"
#include "config.h"

using namespace Config;

AccessControl::AccessControl(Display& display, Buzzer& buzzer, Handlers handlers)
    : display_(display),
      buzzer_(buzzer),
      handlers_(handlers),
      rfid_(Pin::kRfidSs, Pin::kRfidRst),
      keypad_(makeKeymap(Keypad4x4::keys), Pin::keypadRows, Pin::keypadCols, Keypad4x4::kRows,
              Keypad4x4::kCols) {}

void AccessControl::begin() {
  SPI.begin(Pin::kRfidSck, Pin::kRfidMiso, Pin::kRfidMosi, Pin::kRfidSs);
  rfid_.PCD_Init();
}

void AccessControl::update() {
  pollCard();
  pollKeypad();
}

void AccessControl::pollCard() {
  if (!rfid_.PICC_IsNewCardPresent() || !rfid_.PICC_ReadCardSerial()) {
    return;
  }
  const String uid = uidToString(rfid_.uid);
  rfid_.PICC_HaltA();
  if (isLocked()) {
    return;
  }
  for (const Card& card : kAuthorizedCards) {
    if (uid.equalsIgnoreCase(card.uid)) {
      grant(card.owner, Msg::kByCard);
      return;
    }
  }
  deny(format(Msg::kUnknownCard, uid.c_str()));
}

void AccessControl::pollKeypad() {
  const char key = keypad_.getKey();
  if (key == NO_KEY) {
    return;
  }
  buzzer_.beep(Timing::kKeyBeepMs);
  if (isLocked()) {
    return;
  }
  if (key == Keypad4x4::kArm) {
    pinBuffer_ = "";
    armPendingUntil_ = millis() + Timing::kArmAuthTimeoutMs;
    display_.flash(LcdText::kArmPrompt, "");
  } else if (key == Keypad4x4::kSetup) {
    const bool ok = pinBuffer_ == Secrets::kDoorPin;
    pinBuffer_ = "";
    if (ok) {
      failedAttempts_ = 0;
      handlers_.onSetupRequest();
    } else {
      deny(Msg::kWrongPin);
    }
  } else if (key == Keypad4x4::kTest) {
    pinBuffer_ = "";
    handlers_.onTestRequest();
  } else if (key == Keypad4x4::kClear) {
    pinBuffer_ = "";
    display_.showInput(LcdText::kPinPrompt, 0);
  } else if (key == Keypad4x4::kSubmit) {
    const bool ok = pinBuffer_ == Secrets::kDoorPin;
    pinBuffer_ = "";
    if (ok) {
      grant(Msg::kPinOwner, Msg::kByPin);
    } else {
      deny(Msg::kWrongPin);
    }
  } else if (isDigit(key) && pinBuffer_.length() < Security::kPinMaxLength) {
    pinBuffer_ += key;
    display_.showInput(LcdText::kPinPrompt, pinBuffer_.length());
  }
}

void AccessControl::grant(const String& who, const char* method) {
  failedAttempts_ = 0;
  if (armPendingUntil_ != 0 && static_cast<int32_t>(millis() - armPendingUntil_) < 0) {
    armPendingUntil_ = 0;  // authenticated arm request: arm instead of opening the door
    buzzer_.beep(Timing::kGrantedBeepMs);
    handlers_.onArmRequest();
    return;
  }
  armPendingUntil_ = 0;
  buzzer_.beep(Timing::kGrantedBeepMs);
  display_.flash(LcdText::kGranted, who);
  handlers_.onGranted(who, method);
}

void AccessControl::deny(const String& reason) {
  buzzer_.beep(Timing::kDeniedBeepMs);
  display_.flash(LcdText::kDenied, "");
  handlers_.onEvent(reason, false);
  if (++failedAttempts_ >= Security::kMaxFailedAttempts) {
    lockedUntil_ = millis() + Timing::kLockoutMs;
    handlers_.onEvent(format(Msg::kLockout, Timing::kLockoutMs / 1000), true);
  }
}

bool AccessControl::isLocked() {
  if (lockedUntil_ == 0) {
    return false;
  }
  if (static_cast<int32_t>(millis() - lockedUntil_) >= 0) {
    lockedUntil_ = 0;
    failedAttempts_ = 0;
    return false;
  }
  display_.flash(LcdText::kLocked, "");
  return true;
}

String AccessControl::uidToString(const MFRC522::Uid& uid) {
  String out;
  for (byte i = 0; i < uid.size; ++i) {
    if (i > 0) {
      out += ' ';
    }
    out += format("%02X", uid.uidByte[i]);
  }
  return out;
}
