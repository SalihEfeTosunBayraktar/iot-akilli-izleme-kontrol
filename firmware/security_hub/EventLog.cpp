#include "EventLog.h"
#include <time.h>

void EventLog::add(const String& text, bool urgent) {
  const Entry entry{timestamp(), text, urgent};
  Serial.println(entry.time + " " + entry.text);
  entries_[head_] = entry;
  head_ = (head_ + 1) % Config::Security::kLogCapacity;
  ++total_;
  if (count_ < Config::Security::kLogCapacity) {
    ++count_;
  }
}

const EventLog::Entry& EventLog::newest(size_t index) const {
  const size_t cap = Config::Security::kLogCapacity;
  return entries_[(head_ + cap - 1 - index) % cap];
}

String EventLog::timestamp() {
  struct tm now;
  if (getLocalTime(&now, 0)) {
    char buffer[12];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &now);
    return String(buffer);
  }
  return "+" + String(millis() / 1000) + "s";
}
