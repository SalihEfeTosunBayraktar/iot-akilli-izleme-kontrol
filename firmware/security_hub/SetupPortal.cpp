#include "SetupPortal.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "Format.h"
#include "SecretsLoader.h"
#include "config.h"
#include "setup_page.h"

using namespace Config;

namespace {
IPAddress toIp(const uint8_t (&bytes)[4]) {
  return IPAddress(bytes[0], bytes[1], bytes[2], bytes[3]);
}

bool isPem(const String& value) {
  return value.indexOf("-----BEGIN") >= 0 && value.indexOf("-----END") >= 0;
}
}  // namespace

void SetupPortal::begin(bool staConnected, bool requested) {
  addRoutes();
  if (!startAp()) {
    return;
  }
  if (staConnected && !requested) {
    closeAt_ = millis() + Timing::kSetupWindowMs;
  } else {
    holdOpen();
  }
  WiFi.scanNetworks(true);  // results are ready when the page asks for them
}

void SetupPortal::update() {
  const uint32_t now = millis();
  if (restartPending_ && static_cast<int32_t>(now - restartAt_) >= 0) {
    ESP.restart();
  }
  if (!active_) {
    return;
  }
  dns_.processNextRequest();
  if (indefinite_) {
    retryStation(now);
    return;
  }
  if (static_cast<int32_t>(now - closeAt_) < 0 || WiFi.softAPgetStationNum() > 0) {
    return;  // never cut off a phone that is still filling in the form
  }
  if (WiFi.status() == WL_CONNECTED) {
    stopAp();
  } else {
    holdOpen();
  }
}

void SetupPortal::requestSetupMode(const char* by) {
  settings_.requestSetup();
  sink_(format(Msg::kSetupRequested, by), false);
  scheduleRestart();
}

void SetupPortal::addRoutes() {
  // AP clients opening "/" (typed address or captive sheet) get the setup page; LAN clients still get the panel.
  server_.on("/", HTTP_GET, [this] { redirectToSetup(); }).setFilter([this](WebServer&) {
    return active_ && fromAp();
  });
  server_.on("/setup", HTTP_GET, [this] {
    if (allowed()) {
      server_.send_P(200, "text/html; charset=utf-8", kSetupPage);
    }
  });
  server_.on("/setup/scan", HTTP_GET, [this] {
    if (allowed()) {
      sendScan();
    }
  });
  server_.on("/setup/save", HTTP_POST, [this] {
    if (allowed()) {
      save();
    }
  });
  // OS connectivity probes (generate_204, hotspot-detect.html, connecttest.txt, ...) and every other
  // unknown path from an AP client are redirected, which makes the phone open the captive portal.
  server_.onNotFound([this] {
    if (active_ && fromAp()) {
      redirectToSetup();
    } else {
      server_.send(404, "text/plain", "Not found");
    }
  });
}

bool SetupPortal::startAp() {
  WiFi.mode(WIFI_AP_STA);  // keeps an existing station connection
  uint8_t mac[6];
  WiFi.macAddress(mac);
  apSuffix_ = format("%02X%02X", mac[4], mac[5]);
  const String ssid = String(Setup::kApSsidPrefix) + apSuffix_;
  const IPAddress ip = toIp(Setup::kApIp);
  if (!WiFi.softAP(ssid.c_str(), Secrets::kSetupApPassword) ||
      !WiFi.softAPConfig(ip, ip, toIp(Setup::kApNetmask))) {
    sink_(Msg::kSetupApFailed, false);
    return false;
  }
  dns_.setErrorReplyCode(DNSReplyCode::NoError);
  dns_.start(Setup::kDnsPort, "*", ip);
  active_ = true;
  sink_(format(Msg::kSetupStarted, ssid.c_str(), ip.toString().c_str()), false);
  return true;
}

void SetupPortal::stopAp() {
  dns_.stop();
  WiFi.scanDelete();
  WiFi.softAPdisconnect(true);
  active_ = false;
  sink_(Msg::kSetupStopped, false);
}

void SetupPortal::holdOpen() {
  indefinite_ = true;
  // A station that keeps retrying scans every channel and drags the AP along, so retries are manual.
  WiFi.setAutoReconnect(false);
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
  }
  nextStaAttempt_ = millis() + Timing::kSetupStaRetryMs;
}

