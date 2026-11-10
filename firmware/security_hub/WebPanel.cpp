#include "WebPanel.h"
#include "Format.h"
#include "SecretsLoader.h"
#include "config.h"
#include "web_page.h"

void WebPanel::begin() {
  route("/", HTTP_GET, [this] { server_.send_P(200, "text/html; charset=utf-8", kWebPage); });
  route("/api/status", HTTP_GET, [] {});
  route("/api/arm", HTTP_POST, [this] { d_.alarm.arm(Config::Msg::kByPanel); });
  route("/api/disarm", HTTP_POST, [this] { d_.alarm.disarm(Config::Msg::kByPanel); });
  route("/api/door", HTTP_POST, [this] {
    d_.door.open();
    d_.sink(Config::Msg::kDoorRemote, true);
  });
  route("/api/setup", HTTP_POST, [this] { d_.onSetupRequest(); });
  route("/api/test", HTTP_POST, [this] { d_.test.begin(Config::Msg::kByPanel); });
  route("/api/relay", HTTP_POST, [this] {
    const bool on = server_.arg("on") == "1";
    d_.relay.set(on);
    d_.sink(format(Config::Msg::kRelayState, on ? Config::Msg::kOn : Config::Msg::kOff), false);
  });
  server_.begin();
}

// Every route is authenticated; API routes answer with the fresh status after acting.
void WebPanel::route(const char* path, HTTPMethod method, std::function<void()> action) {
  const bool isPage = strcmp(path, "/") == 0;
  server_.on(path, method, [this, action, isPage] {
    if (!authorized()) {
      server_.requestAuthentication();
      return;
    }
    action();
    if (!isPage) {
      sendStatus();
    }
  });
}

bool WebPanel::authorized() {
  return server_.authenticate(Secrets::kPanelUser, Secrets::kPanelPassword);
}

void WebPanel::sendStatus() {
  server_.send(200, "application/json", d_.status.toJson());
}
