#pragma once

#include <Arduino.h>
#include <DNSServer.h>

class Settings;

// Keeps retrying the saved network forever. The setup hotspot (open, with a
// captive-portal DNS) only starts when startHotspot() is called.
class WifiService {
 public:
  enum class Event : uint8_t { None = 0, Connected = 1, Disconnected = 2 };

  explicit WifiService(Settings& settings);

  void begin();
  void update();

  // Returns pending events one at a time until Event::None.
  Event takeEvent();

  // Reconnect with the credentials currently stored in Settings.
  void reconnect() { reconnectPending_ = true; }

  // Stays on until a network saved while it is on connects (then closes
  // after WIFI_AP_LINGER_MS) or until the device restarts.
  void startHotspot();

  bool connected() const;
  bool apActive() const { return apActive_; }
  String ip() const;
  String apIp() const;
  const String& apSsid() const { return apSsid_; }
  int32_t rssi() const;

 private:
  void connectSta();
  void stopAp();
  void startMdns();
  void push(Event event) { pendingEvents_ |= static_cast<uint8_t>(event); }

  Settings& settings_;
  DNSServer dns_;
  String apSsid_;
  bool apActive_ = false;
  bool wasConnected_ = false;
  bool mdnsStarted_ = false;
  bool reconnectPending_ = false;
  bool closeApOnConnect_ = false;
  bool apStopScheduled_ = false;
  uint8_t pendingEvents_ = 0;
  uint32_t lastAttemptMs_ = 0;
  uint32_t apStopAtMs_ = 0;
};
