#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include <functional>

#include "users.h"

class Settings;
class WifiService;

// Outcomes of web actions, reported so main.cpp can give sound feedback.
enum class WebEvent : uint8_t { UserAdded, UserUpdated, UserRemoved, WifiSaved, Rejected, Reboot };

// Web UI and JSON API. No login: this is a toy on a home network.
class WebUi {
 public:
  using OtaStartHandler = std::function<void()>;
  using OtaEndHandler = std::function<void(bool success)>;
  using EventHandler = std::function<void(WebEvent)>;

  WebUi(UserStore& users, Settings& settings, WifiService& wifi);

  void begin();
  void update();

  void onOtaStart(OtaStartHandler handler) { otaStart_ = handler; }
  void onOtaEnd(OtaEndHandler handler) { otaEnd_ = handler; }
  // Not called for OTA results; those go to onOtaEnd().
  void onEvent(EventHandler handler) { event_ = handler; }

 private:
  void sendJson(int code, const String& body);
  void sendOk();
  void sendError(int code, const String& message);
  void sendUserResult(UserResult result, WebEvent success);
  void sendRejected(int code, const String& message);
  void notify(WebEvent event);
  void scheduleReboot(uint32_t delayMs);

  void handleIndex();
  void handleStatus();
  void handleUsersList();
  void handleUserAdd();
  void handleUserUpdate();
  void handleUserDelete();
  void handleWifiScanStart();
  void handleWifiScanResult();
  void handleWifiSave();
  void handleReboot();
  void handleOtaUpload();
  void handleOtaDone();
  void handleNotFound();

  WebServer server_;
  UserStore& users_;
  Settings& settings_;
  WifiService& wifi_;

  OtaStartHandler otaStart_;
  OtaEndHandler otaEnd_;
  EventHandler event_;
  bool otaStarted_ = false;
  String otaError_;

  bool rebootPending_ = false;
  uint32_t rebootAtMs_ = 0;
};
