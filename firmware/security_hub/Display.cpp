#include "Display.h"
#include <Wire.h>
#include "config.h"

Display::Display() : lcd_(Config::Lcd::kAddress, Config::Lcd::kCols, Config::Lcd::kRows) {}

void Display::begin() {
  Wire.begin(Config::Pin::kLcdSda, Config::Pin::kLcdScl);
  lcd_.init();
  lcd_.backlight();
  draw(Config::LcdText::kBootTitle, Config::LcdText::kBootWait);
}

void Display::setStatus(const String& line1, const String& line2) {
  if (line1 == status1_ && line2 == status2_) {
    return;
  }
  status1_ = line1;
  status2_ = line2;
  if (flashUntil_ == 0) {
    draw(status1_, status2_);
  }
}

void Display::flash(const String& line1, const String& line2) {
  flashUntil_ = millis() + Config::Timing::kLcdMessageMs;
  draw(line1, line2);
}

void Display::showInput(const char* prompt, size_t length) {
  String masked;
  for (size_t i = 0; i < length; ++i) {
    masked += '*';
  }
  flash(prompt, masked);
}

void Display::update() {
  if (flashUntil_ != 0 && static_cast<int32_t>(millis() - flashUntil_) >= 0) {
    flashUntil_ = 0;
    draw(status1_, status2_);
  }
}

void Display::draw(const String& line1, const String& line2) {
  lcd_.clear();
  lcd_.setCursor(0, 0);
  lcd_.print(line1.substring(0, Config::Lcd::kCols));
  lcd_.setCursor(0, 1);
  lcd_.print(line2.substring(0, Config::Lcd::kCols));
}
