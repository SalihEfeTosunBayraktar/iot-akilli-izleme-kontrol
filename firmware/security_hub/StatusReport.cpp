#include "StatusReport.h"
#include <ArduinoJson.h>
#include "config.h"

String StatusReport::toJson() const {
  JsonDocument doc;
  doc["state"] = alarm_.stateCode();
  doc["gasAlarm"] = alarm_.gasAlarm();
  doc["relay"] = relay_.isOn();
  doc["door"] = door_.isOpen();
  doc["armLeft"] = (alarm_.armingRemainingMs() + 999) / 1000;
  doc["armDelay"] = Config::Timing::kArmDelayMs / 1000;
  doc["gasLimit"] = Config::Security::kGasAlarmLevel;
  doc["tempHigh"] = Config::Indicators::kTempHighC;
  doc["uptime"] = millis() / 1000;

  JsonArray nodes = doc["nodes"].to<JsonArray>();
  const uint32_t now = millis();
  for (uint8_t id = 0; id < NodeMonitor::count(); ++id) {
    const NodeMonitor::Node& node = nodes_.node(id);
    if (node.lastSeen == 0) {
      continue;
    }
    JsonObject n = nodes.add<JsonObject>();
    n["id"] = id;
    n["online"] = node.online;
    n["temp"] = node.data.temperature;
    n["hum"] = node.data.humidity;
    n["gas"] = node.data.gasLevel;
    n["motion"] = node.data.motion;
    n["age"] = (now - node.lastSeen) / 1000;
  }

  JsonArray log = doc["log"].to<JsonArray>();
  for (size_t i = 0; i < log_.size(); ++i) {
    JsonObject e = log.add<JsonObject>();
    e["t"] = log_.newest(i).time;
    e["m"] = log_.newest(i).text;
    e["u"] = log_.newest(i).urgent;
  }

  String body;
  serializeJson(doc, body);
  return body;
}
