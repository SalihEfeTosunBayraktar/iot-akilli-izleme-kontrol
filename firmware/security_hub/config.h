#pragma once
#include <Arduino.h>

// Single place for every pin, timing, threshold and user-facing text of the hub.
// Pin numbers target the ESP32-S3-DevKitC-1 board.
namespace Config {

namespace Pin {
constexpr uint8_t kRfidSs = 10;
constexpr uint8_t kRfidMosi = 11;
constexpr uint8_t kRfidSck = 12;
constexpr uint8_t kRfidMiso = 13;
constexpr uint8_t kRfidRst = 14;
constexpr uint8_t kLcdSda = 8;
constexpr uint8_t kLcdScl = 9;
constexpr uint8_t kServo = 21;
constexpr uint8_t kBuzzer = 2;
constexpr uint8_t kRelay = 47;
constexpr uint8_t kRfReceiver = 1;
constexpr uint8_t kLedGas = 38;     // red
constexpr uint8_t kLedMotion = 39;  // yellow
constexpr uint8_t kLedTemp = 40;    // blue
// Ribbon is plugged in reversed on this build (measured key map), so rows/cols are swapped and mirrored.
inline uint8_t keypadRows[4] = {18, 17, 16, 15};
inline uint8_t keypadCols[4] = {7, 6, 5, 4};
}  // namespace Pin

namespace Keypad4x4 {
constexpr uint8_t kRows = 4;
constexpr uint8_t kCols = 4;
inline char keys[kRows][kCols] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};
constexpr char kSubmit = '#';
constexpr char kClear = '*';
constexpr char kArm = 'A';
constexpr char kTest = 'B';
constexpr char kSetup = 'D';  // door PIN + this key restarts into setup mode
}  // namespace Keypad4x4

namespace Lcd {
constexpr uint8_t kAddress = 0x27;  // try 0x3F if the screen stays blank
constexpr uint8_t kCols = 16;
constexpr uint8_t kRows = 2;
}  // namespace Lcd

namespace DoorServo {
constexpr uint8_t kClosedAngle = 0;
constexpr uint8_t kOpenAngle = 90;
}  // namespace DoorServo

namespace RelayOutput {
constexpr bool kActiveLow = true;  // most single-channel relay modules switch on LOW
}

namespace Timing {
constexpr uint32_t kDoorOpenMs = 5000;
constexpr uint32_t kServoSettleMs = 700;  // pulses stop after the move so the servo stops buzzing
constexpr uint32_t kArmDelayMs = 10000;
constexpr uint32_t kNodeTimeoutMs = 15000;
constexpr uint32_t kLockoutMs = 30000;
constexpr uint32_t kLcdMessageMs = 2500;
constexpr uint32_t kLcdRefreshMs = 1000;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr uint32_t kSirenToggleMs = 250;
constexpr uint16_t kKeyBeepMs = 40;
constexpr uint16_t kGrantedBeepMs = 150;
constexpr uint16_t kDeniedBeepMs = 800;
constexpr uint32_t kArmAuthTimeoutMs = 15000;   // card/PIN must follow the arm key within this time
constexpr uint32_t kTestAuthTimeoutMs = 15000;  // card/PIN must follow the test key within this time
constexpr uint32_t kTestGapMs = 2000;           // pause after the servo finishes
constexpr uint32_t kTestRelayMs = 10000;
constexpr uint32_t kTestLedStepMs = 150;
constexpr uint8_t kTestLedPasses = 3;
constexpr uint32_t kSetupWindowMs = 180000;      // setup AP stays on this long after every boot
constexpr uint32_t kSetupRestartDelayMs = 2000;  // lets the HTTP reply reach the phone before restarting
constexpr uint32_t kSetupStaRetryMs = 300000;    // home Wi-Fi retry while the setup AP has no clients
constexpr uint32_t kLcdSetupToggleMs = 3000;     // LCD alternates between status and setup AP info
}  // namespace Timing

namespace Security {
constexpr uint8_t kMaxFailedAttempts = 3;
constexpr uint8_t kPinMaxLength = 8;
constexpr uint8_t kGasAlarmLevel = 100;  // 0..255, compare with the panel value after MQ-2 warm-up
constexpr uint8_t kGasClearLevel = 80;   // hysteresis so the alarm does not flicker
constexpr uint8_t kMaxNodes = 4;
constexpr size_t kLogCapacity = 20;
}  // namespace Security

