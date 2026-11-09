#include "wifi_service.h"

#include <ESPmDNS.h>
#include <WiFi.h>

#include "config.h"
#include "settings.h"
#include "util.h"

WifiService::WifiService(Settings& settings) : settings_(settings) {}

void WifiService::begin() {
  const uint64_t mac = ESP.getEfuseMac();
  char ssid[33];
  snprintf(ssid, sizeof(ssid), "%s-%02X%02X", SETUP_AP_PREFIX,
           static_cast<unsigned>((mac >> 32) & 0xFF), static_cast<unsigned>((mac >> 40) & 0xFF));
  apSsid_ = ssid;

  WiFi.persistent(false);  // credentials live in Settings, not in the WiFi driver
  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  disconnectedSinceMs_ = millis();
  if (settings_.hasWifi()) {
    connectSta();
  } else {
    startAp();
  }
}

void WifiService::update() {
  if (apActive_) dns_.processNextRequest();

  const uint32_t now = millis();
  if (reconnectPending_) {
    reconnectPending_ = false;
    disconnectedSinceMs_ = now;
    connectSta();
  }

  const bool isConnected = connected();
  if (isConnected && !wasConnected_) {
    wasConnected_ = true;
    Serial.printf("[wifi] connected to '%s', IP %s\n", WiFi.SSID().c_str(), ip().c_str());
    startMdns();
    push(Event::Connected);
    if (apActive_) {
      // Keep the hotspot a little longer so the setup page can show the result.
      apStopScheduled_ = true;
      apStopAtMs_ = now + WIFI_AP_LINGER_MS;
    }
  } else if (!isConnected && wasConnected_) {
    wasConnected_ = false;
    disconnectedSinceMs_ = now;
    lastAttemptMs_ = now;
    Serial.println("[wifi] connection lost");
    push(Event::Disconnected);
  }

  if (isConnected) {
    if (apStopScheduled_ && timeReached(apStopAtMs_)) stopAp();
    return;
  }

  apStopScheduled_ = false;
  if (!settings_.hasWifi()) {
    if (!apActive_) startAp();
    return;
  }
  if (!apActive_ && timeReached(disconnectedSinceMs_ + WIFI_AP_FALLBACK_MS)) startAp();
  if (timeReached(lastAttemptMs_ + (apActive_ ? WIFI_RETRY_WITH_AP_MS : WIFI_RETRY_MS))) {
    connectSta();
  }
}

WifiService::Event WifiService::takeEvent() {
  for (uint8_t bit = 1; bit <= static_cast<uint8_t>(Event::ApStarted); bit <<= 1) {
    if (pendingEvents_ & bit) {
      pendingEvents_ &= ~bit;
      return static_cast<Event>(bit);
    }
  }
  return Event::None;
}

bool WifiService::connected() const {
  return WiFi.status() == WL_CONNECTED;
}

String WifiService::ip() const {
  return WiFi.localIP().toString();
}

String WifiService::apIp() const {
  return WiFi.softAPIP().toString();
}

int32_t WifiService::rssi() const {
  return WiFi.RSSI();
}

void WifiService::connectSta() {
  lastAttemptMs_ = millis();
  Serial.printf("[wifi] connecting to '%s'\n", settings_.wifiSsid().c_str());
  WiFi.disconnect();
  WiFi.begin(settings_.wifiSsid().c_str(), settings_.wifiPass().c_str());
}

void WifiService::startAp() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSsid_.c_str(), SETUP_AP_PASS);
  dns_.setErrorReplyCode(DNSReplyCode::NoError);
  dns_.start(53, "*", WiFi.softAPIP());
  apActive_ = true;
  Serial.printf("[wifi] setup hotspot '%s' started, IP %s\n", apSsid_.c_str(), apIp().c_str());
  push(Event::ApStarted);
}

void WifiService::stopAp() {
  dns_.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apActive_ = false;
  apStopScheduled_ = false;
  Serial.println("[wifi] setup hotspot stopped");
}

void WifiService::startMdns() {
  if (mdnsStarted_) return;
  if (MDNS.begin(WIFI_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    mdnsStarted_ = true;
    Serial.printf("[wifi] mDNS: http://%s.local/\n", WIFI_HOSTNAME);
  }
}
