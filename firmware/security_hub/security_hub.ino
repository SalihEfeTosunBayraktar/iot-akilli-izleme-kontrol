// Security hub (ESP32-S3): door access, alarm logic, RF sensor nodes, web panel, Telegram.
#include <ESPmDNS.h>
#include <WiFi.h>
#include <time.h>
#include "AccessControl.h"
#include "AlarmSystem.h"
#include "Buzzer.h"
#include "CloudLink.h"
#include "DeviceSettings.h"
#include "Display.h"
#include "Door.h"
#include "EspNowReceiver.h"
#include "EventLog.h"
#include "Format.h"
#include "IndicatorLeds.h"
#include "NodeMonitor.h"
#include "Relay.h"
#include "SecretsLoader.h"
#include "SetupPortal.h"
#include "StatusReport.h"
#include "TelegramNotifier.h"
#include "TelemetryClient.h"
#include "TestSequence.h"
#include "WebPanel.h"
#include "config.h"

using namespace Config;

EventLog eventLog;
DeviceSettings settings;
TelegramNotifier notifier;

void report(const String& message, bool urgent) {
  eventLog.add(message, urgent);
  if (urgent) {
    notifier.send(message);
  }
}

Buzzer buzzer(Pin::kBuzzer);
Door door(Pin::kServo);
Relay relay(Pin::kRelay);
Display display;
IndicatorLeds indicators;
AlarmSystem alarmSystem(buzzer, report);
TestSequence testSequence(door, relay, indicators, report);
NodeMonitor nodeMonitor(report);
EspNowReceiver espNow;

extern SetupPortal portal;  // defined below: it needs the panel's web server

AccessControl accessControl(display, buzzer, {
    [](const String& who, const char* method) {
      alarmSystem.disarm(method);
      report(format(Msg::kGranted, who.c_str()), true);
      if (testSequence.awaitingAuth()) {
        testSequence.begin(method);
      } else {
        door.open();
      }
    },
    [] { alarmSystem.arm(Msg::kByKeypad); },
    [] { testSequence.requestAuth(); },
    [] { portal.requestSetupMode(Msg::kByKeypad); },
    report,
});

StatusReport statusReport(alarmSystem, nodeMonitor, eventLog, relay, door);
WebPanel panel({alarmSystem, relay, door, statusReport, testSequence, [] { portal.requestSetupMode(Msg::kByPanel); }, report});
SetupPortal portal(panel.server(), settings, report);
// Commands from the internet dashboard (PC) arrive in the telemetry reply.
bool runRemoteCommand(const String& command) {
  if (command == Remote::kArm) {
    alarmSystem.arm(Msg::kByRemote);
  } else if (command == Remote::kDisarm) {
    alarmSystem.disarm(Msg::kByRemote);
  } else if (command == Remote::kDoor) {
    door.open();
  } else if (command == Remote::kRelayOn || command == Remote::kRelayOff) {
    relay.set(command == Remote::kRelayOn);
  } else if (command == Remote::kTest) {
    testSequence.begin(Msg::kByRemote);
  } else {
    report(format(Msg::kRemoteUnknown, command.c_str()), false);
    return false;
  }
  report(format(Msg::kRemoteCommand, command.c_str()), false);
  return true;
}

TelemetryClient telemetry(statusReport, runRemoteCommand);
CloudLink cloud(statusReport, eventLog, {
    [](bool on) { relay.set(on); },
    [](bool open) { door.hold(open); },
    [] { return relay.isOn(); },
    [] { return door.isOpen(); },
});

bool connectWifi() {
  const String& ssid = settings.get(DeviceSettings::WifiSsid);
  if (ssid.isEmpty()) {
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid.c_str(), settings.get(DeviceSettings::WifiPassword).c_str());
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < Timing::kWifiConnectTimeoutMs) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

// While the setup AP is on, the LCD alternates between the normal status and the AP details.
bool showSetupInfo() {
  return portal.apActive() && (millis() / Timing::kLcdSetupToggleMs) % 2 == 1;
}