namespace Indicators {
constexpr uint8_t kTempHighC = 30;  // blue LED turns on at or above this temperature
}

namespace Telemetry {
constexpr bool kEnabled = true;
constexpr const char* kServerUrl = "http://192.168.1.91:8080/api/telemetry";  // PC running pc-dashboard
constexpr uint32_t kIntervalMs = 2000;  // also the worst-case delay of a remote command
constexpr uint32_t kRetryMs = 30000;   // back off while the PC is unreachable
constexpr uint8_t kMaxCommandsPerReply = 4;
constexpr uint16_t kTimeoutMs = 800;   // keep the main loop responsive
}  // namespace Telemetry

// Remote commands accepted from the PC dashboard (must match pc-dashboard allowed_commands).
namespace Remote {
constexpr const char* kArm = "arm";
constexpr const char* kDisarm = "disarm";
constexpr const char* kDoor = "door";
constexpr const char* kRelayOn = "relay_on";
constexpr const char* kRelayOff = "relay_off";
constexpr const char* kTest = "test";
}  // namespace Remote

namespace EspNow {
constexpr uint8_t kQueueLength = 8;  // packets buffered between the radio callback and loop()
}  // namespace EspNow

namespace Cloud {
constexpr bool kEnabled = false;  // AWS not used for now; set true to reconnect (stack is still deployed)
constexpr uint16_t kPort = 8883;              // MQTT over TLS
constexpr uint32_t kTelemetryIntervalMs = 5000;
constexpr uint32_t kRetryMs = 30000;          // reconnect back-off while AWS is unreachable
constexpr uint16_t kBufferSize = 4096;        // status JSON with the event log must fit in one packet
constexpr uint16_t kKeepAliveS = 60;
constexpr time_t kMinValidEpoch = 1700000000; // TLS needs a real clock; wait for NTP before connecting
}  // namespace Cloud

namespace Time {
constexpr const char* kTimezone = "<+03>-3";  // Turkiye, UTC+3
constexpr const char* kNtpServer = "pool.ntp.org";
}  // namespace Time

namespace Network {
constexpr const char* kHostname = "guvenlik";  // panel reachable at http://guvenlik.local
constexpr uint16_t kHttpPort = 80;               // one server for the panel and the setup portal
}

// Setup access point and captive portal used to change Wi-Fi/AWS settings without reflashing.
namespace Setup {
constexpr const char* kApSsidPrefix = "Guvenlik-Kurulum-";  // followed by the last two MAC bytes
constexpr uint8_t kApIp[4] = {192, 168, 4, 1};
constexpr uint8_t kApNetmask[4] = {255, 255, 255, 0};
constexpr uint16_t kDnsPort = 53;
constexpr size_t kMaxValueLength = 3900;  // NVS strings are limited to 4000 bytes
}  // namespace Setup

// Cards allowed to open the door. Scan an unknown card and copy its UID from the log.
struct Card {
  const char* uid;
  const char* owner;
};
constexpr Card kAuthorizedCards[] = {
    {"BB 76 10 05", "Kart 1"},
};

// LCD supports ASCII only, so these lines have no Turkish characters.
namespace LcdText {
constexpr const char* kBootTitle = "Guvenlik Sistemi";
constexpr const char* kBootWait = "Baslatiliyor...";
constexpr const char* kDisarmed = "Durum: KAPALI";
constexpr const char* kArming = "Durum: KURULUYOR";
constexpr const char* kArmed = "Durum: KURULU";
constexpr const char* kTriggered = "!!! ALARM !!!";
constexpr const char* kGasAlarm = "!! GAZ ALARMI !!";
constexpr const char* kPinPrompt = "Sifre:";
constexpr const char* kGranted = "Giris onaylandi";
constexpr const char* kDenied = "Erisim reddedildi";
constexpr const char* kLocked = "Kilitli, bekle";
constexpr const char* kNoNodeData = "Dugum bekleniyor";
constexpr const char* kTestWaitAuth = "Test: kart/sifre";
constexpr const char* kArmPrompt = "Kur: kart/sifre";
constexpr const char* kTestRunning = "TEST EDILIYOR";
constexpr const char* kTestServo = "Servo";
constexpr const char* kTestGap = "Bekleniyor";
constexpr const char* kTestRelay = "Role 10 sn";
constexpr const char* kTestLeds = "LED";
constexpr const char* kEnvFormat = "%uC %%%u Gaz:%u";
constexpr const char* kSetupApFormat = "AP:Kurulum-%s";  // %s = SSID suffix
constexpr const char* kSetupMode = "Kurulum modu";
constexpr const char* kRestarting = "Yeniden basliyor";
}  // namespace LcdText

