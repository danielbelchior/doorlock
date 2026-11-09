#include "settings.h"

#include "config.h"

namespace {

constexpr const char* NVS_NAMESPACE = "settings";
constexpr const char* KEY_WIFI_SSID = "wifi_ssid";
constexpr const char* KEY_WIFI_PASS = "wifi_pass";

}  // namespace

void Settings::begin() {
  prefs_.begin(NVS_NAMESPACE, false);
  wifiSsid_ = readString(KEY_WIFI_SSID, DEFAULT_WIFI_SSID);
  wifiPass_ = readString(KEY_WIFI_PASS, DEFAULT_WIFI_PASS);
}

const char* Settings::setWifi(const String& ssid, const String& pass) {
  if (ssid.length() < 1 || ssid.length() > 32) {
    return "O nome da rede deve ter de 1 a 32 caracteres.";
  }
  if (pass.length() > 0 && (pass.length() < 8 || pass.length() > 63)) {
    return "A senha WiFi deve ter de 8 a 63 caracteres (ou ficar vazia em redes abertas).";
  }

  bool ok = prefs_.putString(KEY_WIFI_SSID, ssid) == ssid.length();
  ok = prefs_.putString(KEY_WIFI_PASS, pass) == pass.length() && ok;
  if (!ok) return "Erro ao gravar na memória do dispositivo.";

  wifiSsid_ = ssid;
  wifiPass_ = pass;
  return nullptr;
}

String Settings::readString(const char* key, const char* fallback) {
  if (!prefs_.isKey(key)) return String(fallback);
  return prefs_.getString(key, fallback);
}