// Retries the home network now and then, but only while no phone is connected to the AP.
void SetupPortal::retryStation(uint32_t now) {
  if (staAttempting_) {
    if (WiFi.status() == WL_CONNECTED) {
      staAttempting_ = false;
    } else if (static_cast<int32_t>(now - staAttemptEnds_) >= 0) {
      WiFi.disconnect();
      staAttempting_ = false;
      nextStaAttempt_ = now + Timing::kSetupStaRetryMs;
    }
    return;
  }
  const String& ssid = settings_.get(DeviceSettings::WifiSsid);
  if (WiFi.status() == WL_CONNECTED || ssid.isEmpty() || static_cast<int32_t>(now - nextStaAttempt_) < 0 ||
      WiFi.softAPgetStationNum() > 0) {
    return;
  }
  WiFi.begin(ssid.c_str(), settings_.get(DeviceSettings::WifiPassword).c_str());
  staAttempting_ = true;
  staAttemptEnds_ = now + Timing::kWifiConnectTimeoutMs;
}

// The interface the request arrived on tells AP clients apart, even if the LAN uses 192.168.4.x too.
bool SetupPortal::fromAp() {
  return server_.client().localIP() == WiFi.softAPIP();
}

bool SetupPortal::allowed() {
  if ((active_ && fromAp()) || server_.authenticate(Secrets::kPanelUser, Secrets::kPanelPassword)) {
    return true;
  }
  server_.requestAuthentication();
  return false;
}

void SetupPortal::redirectToSetup() {
  server_.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/setup", true);
  server_.send(302, "text/plain", "");
}

// Async scan: the first call starts it, the page polls until the list is ready.
void SetupPortal::sendScan() {
  const int16_t count = WiFi.scanComplete();
  if (count == WIFI_SCAN_FAILED) {
    WiFi.scanNetworks(true);
  }
  JsonDocument doc;
  doc["scanning"] = count < 0;
  JsonArray list = doc["networks"].to<JsonArray>();
  for (int16_t i = 0; i < count; ++i) {
    const String ssid = WiFi.SSID(i);
    if (ssid.isEmpty()) {
      continue;
    }
    JsonObject entry;
    for (JsonObject seen : list) {
      if (ssid == seen["ssid"].as<const char*>()) {
        entry = seen;
        break;
      }
    }
    if (entry.isNull()) {
      entry = list.add<JsonObject>();
      entry["ssid"] = ssid;
      entry["rssi"] = WiFi.RSSI(i);
      entry["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
    } else if (WiFi.RSSI(i) > entry["rssi"].as<int>()) {
      entry["rssi"] = WiFi.RSSI(i);  // same SSID from several access points: keep the strongest
    }
  }
  if (count >= 0) {
    WiFi.scanDelete();
  }
  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void SetupPortal::save() {
  String values[DeviceSettings::kCount];
  for (uint8_t i = 0; i < DeviceSettings::kCount; ++i) {
    const DeviceSettings::Id id = static_cast<DeviceSettings::Id>(i);
    values[i] = server_.arg(DeviceSettings::key(id));
    values[i].replace("\r", "");
    if (id != DeviceSettings::WifiSsid && id != DeviceSettings::WifiPassword) {
      values[i].trim();  // SSIDs and passwords may legitimately contain edge spaces
    }
    if (values[i].length() > Setup::kMaxValueLength) {
      reply(400, false, format(Msg::kSettingsTooLong, static_cast<unsigned>(Setup::kMaxValueLength)));
      return;
    }
  }
  const struct {
    DeviceSettings::Id id;
    const char* label;
  } pems[] = {
      {DeviceSettings::RootCa, Msg::kFieldRootCa},
      {DeviceSettings::DeviceCert, Msg::kFieldDeviceCert},
      {DeviceSettings::PrivateKey, Msg::kFieldPrivateKey},
  };
  for (const auto& pem : pems) {
    if (!values[pem.id].isEmpty() && !isPem(values[pem.id])) {
      reply(400, false, format(Msg::kSettingsNotPem, pem.label));
      return;
    }
  }
  if (!settings_.store(values)) {
    reply(500, false, Msg::kSettingsStoreFailed);
    return;
  }
  reply(200, true, Msg::kSettingsSavedWeb);
  scheduleRestart();
  sink_(Msg::kSettingsSaved, true);
}

void SetupPortal::reply(int code, bool ok, const String& message) {
  JsonDocument doc;
  doc["ok"] = ok;
  doc["message"] = message;
  String body;
  serializeJson(doc, body);
  server_.send(code, "application/json", body);
}

void SetupPortal::scheduleRestart() {
  restartPending_ = true;
  restartAt_ = millis() + Timing::kSetupRestartDelayMs;
}
