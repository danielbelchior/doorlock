#pragma once

#include <Arduino.h>
#include <Preferences.h>

// WiFi credentials and admin login, persisted in NVS.
class Settings {
 public:
  void begin();

  const String& wifiSsid() const { return wifiSsid_; }
  const String& wifiPass() const { return wifiPass_; }
  const String& adminUser() const { return adminUser_; }
  const String& adminPass() const { return adminPass_; }

  bool hasWifi() const { return wifiSsid_.length() > 0; }
  bool usingDefaultAdminPass() const;

  // Return nullptr on success, otherwise a (Portuguese) message for the web UI.
  const char* setWifi(const String& ssid, const String& pass);
  const char* setAdmin(const String& user, const String& pass);

 private:
  String readString(const char* key, const char* fallback);

  Preferences prefs_;
  String wifiSsid_;
  String wifiPass_;
  String adminUser_;
  String adminPass_;
};
