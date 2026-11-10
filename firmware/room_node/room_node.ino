// Room sensor node (Arduino Nano): PIR + DHT11 + MQ-2, reports to the hub over 433 MHz.
#include <DHT.h>
#include <RCSwitch.h>
#include <RfPacket.h>
#include "config.h"

using namespace Config;

DHT dht(Pin::kDht, kDhtType);
RCSwitch rf;
RfPacket::SensorData reading{kNodeId, false, 0, 0, 0};

uint8_t readGasLevel() {
  if (millis() < Timing::kGasWarmupMs) {
    return 0;
  }
  return analogRead(Pin::kGas) >> 2;
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

void transmit() {
  digitalWrite(Pin::kStatusLed, HIGH);
  rf.send(RfPacket::encode(reading), RfPacket::kBitLength);
  digitalWrite(Pin::kStatusLed, LOW);
}

void showReading() {
  digitalWrite(Pin::kLedGas, reading.gasLevel >= Gas::kLocalAlarmLevel ? HIGH : LOW);
  digitalWrite(Pin::kLedTemp, reading.temperature >= Indicators::kTempHighC ? HIGH : LOW);
}

void setup() {
  pinMode(Pin::kPir, INPUT);
  pinMode(Pin::kBuzzer, OUTPUT);
  pinMode(Pin::kStatusLed, OUTPUT);
  pinMode(Pin::kLedGas, OUTPUT);
  pinMode(Pin::kLedMotion, OUTPUT);
  pinMode(Pin::kLedTemp, OUTPUT);
  dht.begin();
  rf.enableTransmit(Pin::kRfTransmitter);
  rf.setRepeatTransmit(Rf::kRepeatTransmit);
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
    digitalWrite(Pin::kBuzzer, reading.gasLevel >= Gas::kLocalAlarmLevel ? HIGH : LOW);
    showReading();
    transmit();
  }
}