// Log and Telegram messages (UTF-8).
namespace Msg {
constexpr const char* kBoot = "Güvenlik sistemi çalışmaya başladı. Panel: http://%s";
constexpr const char* kGranted = "Giriş onaylandı: %s";
constexpr const char* kUnknownCard = "Tanımsız kart okutuldu: %s";
constexpr const char* kWrongPin = "Hatalı şifre denemesi";
constexpr const char* kLockout = "Çok fazla hatalı deneme! Giriş %lu sn kilitlendi";
constexpr const char* kArmed = "Sistem kuruldu (%s)";
constexpr const char* kArming = "Sistem %lu sn içinde kurulacak (%s)";
constexpr const char* kDisarmed = "Sistem devre dışı bırakıldı (%s)";
constexpr const char* kIntrusion = "ALARM: Sistem kuruluyken hareket algılandı!";
constexpr const char* kGas = "ALARM: Gaz/duman seviyesi yüksek (%u)";
constexpr const char* kGasClear = "Gaz seviyesi normale döndü";
constexpr const char* kNodeOnline = "Düğüm %u bağlandı";
constexpr const char* kNodeOffline = "Uyarı: Düğüm %u sinyali kesildi";
constexpr const char* kDoorRemote = "Kapı panelden açıldı";
constexpr const char* kRelayState = "Röle %s (panel)";
constexpr const char* kOn = "açık";
constexpr const char* kOff = "kapalı";
constexpr const char* kByCard = "kart";
constexpr const char* kByPin = "şifre";
constexpr const char* kByKeypad = "tuş takımı";
constexpr const char* kByPanel = "panel";
constexpr const char* kByRemote = "uzaktan panel";
constexpr const char* kPinOwner = "Şifre ile giriş";
constexpr const char* kEspNowReady = "ESP-NOW alıcı hazır, merkez MAC: %s (kanal %d)";
constexpr const char* kEspNowFailed = "ESP-NOW başlatılamadı";
constexpr const char* kRemoteCommand = "Uzaktan komut uygulandı: %s";
constexpr const char* kRemoteUnknown = "Bilinmeyen uzaktan komut reddedildi: %s";
constexpr const char* kLocalSafetyOn = "Bulut bağlantısı yok: gaz alarmı nedeniyle fan ve vana yerelde açıldı";
constexpr const char* kLocalSafetyOff = "Gaz normale döndü: yerel fan ve vana kapatıldı";
constexpr const char* kTestStarted = "Modül testi başladı (%s)";
constexpr const char* kTestDone = "Modül testi tamamlandı";
constexpr const char* kWifiFailed = "Wi-Fi bağlantısı kurulamadı, çevrimdışı devam ediliyor";
constexpr const char* kSetupStarted = "Kurulum modu başladı: Wi-Fi %s, adres http://%s/setup";
constexpr const char* kSetupStopped = "Kurulum erişim noktası kapatıldı";
constexpr const char* kSetupApFailed = "Kurulum erişim noktası açılamadı (şifre en az 8 karakter olmalı)";
constexpr const char* kSetupRequested = "Kurulum modu istendi (%s), yeniden başlatılıyor";
constexpr const char* kSettingsSaved = "Ayarlar kaydedildi, yeniden başlatılıyor";
constexpr const char* kSettingsSavedWeb = "Kaydedildi, yeniden başlatılıyor...";
constexpr const char* kSettingsNotPem = "%s geçerli bir PEM değil (BEGIN/END satırı yok)";
constexpr const char* kSettingsTooLong = "Alan çok uzun (en fazla %u karakter)";
constexpr const char* kFieldRootCa = "Kök CA sertifikası";
constexpr const char* kFieldDeviceCert = "Cihaz sertifikası";
constexpr const char* kFieldPrivateKey = "Özel anahtar";
constexpr const char* kSettingsStoreFailed = "Ayarlar kaydedilemedi (NVS hatası)";
}  // namespace Msg

}  // namespace Config
