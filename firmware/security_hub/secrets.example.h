#pragma once

// Copy this file as secrets.h and fill in your own values. secrets.h is git-ignored.
namespace Secrets {
constexpr const char* kWifiSsid = "WIFI_ADI";
constexpr const char* kWifiPassword = "WIFI_SIFRESI";

// Leave empty to disable Telegram notifications.
constexpr const char* kTelegramToken = "";   // from @BotFather
constexpr const char* kTelegramChatId = "";  // from @userinfobot

constexpr const char* kDoorPin = "1234";     // keypad password, digits only

constexpr const char* kPanelUser = "admin";  // web panel login
constexpr const char* kPanelPassword = "degistir";
}  // namespace Secrets

// Shared key the PC dashboard expects in the X-Api-Key header (must match pc-dashboard/config.json).
namespace Secrets {
constexpr const char* kTelemetryKey = "degistir";
}  // namespace Secrets

// WPA2 password of the setup access point (Guvenlik-Kurulum-XXXX), at least 8 characters.
namespace Secrets {
constexpr const char* kSetupApPassword = "degistir123";
}  // namespace Secrets
