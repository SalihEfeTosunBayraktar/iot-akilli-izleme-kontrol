#pragma once
#include <Arduino.h>

// Wi-Fi, AWS and telemetry settings stored in NVS; every value falls back to its compile-time
// default (secrets.h, optional aws_secrets.h, config.h) until the setup portal saves one.
// Values are loaded once and never change afterwards (saving is always followed by a restart),
// so the c_str() pointers handed to WiFiClientSecure stay valid for the program lifetime.
class DeviceSettings {
 public:
  enum Id : uint8_t {
    WifiSsid,
    WifiPassword,
    AwsEndpoint,
    ThingName,
    TopicPrefix,
    RootCa,
    DeviceCert,
    PrivateKey,
    TelemetryUrl,
    kCount
  };

  void begin();
  const String& get(Id id) const { return values_[id]; }
  bool cloudConfigured() const;
  static const char* key(Id id);  // NVS key, also the setup form field name

  // Writes the non-empty entries to NVS; empty entries keep the stored value.
  bool store(const String (&values)[kCount]);
  void requestSetup();      // one-shot flag read on the next boot
  bool takeSetupRequest();  // returns and clears the flag

 private:
  String values_[kCount];
};
