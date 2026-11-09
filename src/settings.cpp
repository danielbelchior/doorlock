#include "settings.h"

#include "config.h"

namespace {

constexpr const char* NVS_NAMESPACE = "settings";
constexpr const char* KEY_WIFI_SSID = "wifi_ssid";
constexpr const char* KEY_WIFI_PASS = "wifi_pass";
constexpr const char* KEY_ADMIN_USER = "admin_user";
constexpr const char* KEY_ADMIN_PASS = "admin_pass";

bool isPrintableAscii(const String& text) {
  for (size_t i = 0; i < text.length(); i++) {
    const uint8_t c = static_cast<uint8_t>(text[i]);
    if (c < 32 || c > 126) return false;
  }
  return true;
}

}  // namespace

void Settings::begin() {
  prefs_.begin(NVS_NAMESPACE, false);
  wifiSsid_ = readString(KEY_WIFI_SSID, DEFAULT_WIFI_SSID);
  wifiPass_ = readString(KEY_WIFI_PASS, DEFAULT_WIFI_PASS);
  adminUser_ = readString(KEY_ADMIN_USER, DEFAULT_ADMIN_USER);
  adminPass_ = readString(KEY_ADMIN_PASS, DEFAULT_ADMIN_PASS);
}

bool Settings::usingDefaultAdminPass() const {
  return adminPass_ == DEFAULT_ADMIN_PASS;
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

const char* Settings::setAdmin(const String& userIn, const String& pass) {
  String user = userIn;
  user.trim();
  if (user.length() < 1 || user.length() > 32 || !isPrintableAscii(user) || user.indexOf(':') >= 0) {
    return "Usuário inválido: use 1 a 32 caracteres, sem acentos e sem ':'.";
  }
  if (pass.length() < 4 || pass.length() > 64 || !isPrintableAscii(pass)) {
    return "Senha inválida: use 4 a 64 caracteres, sem acentos.";
  }

  bool ok = prefs_.putString(KEY_ADMIN_USER, user) == user.length();
  ok = prefs_.putString(KEY_ADMIN_PASS, pass) == pass.length() && ok;
  if (!ok) return "Erro ao gravar na memória do dispositivo.";

  adminUser_ = user;
  adminPass_ = pass;
  return nullptr;
}

String Settings::readString(const char* key, const char* fallback) {
  if (!prefs_.isKey(key)) return String(fallback);
  return prefs_.getString(key, fallback);
}
