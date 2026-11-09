#pragma once

#include <Arduino.h>
#include <Preferences.h>

// WiFi credentials, persisted in NVS.
class Settings {
 public:
  void begin();

  const String& wifiSsid() const { return wifiSsid_; }
  const String& wifiPass() const { return wifiPass_; }
  bool hasWifi() const { return wifiSsid_.length() > 0; }

  // Returns nullptr on success, otherwise a (Portuguese) message for the web UI.
  const char* setWifi(const String& ssid, const String& pass);

 private:
  String readString(const char* key, const char* fallback);

  Preferences prefs_;
  String wifiSsid_;
  String wifiPass_;
};
