// Room sensor node (ESP32-WROOM mini): PIR + DHT11 + MQ-2, reports to the hub over ESP-NOW.
// Same packet format and scales as the Nano node, so the hub treats both the same way.
#include <DHT.h>
#include <RfPacket.h>
#include "EspNowSender.h"
#include "config.h"

using namespace Config;

DHT dht(Pin::kDht, kDhtType);
EspNowSender hubLink;
RfPacket::SensorData reading{kNodeId, false, 0, 0, 0};

uint8_t readGasLevel() {
  if (millis() < Timing::kGasWarmupMs) {
    return 0;
  }
  const float sensorMv = analogReadMilliVolts(Pin::kGas) * Gas::kDividerRatio;
  return static_cast<uint8_t>(constrain(sensorMv * 255.0f / Gas::kSensorFullScaleMv, 0.0f, 255.0f));
}

void readSensors() {
  reading.motion = digitalRead(Pin::kPir) == HIGH;
  reading.gasLevel = readGasLevel();
  const float temperature = dht.readTemperature();
  const float humidity = dht.readHumidity();
  if (!isnan(temperature) && !isnan(humidity)) {  // keep the last good value on a failed read
    reading.temperature = static_cast<uint8_t>(constrain(temperature, 0, 127));
    reading.humidity = static_cast<uint8_t>(constrain(humidity, 0, 100));
  }
}

void showReading() {
  digitalWrite(Pin::kLedGas, reading.gasLevel >= Gas::kLocalAlarmLevel ? HIGH : LOW);
  digitalWrite(Pin::kLedTemp, reading.temperature >= Indicators::kTempHighC ? HIGH : LOW);
}

void transmit() {
  if (hubLink.send(RfPacket::encode(reading))) {
    digitalWrite(Pin::kStatusLed, HIGH);
    delay(Timing::kStatusBlinkMs);
    digitalWrite(Pin::kStatusLed, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  for (uint8_t pin : {Pin::kLedGas, Pin::kLedMotion, Pin::kLedTemp, Pin::kStatusLed}) {
    pinMode(pin, OUTPUT);
  }
  pinMode(Pin::kPir, INPUT);
  dht.begin();
  if (!hubLink.begin(kHubMac)) {
    Serial.println(Text::kInitFailed);
  }
}

void loop() {
  static uint32_t lastSend = 0;
  static bool lastMotion = false;

  const bool motion = digitalRead(Pin::kPir) == HIGH;
  const bool motionStarted = motion && !lastMotion;
  lastMotion = motion;
  digitalWrite(Pin::kLedMotion, motion ? HIGH : LOW);

  if (motionStarted || millis() - lastSend >= Timing::kSendIntervalMs) {
    lastSend = millis();
    readSensors();
    showReading();
    transmit();
  }
}
