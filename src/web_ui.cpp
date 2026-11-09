#include "web_ui.h"

#include <Update.h>
#include <WiFi.h>

#include "config.h"
#include "settings.h"
#include "util.h"
#include "web_page.h"
#include "wifi_service.h"

namespace {

String jsonString(const String& value) {
  String out;
  out.reserve(value.length() + 2);
  out += '"';
  for (size_t i = 0; i < value.length(); i++) {
    const char c = value[i];
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<uint8_t>(c) < 0x20) {
          char escaped[7];
          snprintf(escaped, sizeof(escaped), "\\u%04x", static_cast<unsigned>(c));
          out += escaped;
        } else {
          out += c;
        }
    }
  }
  out += '"';
  return out;
}

const char* jsonBool(bool value) {
  return value ? "true" : "false";
}

// Strict parse: String::toInt() would turn garbage into 0, the first user.
bool parseIndex(const String& text, int& out) {
  if (text.length() < 1 || text.length() > 3) return false;
  for (size_t i = 0; i < text.length(); i++) {
    if (text[i] < '0' || text[i] > '9') return false;
  }
  out = text.toInt();
  return true;
}

// Missing or unrecognized values keep the fallback.
bool parseFlag(const String& text, bool fallback) {
  if (text == "1" || text == "true" || text == "on") return true;
  if (text == "0" || text == "false" || text == "off") return false;
  return fallback;
}

}  // namespace

WebUi::WebUi(UserStore& users, Settings& settings, WifiService& wifi)
    : server_(80), users_(users), settings_(settings), wifi_(wifi) {}

void WebUi::begin() {
  server_.on("/", HTTP_GET, [this]() { handleIndex(); });
  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/users", HTTP_GET, [this]() { handleUsersList(); });
  server_.on("/api/users", HTTP_POST, [this]() { handleUserAdd(); });
  server_.on("/api/users/update", HTTP_POST, [this]() { handleUserUpdate(); });
  server_.on("/api/users/delete", HTTP_POST, [this]() { handleUserDelete(); });
  server_.on("/api/wifi/scan", HTTP_POST, [this]() { handleWifiScanStart(); });
  server_.on("/api/wifi/scan", HTTP_GET, [this]() { handleWifiScanResult(); });
  server_.on("/api/wifi", HTTP_POST, [this]() { handleWifiSave(); });
  server_.on("/api/reboot", HTTP_POST, [this]() { handleReboot(); });
  server_.on("/api/update", HTTP_POST, [this]() { handleOtaDone(); }, [this]() { handleOtaUpload(); });
  server_.onNotFound([this]() { handleNotFound(); });
  server_.begin();
  Serial.println("[web] server started on port 80");
}

void WebUi::update() {
  server_.handleClient();
  if (rebootPending_ && timeReached(rebootAtMs_)) {
    Serial.println("[web] restarting");
    ESP.restart();
  }
}

void WebUi::sendJson(int code, const String& body) {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(code, "application/json", body);
}

void WebUi::sendOk() {
  sendJson(200, "{\"ok\":true}");
}

void WebUi::sendError(int code, const String& message) {
  sendJson(code, String("{\"error\":") + jsonString(message) + "}");
}

void WebUi::sendUserResult(UserResult result) {
  if (result == UserResult::Ok) {
    sendOk();
  } else {
    sendError(result == UserResult::StorageError ? 500 : 400, userResultMessage(result));
  }
}

void WebUi::scheduleReboot(uint32_t delayMs) {
  rebootPending_ = true;
  rebootAtMs_ = millis() + delayMs;
}

