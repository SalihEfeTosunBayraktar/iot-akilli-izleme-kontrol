#pragma once
#include <Arduino.h>
#include "config.h"

// Fixed-size ring buffer of timestamped events shown on the web panel.
class EventLog {
 public:
  struct Entry {
    String time;
    String text;
    bool urgent = false;
  };

  void add(const String& text, bool urgent);
  size_t size() const { return count_; }
  uint32_t total() const { return total_; }  // events ever added; lets readers detect new ones
  const Entry& newest(size_t index) const;  // 0 = most recent

 private:
  static String timestamp();

  Entry entries_[Config::Security::kLogCapacity];
  size_t head_ = 0;
  size_t count_ = 0;
  uint32_t total_ = 0;
};
