#include "TelegramNotifier.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "SecretsLoader.h"

namespace {
constexpr const char* kApiHost = "https://api.telegram.org/bot";
constexpr uint16_t kTimeoutMs = 5000;
}  // namespace

bool TelegramNotifier::enabled() const {
  return Secrets::kTelegramToken[0] != '\0' && Secrets::kTelegramChatId[0] != '\0';
}

bool TelegramNotifier::send(const String& text) {
  if (!enabled() || WiFi.status() != WL_CONNECTED) {
    return false;
  }
  WiFiClientSecure client;
  client.setInsecure();  // demo project: skip certificate pinning
  HTTPClient http;
  http.setTimeout(kTimeoutMs);
  if (!http.begin(client, String(kApiHost) + Secrets::kTelegramToken + "/sendMessage")) {
    return false;
  }
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  const int code = http.POST("chat_id=" + String(Secrets::kTelegramChatId) + "&text=" + urlEncode(text));
  http.end();
  return code == HTTP_CODE_OK;
}

String TelegramNotifier::urlEncode(const String& text) {
  static const char* kHex = "0123456789ABCDEF";
  String out;
  for (size_t i = 0; i < text.length(); ++i) {
    const uint8_t c = static_cast<uint8_t>(text[i]);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += static_cast<char>(c);
    } else {
      out += '%';
      out += kHex[c >> 4];
      out += kHex[c & 0x0F];
    }
  }
  return out;
}
