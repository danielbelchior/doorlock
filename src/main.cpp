#include <Arduino.h>
#include <time.h>

#include "buzzer.h"
#include "config.h"
#include "display.h"
#include "game.h"
#include "led.h"
#include "settings.h"
#include "users.h"
#include "util.h"
#include "web_ui.h"
#include "wifi_service.h"

// Keypad.h defines generic macros (OPEN, CLOSED, ...); including it last
// keeps them out of every other header.
#include <Keypad.h>

namespace {

char keymap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};
byte rowPins[KEYPAD_ROWS] = {PIN_KEYPAD_ROW_1, PIN_KEYPAD_ROW_2, PIN_KEYPAD_ROW_3, PIN_KEYPAD_ROW_4};
byte colPins[KEYPAD_COLS] = {PIN_KEYPAD_COL_1, PIN_KEYPAD_COL_2, PIN_KEYPAD_COL_3, PIN_KEYPAD_COL_4};

Keypad keypad(makeKeymap(keymap), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);
Display display;
StatusLed led;
Buzzer buzzer;
UserStore users;
Settings settings;
WifiService wifi(settings);
Game game(display, led, buzzer, users);
WebUi web(users, settings, wifi);

bool restartPending = false;
uint32_t restartAtMs = 0;
uint32_t nextClockCheckMs = 0;

void showNetworkInfo(uint32_t durationMs) {
  if (wifi.connected()) {
    game.showNotice(messageScreen("Rede WiFi", wifi.ip(), String(WIFI_HOSTNAME) + ".local"),
                    durationMs);
  } else if (!settings.hasWifi()) {
    game.showNotice(messageScreen("Rede WiFi", "Sem WiFi", "Nao configurado", Icon::Error),
                    durationMs);
  } else {
    game.showNotice(messageScreen("Rede WiFi", "Sem WiFi", "Conectando..."), durationMs);
  }
}

void handleMenuAction(MenuAction action) {
  switch (action) {
    case MenuAction::Hotspot:
      wifi.startHotspot();
      game.showNotice(messageScreen("Hotspot", wifi.apSsid(), wifi.apIp()), HOTSPOT_INFO_MS);
      break;
    case MenuAction::ShowIp:
      showNetworkInfo(MENU_INFO_MS);
      break;
    case MenuAction::Restart:
      game.showBusy(messageScreen("Sistema", "Reiniciando..."));
      restartPending = true;
      restartAtMs = millis() + 1000;
      break;
  }
}

void handleWifiEvents() {
  WifiService::Event event;
  while ((event = wifi.takeEvent()) != WifiService::Event::None) {
    if (event == WifiService::Event::Connected && game.isIdle()) showNetworkInfo(NOTICE_MS);
  }
}

bool inQuietHours(int hour) {
  if (DISPLAY_OFF_HOUR == DISPLAY_ON_HOUR) return false;
  if (DISPLAY_OFF_HOUR < DISPLAY_ON_HOUR) return hour >= DISPLAY_OFF_HOUR && hour < DISPLAY_ON_HOUR;
  return hour >= DISPLAY_OFF_HOUR || hour < DISPLAY_ON_HOUR;  // window crosses midnight
}

// Once a second: header clock, network icon and the night screen-off.
void updateDisplayStatus() {
  if (!timeReached(nextClockCheckMs)) return;
  nextClockCheckMs = millis() + 1000;

  display.setNetwork(wifi.apActive()    ? NetStatus::Hotspot
                     : wifi.connected() ? NetStatus::Online
                                        : NetStatus::Offline);

  // Before the first NTP sync the clock counts from 1970.
  const time_t now = time(nullptr);
  if (now < 1700000000) {
    display.setClock("");
    display.setAsleep(false);
    return;
  }
  struct tm local;
  localtime_r(&now, &local);
  char hhmm[6];
  snprintf(hhmm, sizeof(hhmm), "%02d:%02d", local.tm_hour, local.tm_min);
  display.setClock(hhmm);
  display.setAsleep(inQuietHours(local.tm_hour) && game.restingFor(DISPLAY_NIGHT_WAKE_MS));
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("[boot] Teclado v" FIRMWARE_VERSION);

  display.begin();  // also starts the I2C bus
  display.show(messageScreen("Teclado", "v" FIRMWARE_VERSION, "Iniciando..."));
  led.begin();
  buzzer.begin();
  randomSeed(esp_random());

  settings.begin();
  users.begin();
  wifi.begin();  // must run before web.begin(): it starts the TCP/IP stack
  configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2);  // syncs in the background once online

  web.onOtaStart([]() {
    buzzer.stop();  // loop() stalls during the upload, so a note would never end
    display.setAsleep(false);
    game.showBusy(messageScreen("Firmware", "Atualizando...", "Nao desligue!"));
  });
  web.onOtaEnd([](bool success) {
    if (success) {
      game.showBusy(messageScreen("Firmware", "Atualizado!", "Reiniciando...", Icon::Ok));
      led.blink(LED_GREEN);
      buzzer.play(Sound::Success);
    } else {
      game.endBusy();
      led.blink(LED_RED);
      buzzer.play(Sound::Error);
    }
  });
  web.begin();

  game.onMenuAction(handleMenuAction);
  game.begin();
  if (!settings.hasWifi()) showNetworkInfo(NOTICE_MS);
}

void loop() {
  wifi.update();
  web.update();
  led.update();
  buzzer.update();
  game.update();
  handleWifiEvents();
  updateDisplayStatus();

  if (restartPending && timeReached(restartAtMs)) ESP.restart();

  const char key = keypad.getKey();
  if (key == NO_KEY) return;

  display.setAsleep(false);
  led.keyFlash();
  buzzer.play(Sound::Key);
  game.handleKey(key);
}
