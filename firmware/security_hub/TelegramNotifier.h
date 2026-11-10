#pragma once
#include <Arduino.h>

// Sends a message to a Telegram chat through the Bot API. Blocks for ~1 s per message.
class TelegramNotifier {
 public:
  bool enabled() const;
  bool send(const String& text);

 private:
  static String urlEncode(const String& text);
};
