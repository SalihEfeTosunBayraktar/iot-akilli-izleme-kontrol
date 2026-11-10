#pragma once
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// 16x2 I2C LCD: a status screen plus short temporary messages.
class Display {
 public:
  Display();
  void begin();
  void setStatus(const String& line1, const String& line2);
  void flash(const String& line1, const String& line2);
  void showInput(const char* prompt, size_t length);
  void update();

 private:
  void draw(const String& line1, const String& line2);

  LiquidCrystal_I2C lcd_;
  String status1_;
  String status2_;
  uint32_t flashUntil_ = 0;
};
