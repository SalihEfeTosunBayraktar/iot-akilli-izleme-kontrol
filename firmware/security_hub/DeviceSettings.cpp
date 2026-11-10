#include "DeviceSettings.h"
#include <Preferences.h>
#include "SecretsLoader.h"
#include "config.h"

#if __has_include("aws_secrets.h")
#include "aws_secrets.h"
#define AWS_DEFAULT(name) AwsSecrets::name
#else
#define AWS_DEFAULT(name) ""
#endif

namespace {
constexpr const char* kNamespace = "settings";
constexpr const char* kSetupFlag = "setupReq";
constexpr const char* kKeys[DeviceSettings::kCount] = {
    "ssid", "pass", "endpoint", "thing", "prefix", "rootca", "cert", "key", "telemetry",
};

const char* fallback(DeviceSettings::Id id) {
  switch (id) {
    case DeviceSettings::WifiSsid: return Secrets::kWifiSsid;
    case DeviceSettings::WifiPassword: return Secrets::kWifiPassword;
    case DeviceSettings::AwsEndpoint: return AWS_DEFAULT(kEndpoint);
    case DeviceSettings::ThingName: return AWS_DEFAULT(kThingName);
    case DeviceSettings::TopicPrefix: return AWS_DEFAULT(kTopicPrefix);
    case DeviceSettings::RootCa: return AWS_DEFAULT(kRootCa);
    case DeviceSettings::DeviceCert: return AWS_DEFAULT(kDeviceCert);
    case DeviceSettings::PrivateKey: return AWS_DEFAULT(kPrivateKey);
    case DeviceSettings::TelemetryUrl: return Config::Telemetry::kServerUrl;
    default: return "";
  }
}
}  // namespace

void DeviceSettings::begin() {
  Preferences prefs;
  const bool opened = prefs.begin(kNamespace, false);  // read-write so the namespace exists on first boot
  for (uint8_t i = 0; i < kCount; ++i) {
    const Id id = static_cast<Id>(i);
    values_[i] = opened && prefs.isKey(kKeys[i]) ? prefs.getString(kKeys[i]) : String(fallback(id));
  }
  prefs.end();
}

bool DeviceSettings::cloudConfigured() const {
  for (Id id : {AwsEndpoint, ThingName, RootCa, DeviceCert, PrivateKey}) {
    if (values_[id].isEmpty()) {
      return false;
    }
  }
  return true;
}

const char* DeviceSettings::key(Id id) {
  return id < kCount ? kKeys[id] : "";
}

bool DeviceSettings::store(const String (&values)[kCount]) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }
  bool ok = true;
  for (uint8_t i = 0; i < kCount; ++i) {
    if (!values[i].isEmpty() && prefs.putString(kKeys[i], values[i]) != values[i].length()) {
      ok = false;
    }
  }
  prefs.end();
  return ok;
}

void DeviceSettings::requestSetup() {
  Preferences prefs;
  if (prefs.begin(kNamespace, false)) {
    prefs.putBool(kSetupFlag, true);
    prefs.end();
  }
}

bool DeviceSettings::takeSetupRequest() {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }
  const bool requested = prefs.isKey(kSetupFlag) && prefs.getBool(kSetupFlag);
  if (requested) {
    prefs.remove(kSetupFlag);
  }
  prefs.end();
  return requested;
}
