# Codebase

A file-by-file map. Paths are relative to the repository root.

```
platformio.ini            Board, libraries, partition table, -Wall
src/
  main.cpp                Globals, setup(), loop(), menu and OTA glue
  config.h                Every pin, timing, limit and default
  util.h                  timeReached()
  game.h / game.cpp       Game: keypad state machine and LCD text
  display.h / .cpp        Display: flicker-free 16x2 LCD
  led.h / led.cpp         StatusLed: SK6812 effects
  buzzer.h / .cpp         Buzzer: active buzzer patterns
  users.h / users.cpp     UserStore: users + challenge option in NVS
  settings.h / .cpp       Settings: WiFi credentials in NVS
  wifi_service.h / .cpp   WifiService: STA, hotspot, DNS, mDNS, events
  web_ui.h / web_ui.cpp   WebUi: routes, JSON, OTA
  web_page.h              INDEX_HTML: the whole web page
docs/                     Documentation and SVG diagrams
```

## `src/main.cpp`

- Global objects, in dependency order: `keypad`, `display`, `led`, `buzzer`, `users`, `settings`, `wifi(settings)`, `game(display, led, buzzer, users)`, `web(users, settings, wifi)`.
- `showNetworkInfo(ms)`: IP, "Sem WiFi / nao configurado" or "Sem WiFi / Conectando...".
- `handleMenuAction(MenuAction)`: hotspot, show IP, restart (sets `restartPending`).
- `handleWifiEvents()`: shows the IP on `Event::Connected` if `game.isIdle()`.
- `<Keypad.h>` is the last include because it defines macros such as `OPEN` and `CLOSED`.

## `src/config.h`

Compile-time configuration only (`constexpr` and `FIRMWARE_VERSION`). Full table in [firmware.md](firmware.md#configuration). Changing `MAX_USERS`, `USER_NAME_LEN` or `USER_PIN_LEN` resets stored users to the defaults.

## `src/game.h`, `src/game.cpp`

`class Game`

| Member | Purpose |
|---|---|
| `begin()`, `update()`, `handleKey(char)` | Lifecycle; `update()` handles timeouts per state |
| `onMenuAction(handler)` | Callback for menu options 1 to 3 (`MenuAction::Hotspot`, `ShowIp`, `Restart`) |
| `isIdle()` | True at an empty PIN prompt or on a notice |
| `showNotice(top, bottom, ms)` | Temporary message, any key dismisses |
| `showBusy(top, bottom)`, `endBusy()` | Message that ignores keys until ended |
| `submitPin()` | Finds the user, plays feedback, chooses `Greeting` (challenge on) or `Result` (off) |
| `startChallenge()`, `submitAnswer()` | Builds a random question, checks the answer |
| `userName_` | Copy of the name, so editing the user mid-session is safe |

## `src/display.*`

`Display::show(top, bottom)` pads or cuts to 16 characters and writes a line only if it changed.

## `src/led.*`

`StatusLed::keyFlash()` (ignored during a blink), `blink(color, times)`, `update()`. Colors: `LED_YELLOW`, `LED_GREEN`, `LED_RED`, built with `ledColor(r, g, b, w)`. Pixel type `NEO_GRBW + NEO_KHZ800`.

## `src/buzzer.*`

`enum class Sound { Key, Success, Error }`. `Buzzer::play(Sound)` starts a pattern of `Step {bool on; uint16_t ms;}`; `Key` is ignored while a `Success` or `Error` pattern plays. `stop()` silences at once. `update()` advances steps. Patterns are `KEY_STEPS`, `SUCCESS_STEPS`, `ERROR_STEPS` at the top of `buzzer.cpp`. The pin level for "on" comes from `BUZZER_ACTIVE_LOW`.

## `src/users.*`

`struct User { char name[17]; char pin[9]; }` must keep the v1 layout.

`class UserStore`

| Member | Purpose |
|---|---|
| `begin()` | Loads users and challenge flags; seeds `Daniel/1234` and `Silvia/5678` on a fresh device |
| `count()`, `at(i)`, `challenge(i)`, `findByPin(pin)` | Read access |
| `add(name, pin, withChallenge)` | Validates and saves |
| `update(index, name, pin, withChallenge)` | Validates (ignoring its own PIN) and saves |
| `remove(index)` | Shifts users and flags down and saves |
| `save()` | Writes `users`, `challenge`, then `count` |

Returns `UserResult` (`Ok`, `InvalidName`, `InvalidPin`, `DuplicatePin`, `Full`, `NotFound`, `StorageError`); `userResultMessage()` gives the Portuguese text.

## `src/settings.*`

`Settings::setWifi(ssid, pass)` returns `nullptr` or a Portuguese error. SSID 1 to 32 characters; password empty or 8 to 63.

## `src/wifi_service.*`

`begin()`, `update()`, `takeEvent()` (`Connected`, `Disconnected`), `reconnect()`, `startHotspot()`, getters `connected()`, `ip()`, `apActive()`, `apSsid()`, `apIp()`, `rssi()`. WiFi credentials are not persisted by the driver (`WiFi.persistent(false)`).

## `src/web_ui.*`

`WebUi::begin()` registers routes; `update()` serves one client and handles the delayed reboot. Helpers in an anonymous namespace: `jsonString()` (escaping), `jsonBool()`, `parseIndex()` (strict 1 to 3 digits), `parseFlag()` (`1/true/on`, `0/false/off`, else fallback). Route table in [API.md](API.md).

## `src/web_page.h`

`INDEX_HTML` raw string literal: CSS, markup and script. Script globals: `users`, `maxUsers`, `editing` (index or -1), `ssidLoaded`. Helpers: `$`, `$$`, `esc()`, `api(path, data)`, `toast(msg, bad)`. Sections: Tabs, Users, Status, WiFi, System.
