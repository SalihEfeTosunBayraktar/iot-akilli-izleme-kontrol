#include "CloudLink.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"

namespace {
constexpr const char* kShadowDelta = "$aws/things/%s/shadow/update/delta";
constexpr const char* kShadowUpdate = "$aws/things/%s/shadow/update";
}  // namespace

CloudLink::CloudLink(const StatusReport& status, const EventLog& log, Actuators actuators)
    : status_(status), log_(log), actuators_(actuators), mqtt_(net_) {
  mqtt_.setBufferSize(Config::Cloud::kBufferSize);
  mqtt_.setKeepAlive(Config::Cloud::kKeepAliveS);
  mqtt_.setCallback([this](char* t, uint8_t* p, unsigned int n) { onMessage(t, p, n); });
}

// Settings are loaded once and never change while running, so the PEM pointers stay valid.
void CloudLink::begin(const DeviceSettings& settings) {
  configured_ = Config::Cloud::kEnabled && settings.cloudConfigured();
  if (!configured_) {
    return;
  }
  thing_ = settings.get(DeviceSettings::ThingName);
  prefix_ = settings.get(DeviceSettings::TopicPrefix);
  net_.setCACert(settings.get(DeviceSettings::RootCa).c_str());
  net_.setCertificate(settings.get(DeviceSettings::DeviceCert).c_str());
  net_.setPrivateKey(settings.get(DeviceSettings::PrivateKey).c_str());
  mqtt_.setServer(settings.get(DeviceSettings::AwsEndpoint).c_str(), Config::Cloud::kPort);
}

void CloudLink::update() {
  if (!configured_ || WiFi.status() != WL_CONNECTED) {
    return;
  }
  const uint32_t now = millis();
  if (!mqtt_.connected()) {
    if (static_cast<int32_t>(now - nextConnect_) < 0 || time(nullptr) < Config::Cloud::kMinValidEpoch) {
      return;
    }
    if (!connect()) {
      nextConnect_ = now + Config::Cloud::kRetryMs;
      return;
    }
  }
  mqtt_.loop();
  publishNewEvents();
  if (static_cast<int32_t>(now - nextTelemetry_) >= 0) {
    nextTelemetry_ = now + Config::Cloud::kTelemetryIntervalMs;
    mqtt_.publish(topic("telemetry").c_str(), status_.toJson().c_str());
  }
}

bool CloudLink::connect() {
  char shadow[96];
  if (!mqtt_.connect(thing_.c_str())) {
    return false;
  }
  snprintf(shadow, sizeof(shadow), kShadowDelta, thing_.c_str());
  mqtt_.subscribe(shadow, 1);
  reportState();
  return true;
}

void CloudLink::publishNewEvents() {
  const uint32_t pending = min<uint32_t>(log_.total() - publishedEvents_, log_.size());
  for (int32_t i = static_cast<int32_t>(pending) - 1; i >= 0; --i) {  // oldest first
    const EventLog::Entry& e = log_.newest(i);
    JsonDocument doc;
    doc["t"] = e.time;
    doc["m"] = e.text;
    doc["u"] = e.urgent;
    doc["seq"] = log_.total() - i;  // makes the cloud key unique when events arrive in the same millisecond
    String body;
    serializeJson(doc, body);
    if (!mqtt_.publish(topic("events").c_str(), body.c_str())) {
      return;  // the rest goes out on the next loop
    }
    publishedEvents_ = log_.total() - i;  // count each success so a failure never resends earlier ones
  }
}

// Shadow delta: {"state":{"fan":"on","valve":"open"},...}
void CloudLink::onMessage(char*, uint8_t* payload, unsigned int length) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, length)) {
    return;
  }
  JsonObject state = doc["state"];
  if (state["fan"].is<const char*>()) {
    actuators_.setFan(strcmp(state["fan"], "on") == 0);
  }
  if (state["valve"].is<const char*>()) {
    actuators_.setValve(strcmp(state["valve"], "open") == 0);
  }
  reportState();
}

void CloudLink::reportState() {
  JsonDocument doc;
  doc["state"]["reported"]["fan"] = actuators_.fanOn() ? "on" : "off";
  doc["state"]["reported"]["valve"] = actuators_.valveOpen() ? "open" : "closed";
  String body;
  serializeJson(doc, body);
  char shadow[96];
  snprintf(shadow, sizeof(shadow), kShadowUpdate, thing_.c_str());
  mqtt_.publish(shadow, body.c_str());
}

String CloudLink::topic(const char* suffix) const {
  return prefix_ + "/" + thing_ + "/" + suffix;
}