void WebUi::handleIndex() {
  server_.sendHeader("Cache-Control", "no-cache");
  server_.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void WebUi::handleStatus() {
  const bool connected = wifi_.connected();
  String json;
  json.reserve(512);
  json += "{\"version\":\"" FIRMWARE_VERSION "\",\"uptime\":";
  json += String(millis() / 1000);
  json += ",\"heap\":";
  json += String(ESP.getFreeHeap());
  json += ",\"hostname\":";
  json += jsonString(WIFI_HOSTNAME);
  json += ",\"users\":{\"count\":";
  json += String(users_.count());
  json += ",\"max\":";
  json += String(MAX_USERS);
  json += "},\"wifi\":{\"configured\":";
  json += jsonBool(settings_.hasWifi());
  json += ",\"connected\":";
  json += jsonBool(connected);
  json += ",\"ssid\":";
  json += jsonString(settings_.wifiSsid());
  json += ",\"ip\":";
  json += jsonString(connected ? wifi_.ip() : String());
  json += ",\"rssi\":";
  json += String(connected ? static_cast<int>(wifi_.rssi()) : 0);
  json += ",\"ap\":";
  json += jsonBool(wifi_.apActive());
  json += ",\"apSsid\":";
  json += jsonString(wifi_.apSsid());
  json += ",\"apIp\":";
  json += jsonString(wifi_.apActive() ? wifi_.apIp() : String());
  json += "}}";
  sendJson(200, json);
}

void WebUi::handleUsersList() {
  String json;
  json.reserve(64 + users_.count() * 64);
  json += "{\"max\":";
  json += String(MAX_USERS);
  json += ",\"users\":[";
  for (uint8_t i = 0; i < users_.count(); i++) {
    const User& user = users_.at(i);
    if (i > 0) json += ',';
    json += "{\"name\":";
    json += jsonString(user.name);
    json += ",\"pin\":";
    json += jsonString(user.pin);
    json += ",\"challenge\":";
    json += jsonBool(users_.challenge(i));
    json += '}';
  }
  json += "]}";
  sendJson(200, json);
}

void WebUi::handleUserAdd() {
  sendUserResult(
      users_.add(server_.arg("name"), server_.arg("pin"), parseFlag(server_.arg("challenge"), true)));
}

void WebUi::handleUserUpdate() {
  int index;
  if (!parseIndex(server_.arg("index"), index)) {
    sendUserResult(UserResult::NotFound);
    return;
  }
  const bool current = index < users_.count() ? users_.challenge(static_cast<uint8_t>(index)) : true;
  sendUserResult(users_.update(index, server_.arg("name"), server_.arg("pin"),
                               parseFlag(server_.arg("challenge"), current)));
}

void WebUi::handleUserDelete() {
  int index;
  if (!parseIndex(server_.arg("index"), index)) {
    sendUserResult(UserResult::NotFound);
    return;
  }
  sendUserResult(users_.remove(index));
}

void WebUi::handleWifiScanStart() {
  WiFi.scanDelete();
  if (WiFi.scanNetworks(true) == WIFI_SCAN_FAILED) {
    sendError(503, "Não foi possível procurar redes agora. Tente novamente.");
    return;
  }
  sendOk();
}

void WebUi::handleWifiScanResult() {
  const int count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    sendJson(200, "{\"scanning\":true}");
    return;
  }
  if (count < 0) {
    sendError(503, "A procura de redes falhou. Tente novamente.");
    return;
  }

  String json = "{\"scanning\":false,\"networks\":[";
  bool first = true;
  for (int i = 0; i < count; i++) {
    const String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) continue;  // hidden network
    if (!first) json += ',';
    first = false;
    json += "{\"ssid\":";
    json += jsonString(ssid);
    json += ",\"rssi\":";
    json += String(static_cast<int>(WiFi.RSSI(i)));
    json += ",\"secure\":";
    json += jsonBool(WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    json += '}';
  }
  json += "]}";
  sendJson(200, json);
}

void WebUi::handleWifiSave() {
  const char* error = settings_.setWifi(server_.arg("ssid"), server_.arg("pass"));
  if (error) {
    sendError(400, error);
    return;
  }
  sendOk();
  wifi_.reconnect();
}

void WebUi::handleReboot() {
  sendOk();
  scheduleReboot(1000);
}

// Runs for each chunk of the multipart upload, before handleOtaDone().
void WebUi::handleOtaUpload() {
  HTTPUpload& upload = server_.upload();

  switch (upload.status) {
    case UPLOAD_FILE_START:
      otaError_ = "";
      otaStarted_ = true;
      Serial.printf("[ota] receiving %s\n", upload.filename.c_str());
      if (otaStart_) otaStart_();
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) otaError_ = Update.errorString();
      break;

    case UPLOAD_FILE_WRITE:
      if (!otaStarted_ || otaError_.length() > 0) return;
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        otaError_ = Update.errorString();
      }
      break;

    case UPLOAD_FILE_END:
      if (!otaStarted_ || otaError_.length() > 0) return;
      if (!Update.end(true)) otaError_ = Update.errorString();
      break;

    case UPLOAD_FILE_ABORTED:
      if (otaStarted_) otaError_ = "Envio interrompido.";
      break;

    default:
      break;
  }
}

void WebUi::handleOtaDone() {
  const bool started = otaStarted_;
  const bool ok = started && otaError_.length() == 0;
  otaStarted_ = false;

  if (!ok && Update.isRunning()) Update.abort();
  if (started && otaEnd_) otaEnd_(ok);

  if (ok) {
    Serial.println("[ota] update installed");
    sendOk();
    scheduleReboot(1500);
  } else if (!started) {
    sendError(400, "Nenhum arquivo recebido.");
  } else {
    Serial.printf("[ota] failed: %s\n", otaError_.c_str());
    sendError(500, String("Falha na atualização: ") + otaError_);
  }
}

void WebUi::handleNotFound() {
  // Captive portal: send hotspot clients that probe other hosts to the UI.
  if (wifi_.apActive()) {
    const String host = server_.hostHeader();
    if (host != wifi_.apIp() && host != WiFi.localIP().toString()) {
      server_.sendHeader("Location", String("http://") + wifi_.apIp() + "/", true);
      server_.send(302, "text/plain", "");
      return;
    }
  }
  server_.send(404, "text/plain", "Not found");
}
