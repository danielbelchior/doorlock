# AI assistant guide

Read this before changing anything. It is the short version of the other documents, plus the rules an assistant tends to break.

## Project in five lines

- ESP32 (Arduino, PlatformIO env `esp32dev`) toy keypad: PIN, greeting, optional per-user math challenge, 128x64 OLED, RGBW LED, active buzzer.
- Web page and JSON API served by the ESP32 (no login), WiFi setup hotspot from the keypad menu, OTA updates.
- Firmware `2.3.0`. Code in `src/`, docs in `docs/`, overview in `README.md`.
- No automated tests; verification is static checks, a build, and a manual checklist ([TESTING.md](TESTING.md)).
- Owner's language for UI text is Portuguese; everything else English.

## Hard rules

1. **Do not download anything to compile.** No `pio` platform or library installs, no toolchain downloads, unless the user explicitly allows it. If the toolchain is missing, use the static checks in [TESTING.md](TESTING.md#1-static-checks-without-the-toolchain) and say the firmware was not built.
2. **No new libraries.** Only Keypad, U8g2 and Adafruit NeoPixel, plus the ESP32 core ([DECISIONS.md](DECISIONS.md#d16-u8g2-for-the-096-oled-v22)).
3. **No `delay()` in the loop path.** Use `millis()` deadlines and `timeReached()`.
4. **Do not change `struct User`, the `keypad` namespace or its keys.** Add per-user data as a parallel blob (pattern: `challenge`).
5. **The buzzer is active (MH-FMD, LOW = on, VCC 3V3).** Use on/off patterns; never `tone()`, LEDC or frequencies.
6. **`<Keypad.h>` stays the last include in `main.cpp`.**
7. **Display text:** Portuguese, ASCII only (the fonts have no accents). Keep it short: long text shrinks to a smaller font and is clipped if it still does not fit. **Web and API text:** Portuguese with accents.
8. **Escape everything:** `jsonString()` on the server, `esc()` in the page.
9. **Keep pins off** GPIO 0, 2, 12, 15, 6 to 11 and 34 to 39.
10. **Do not commit, push or delete user files** unless asked.

## Where things are

| Task | Files |
|---|---|
| Pins, timings, limits | `src/config.h` |
| Keypad flow, screen text, menu | `src/game.cpp`, `src/game.h` |
| Screen layout, fonts, icons, header | `src/display.cpp`, `src/display.h` |
| Sounds | `src/buzzer.cpp` (pattern arrays), `src/buzzer.h` (`Sound`) |
| LED colors and blinks | `src/led.h`, `src/led.cpp` |
| Users, PIN rules, challenge option, storage | `src/users.cpp`, `src/users.h` |
| WiFi credentials | `src/settings.cpp` |
| WiFi, hotspot, mDNS | `src/wifi_service.cpp` |
| HTTP routes and JSON | `src/web_ui.cpp` |
| Web page (HTML, CSS, JS) | `src/web_page.h` |
| Wiring into the loop, OTA feedback, clock, night screen-off | `src/main.cpp` |

Recipes for common changes: [DEVELOPMENT.md](DEVELOPMENT.md#how-to-make-common-changes). File details: [CODEBASE.md](CODEBASE.md).

## Workflow

1. Read the files you will touch and the doc that owns the topic.
2. Make the smallest complete change that matches the existing style ([STYLEGUIDE.md](STYLEGUIDE.md)).
3. Verify:
   - `node --check` on the extracted page script if `web_page.h` changed.
   - `clang++ -fsyntax-only` with stubs for `buzzer.cpp`, `users.cpp`, `game.cpp`, `display.cpp` if they changed.
   - `pio run` only if the toolchain is already installed.
4. Update documentation (map below). Bump `FIRMWARE_VERSION` for user-visible firmware changes.
5. Report what changed, what was checked, and what was not (for example "not built on hardware").

## Documentation update map

| If you change | Update |
|---|---|
| A pin or wiring | `src/config.h`, `docs/hardware.md`, `docs/wiring.svg`, `docs/esp32-devkit-pinout.svg`, README wiring table, `FUNDAMENTALS.md` |
| A `config.h` value | `firmware.md` configuration table, plus any doc quoting it (`DESIGN.md` timings) |
| Screen text or a state | `DESIGN.md` (table and `docs/oled-screens.svg` preview), `ARCHITECTURE.md` diagram, `firmware.md` state machine |
| LED or buzzer feedback | `DESIGN.md`, `firmware.md` LED/Buzzer sections, README features |
| Storage | `ARCHITECTURE.md` storage, `firmware.md` storage, `DECISIONS.md` |
| An API route or field | `API.md`, `web-ui.md`, `CODEBASE.md` |
| The web page | `web-ui.md`, `DESIGN.md` |
| Version | `config.h`, `API.md` and `web-ui.md` status examples, `FUNDAMENTALS.md`, `DEPLOYMENT.md`, wiring diagram title |
| A design choice | Add an entry to `DECISIONS.md` |
| A new failure mode | `TROUBLESHOOTING.md` |
| Behavior to verify | `TESTING.md` checklist and `firmware.md` manual checklist |

## Known pitfalls

- `UserStore::update()` and `add()` take `withChallenge`; the web layer must pass the current value when the parameter is missing on update.
- `UserStore::remove()` must shift `challenge_` together with `users_`.
- `Buzzer::play(Sound::Key)` and `Sound::Invalid` are ignored while any other pattern plays, on purpose. The `#` key plays the click first and then the result pattern replaces it.
- Every `Sound` must keep a rhythm no other sound uses, `Restart` must end within 1 s and `UpdateDone` within 1.5 s (the reboot delays).
- `Game::isIdle()` decides whether a WiFi notice may interrupt the screen; keep it true only when nothing is being typed.
- During OTA the loop stops; `Buzzer::stop()` is called at upload start so a beep cannot stay on.
- `String::toInt()` returns 0 for garbage; use the strict `parseIndex()` for indexes.
- The web page must not load anything from the internet (hotspot mode is offline).
- `Display::show()` does nothing when the new `Screen` equals the current one (`Screen::operator==`). A new `Screen` field must be added to that comparison.
- At night the first key both wakes the screen and is typed. `Game::restingFor()` only counts an empty PIN prompt, so the screen never turns off mid-entry or on another screen.
- The clock and night screen-off depend on NTP. Before the first sync `time()` is near 1970; `updateDisplayStatus()` treats that as "no clock".

## Current state and open ideas

- Done in v2.3: a distinct sound for every outcome (keypad results, invalid keys, timeouts, menu actions, web actions through `WebUi::onEvent`, boot, WiFi connected, OTA). Not built (no PlatformIO); `buzzer.cpp` and `game.cpp` compiled and ran on the host with stub headers; `main.cpp` and `web_ui.cpp` reviewed by reading. Not tested on hardware.
- Done in v2.2: 0.96" SSD1306 OLED (U8g2) instead of the 16x2 LCD, a new screen design (header with clock and network icon, icons, key hints), header clock from NTP, and the night screen-off.
- v2.2 was not built: PlatformIO was not installed. `display.cpp`, `game.cpp`, `users.cpp`, `led.cpp` and `buzzer.cpp` compiled and ran on the host with stub headers (that run produced `docs/oled-screens.svg`). `main.cpp`, `web_ui.cpp` and `wifi_service.cpp` were only reviewed by reading. Nothing was tested on hardware. Treat the first real build and the manual checklist as part of the next change.
- Done in v2.1: per-user math challenge option; MH-FMD active buzzer feedback. Also not verified on hardware in the session that added them.
- Roadmap ideas: lockout after several wrong PINs, scores and ranking, difficulty levels, hashed PINs, MQTT events, buzzer mute option in the web page, user list export/import.