// No cloud means no Lambda decision, so a gas alarm opens the fan and valve locally;
// once the cloud is back it owns the actuators again.
void applyLocalSafety() {
  static bool active = false;
  const bool gas = alarmSystem.gasAlarm();
  if (!active && gas && !cloud.connected()) {
    active = true;
    relay.set(true);
    door.hold(true);
    report(Msg::kLocalSafetyOn, true);
  } else if (active && !gas) {
    active = false;
    relay.set(false);
    door.hold(false);
    report(Msg::kLocalSafetyOff, true);
  } else if (active && cloud.connected()) {
    active = false;
  }
}

void handleSensorData(const RfPacket::SensorData& data) {
  if (data.motion) {
    alarmSystem.onMotion();
  }
  alarmSystem.onGasLevel(data.gasLevel);
}

String statusLine() {
  if (showSetupInfo()) {
    return format(LcdText::kSetupApFormat, portal.apSuffix().c_str());
  }
  if (testSequence.running() || testSequence.awaitingAuth()) {
    return testSequence.running() ? LcdText::kTestRunning : LcdText::kTestWaitAuth;
  }
  if (alarmSystem.gasAlarm()) {
    return LcdText::kGasAlarm;
  }
  switch (alarmSystem.state()) {
    case AlarmState::Arming: return LcdText::kArming;
    case AlarmState::Armed: return LcdText::kArmed;
    case AlarmState::Triggered: return LcdText::kTriggered;
    default: return LcdText::kDisarmed;
  }
}

String environmentLine() {
  if (showSetupInfo()) {
    return WiFi.softAPIP().toString();
  }
  if (testSequence.running()) {
    return testSequence.stepLabel();
  }
  for (uint8_t id = 0; id < NodeMonitor::count(); ++id) {
    const NodeMonitor::Node& node = nodeMonitor.node(id);
    if (node.online) {
      return format(LcdText::kEnvFormat, node.data.temperature, node.data.humidity, node.data.gasLevel);
    }
  }
  return LcdText::kNoNodeData;
}

void setup() {
  Serial.begin(115200);
  buzzer.begin();
  relay.begin();
  indicators.begin();
  door.begin();
  display.begin();
  accessControl.begin();
  nodeMonitor.begin(Pin::kRfReceiver);
  settings.begin();
  telemetry.setUrl(settings.get(DeviceSettings::TelemetryUrl));
  cloud.begin(settings);

  const bool setupRequested = settings.takeSetupRequest();
  const bool connected = connectWifi();
  configTzTime(Time::kTimezone, Time::kNtpServer);  // syncs whenever the station gets online
  portal.begin(connected, setupRequested);          // before panel.begin(): AP clients get the setup page
  panel.begin();
  if (espNow.begin()) {
    report(format(Msg::kEspNowReady, WiFi.macAddress().c_str(), WiFi.channel()), false);
  } else {
    report(Msg::kEspNowFailed, true);
  }
  if (connected) {
    if (MDNS.begin(Network::kHostname)) {
      MDNS.addService("http", "tcp", 80);
    }
    report(format(Msg::kBoot, WiFi.localIP().toString().c_str()), true);
    display.flash(LcdText::kBootTitle, WiFi.localIP().toString());
  } else {
    report(Msg::kWifiFailed, false);
  }
}

void loop() {
  buzzer.update();
  door.update();
  display.update();
  accessControl.update();
  alarmSystem.update();
  nodeMonitor.update();

  RfPacket::SensorData data;
  if (nodeMonitor.poll(data)) {
    handleSensorData(data);
  }
  uint32_t raw;
  while (espNow.pop(raw)) {
    if (nodeMonitor.ingest(raw, data)) {
      handleSensorData(data);
    }
  }
  testSequence.update();
  if (!testSequence.running()) {
    indicators.update(nodeMonitor, alarmSystem.gasAlarm());
  }

  applyLocalSafety();

  panel.handle();
  portal.update();
  telemetry.update();
  cloud.update();

  static uint32_t lastRefresh = 0;
  if (millis() - lastRefresh >= Timing::kLcdRefreshMs) {
    lastRefresh = millis();
    display.setStatus(statusLine(), environmentLine());
  }
}
