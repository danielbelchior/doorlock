# Codebase

A file-by-file map. Paths are relative to the repository root.

```
platformio.ini            Board, libraries, partition table, -Wall
src/
  main.cpp                Globals, setup(), loop(), menu and OTA glue
  config.h                Every pin, timing, limit and default
  util.h                  timeReached()
  game.h / game.cpp       Game: keypad state machine and screen content
  display.h / .cpp        Display: 128x64 OLED drawing with U8g2
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
- `showNetworkInfo(ms)`: a `Rede WiFi` notice with the IP and `keypad.local`, `Sem WiFi` / `Nao configurado` (error icon), or `Sem WiFi` / `Conectando...`.
- `handleMenuAction(MenuAction)`: hotspot, show IP, restart (sets `restartPending`).
- `handleWifiEvents()`: shows the IP on `Event::Connected` if `game.isIdle()`, and plays `Sound::WifiConnected` unless the screen is off for the night.
- `soundFor(WebEvent)`: maps web outcomes to sounds; registered with `web.onEvent()`. `handleMenuAction()` plays `Hotspot`, `ShowIp` or `Restart`, and `setup()` ends with `Sound::Ready`.
- `updateDisplayStatus()`: runs once a second from `loop()`. Sets the header network icon (`NetStatus::Online`, `Offline`, `Hotspot`) and, once NTP has synced, the `HH:MM` clock. Turns the panel off with `display.setAsleep()` during quiet hours while `game.restingFor(DISPLAY_NIGHT_WAKE_MS)`.
- `inQuietHours(hour)`: true between `DISPLAY_OFF_HOUR` and `DISPLAY_ON_HOUR`, also when the window crosses midnight; always false when the two are equal.
- `setup()` calls `configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2)` after `wifi.begin()`. It does not call `Wire.begin()`: `display.begin()` starts I2C.
- `loop()` wakes the display (`display.setAsleep(false)`) on every key before passing the key on.
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
| `restingFor(ms)` | True at an empty PIN prompt with no key for at least `ms`; used for the night screen-off |
| `showNotice(screen, ms)` | Temporary `Screen`, any key dismisses |
| `showBusy(screen)`, `endBusy()` | `Screen` that ignores keys until ended |
| `submitPin()` | Finds the user, plays feedback, chooses `Greeting` (challenge on) or `Result` (off) |
| `startChallenge()`, `submitAnswer()` | Builds a random question, checks the answer |
| `userName_` | Copy of the name, so editing the user mid-session is safe |

## `src/display.*`

`struct Screen` describes one screen:

| Field | Meaning |
|---|---|
| `layout` | `Layout::Message` (text, detail, icon), `Layout::Pin` (one dot per character of `text`, 8 slots) or `Layout::Menu` (one row per `\n`-separated item of `text`) |
| `icon` | `Icon::None`, `Icon::Ok` (check) or `Icon::Error` (X), drawn on the left of a Message screen |
| `title` | Header text, top left |
| `text`, `detail` | Main text and the smaller line below it |
| `footer` | Key hints as `"<key> <label>"` items separated by `\|`, for example `"# Entrar\|* Apagar"` |
| `selected` | Highlighted menu row, 1-based; 0 = none |

`messageScreen(title, text, detail, icon)` builds a Message screen. `Screen::operator==` compares every field.

`class Display`

| Member | Purpose |
|---|---|
| `begin()` | Sets the I2C address (`DISPLAY_I2C_ADDRESS`) and clock, starts U8g2 (which starts `Wire` on GPIO 21 and 22), sets `DISPLAY_CONTRAST` |
| `show(screen)` | Draws a screen; does nothing if it equals the current one |
| `setNetwork(NetStatus)` | Header icon: filled bars (`Online`), outlined bars with a slash (`Offline`), `AP` badge (`Hotspot`) |
| `setClock(hhmm)` | Header clock; an empty string hides it |
| `setAsleep(bool)`, `asleep()` | Panel power save; the content is kept and shows again on wake |

Every change redraws the full 1 KB frame buffer and sends it only when something visible changed. `fitFont()` picks the largest font that fits the width: `helvB14` to `helvB08` for the main text, `helvR10` or `helvR08` for the detail. The driver class is `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`; a 1.3" SH1106 module needs `U8G2_SH1106_128X64_NONAME_F_HW_I2C`. Layout constants (header, footer and icon positions) are at the top of `display.cpp`.

## `src/led.*`

`StatusLed::keyFlash()` (ignored during a blink), `blink(color, times)`, `update()`. Colors: `LED_YELLOW`, `LED_GREEN`, `LED_RED`, built with `ledColor(r, g, b, w)`. Pixel type `NEO_GRBW + NEO_KHZ800`.

## `src/buzzer.*`

`enum class Sound` has one value per outcome (`Key`, `Invalid`, `Welcome`, `AccessDenied`, `Correct`, `Wrong`, `Timeout`, `MenuOpen`, `ShowIp`, `Hotspot`, `Restart`, `Ready`, `WifiConnected`, `UserAdded`, `UserUpdated`, `UserRemoved`, `WifiSaved`, `Rejected`, `UpdateDone`, `UpdateFailed`). `Buzzer::play(Sound)` starts a pattern of `Step {bool on; uint16_t ms;}`; the clicks `Key` and `Invalid` are ignored while any other pattern plays. `stop()` silences at once. `update()` advances steps. The `*_STEPS` arrays at the top of `buzzer.cpp` are built from the blocks `S`, `M`, `L` (beeps) and `G` (gap); `patternFor()` maps a `Sound` to its array. The pin level for "on" comes from `BUZZER_ACTIVE_LOW`. Rhythm table in [firmware.md](firmware.md#buzzer).

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

`WebUi::begin()` registers routes; `update()` serves one client and handles the delayed reboot. `onEvent(handler)` reports `WebEvent` outcomes (`UserAdded`, `UserUpdated`, `UserRemoved`, `WifiSaved`, `Rejected`, `Reboot`); `sendUserResult(result, successEvent)` and `sendRejected(code, message)` send the response and report the event. OTA results go to `onOtaEnd` instead. Helpers in an anonymous namespace: `jsonString()` (escaping), `jsonBool()`, `parseIndex()` (strict 1 to 3 digits), `parseFlag()` (`1/true/on`, `0/false/off`, else fallback). Route table in [API.md](API.md).

## `src/web_page.h`

`INDEX_HTML` raw string literal: CSS, markup and script. Script globals: `users`, `maxUsers`, `editing` (index or -1), `ssidLoaded`. Helpers: `$`, `$$`, `esc()`, `api(path, data)`, `toast(msg, bad)`. Sections: Tabs, Users, Status, WiFi, System.
