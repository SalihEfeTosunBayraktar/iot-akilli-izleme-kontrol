#include "TelemetryClient.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "SecretsLoader.h"
#include "config.h"

void TelemetryClient::update() {
  using namespace Config::Telemetry;
  const uint32_t now = millis();
  if (!kEnabled || url_.isEmpty() || WiFi.status() != WL_CONNECTED || static_cast<int32_t>(now - nextAttempt_) < 0) {
    return;
  }
  String reply;
  const bool ok = post(withAcks(status_.toJson()), reply);
  nextAttempt_ = now + (ok ? kIntervalMs : kRetryMs);
  if (ok) {
    ackCount_ = 0;  // delivered; results of the commands below go out with the next upload
    runCommands(reply);
  }
}

bool TelemetryClient::post(const String& body, String& reply) {
  HTTPClient http;
  http.setConnectTimeout(Config::Telemetry::kTimeoutMs);
  http.setTimeout(Config::Telemetry::kTimeoutMs);
  if (!http.begin(url_)) {
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Api-Key", Secrets::kTelemetryKey);
  const int code = http.POST(body);
  if (code == HTTP_CODE_OK) {
    reply = http.getString();
  }
  http.end();
  return code >= 200 && code < 300;
}

String TelemetryClient::withAcks(const String& status) const {
  if (ackCount_ == 0) {
    return status;
  }
  JsonDocument doc;
  if (deserializeJson(doc, status)) {
    return status;
  }
  JsonArray acks = doc["acks"].to<JsonArray>();
  for (uint8_t i = 0; i < ackCount_; ++i) {
    JsonObject a = acks.add<JsonObject>();
    a["id"] = acks_[i].id;
    a["ok"] = acks_[i].ok;
  }
  String body;
  serializeJson(doc, body);
  return body;
}

// Reply: {"commands":[{"id":7,"cmd":"relay_on"}]}. Malformed replies are ignored.
void TelemetryClient::runCommands(const String& reply) {
  JsonDocument doc;
  if (reply.isEmpty() || deserializeJson(doc, reply)) {
    return;
  }
  for (JsonObject command : doc["commands"].as<JsonArray>()) {
    if (ackCount_ >= Config::Telemetry::kMaxCommandsPerReply || !command["id"].is<uint32_t>() || !command["cmd"].is<const char*>()) {
      continue;  // anything not acknowledged simply expires on the PC side
    }
    acks_[ackCount_++] = {command["id"].as<uint32_t>(), onCommand_(command["cmd"].as<const char*>())};
  }
}
