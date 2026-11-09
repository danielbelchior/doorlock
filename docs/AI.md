# AI assistant guide

Read this before changing anything. It is the short version of the other documents, plus the rules an assistant tends to break.

## Project in five lines

- ESP32 (Arduino, PlatformIO env `esp32dev`) toy keypad: PIN, greeting, optional per-user math challenge, LCD, RGBW LED, active buzzer.
- Web page and JSON API served by the ESP32 (no login), WiFi setup hotspot from the keypad menu, OTA updates.
- Firmware `2.1.0`. Code in `src/`, docs in `docs/`, overview in `README.md`.
- No automated tests; verification is static checks, a build, and a manual checklist ([TESTING.md](TESTING.md)).
- Owner's language for UI text is Portuguese; everything else English.

## Hard rules

1. **Do not download anything to compile.** No `pio` platform or library installs, no toolchain downloads, unless the user explicitly allows it. If the toolchain is missing, use the static checks in [TESTING.md](TESTING.md#1-static-checks-without-the-toolchain) and say the firmware was not built.
2. **No new libraries.** Only Keypad, LiquidCrystal_I2C and Adafruit NeoPixel, plus the ESP32 core ([DECISIONS.md](DECISIONS.md#d11-no-libraries-beyond-the-three-in-platformioini)).
3. **No `delay()` in the loop path.** Use `millis()` deadlines and `timeReached()`.
4. **Do not change `struct User`, the `keypad` namespace or its keys.** Add per-user data as a parallel blob (pattern: `challenge`).
5. **The buzzer is active (MH-FMD, LOW = on, VCC 3V3).** Use on/off patterns; never `tone()`, LEDC or frequencies.
6. **`<Keypad.h>` stays the last include in `main.cpp`.**
7. **LCD text:** Portuguese, ASCII only, at most 16 characters per line. **Web and API text:** Portuguese with accents.
8. **Escape everything:** `jsonString()` on the server, `esc()` in the page.
9. **Keep pins off** GPIO 0, 2, 12, 15, 6 to 11 and 34 to 39.
10. **Do not commit, push or delete user files** unless asked.

## Where things are

| Task | Files |
|---|---|
| Pins, timings, limits | `src/config.h` |
| Keypad flow, LCD text, menu | `src/game.cpp`, `src/game.h` |
| Sounds | `src/buzzer.cpp` (pattern arrays), `src/buzzer.h` (`Sound`) |
| LED colors and blinks | `src/led.h`, `src/led.cpp` |
| Users, PIN rules, challenge option, storage | `src/users.cpp`, `src/users.h` |
| WiFi credentials | `src/settings.cpp` |
| WiFi, hotspot, mDNS | `src/wifi_service.cpp` |
| HTTP routes and JSON | `src/web_ui.cpp` |
| Web page (HTML, CSS, JS) | `src/web_page.h` |
| Wiring into the loop, OTA feedback | `src/main.cpp` |

Recipes for common changes: [DEVELOPMENT.md](DEVELOPMENT.md#how-to-make-common-changes). File details: [CODEBASE.md](CODEBASE.md).

## Workflow

1. Read the files you will touch and the doc that owns the topic.
2. Make the smallest complete change that matches the existing style ([STYLEGUIDE.md](STYLEGUIDE.md)).
3. Verify:
   - `node --check` on the extracted page script if `web_page.h` changed.
   - `clang++ -fsyntax-only` with stubs for `buzzer.cpp`, `users.cpp`, `game.cpp` if they changed.
   - `pio run` only if the toolchain is already installed.
4. Update documentation (map below). Bump `FIRMWARE_VERSION` for user-visible firmware changes.
5. Report what changed, what was checked, and what was not (for example "not built on hardware").

## Documentation update map

| If you change | Update |
|---|---|
| A pin or wiring | `src/config.h`, `docs/hardware.md`, `docs/wiring.svg`, `docs/esp32-devkit-pinout.svg`, README wiring table, `FUNDAMENTALS.md` |
| A `config.h` value | `firmware.md` configuration table, plus any doc quoting it (`DESIGN.md` timings) |
| LCD text or a state | `DESIGN.md`, `ARCHITECTURE.md` diagram, `firmware.md` state machine |
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
- `Buzzer::play(Sound::Key)` is ignored during a result pattern, on purpose. The `#` key plays the click first and then the result pattern replaces it.
- `Game::isIdle()` decides whether a WiFi notice may interrupt the screen; keep it true only when nothing is being typed.
- During OTA the loop stops; `Buzzer::stop()` is called at upload start so a beep cannot stay on.
- `String::toInt()` returns 0 for garbage; use the strict `parseIndex()` for indexes.
- The web page must not load anything from the internet (hotspot mode is offline).

## Current state and open ideas

- Done in v2.1: per-user math challenge option; MH-FMD active buzzer feedback.
- Not verified on hardware in the session that added them; the firmware was not compiled because the toolchain was not installed. Treat the first real build as part of the next change.
- Roadmap ideas: lockout after several wrong PINs, scores and ranking, difficulty levels, hashed PINs, MQTT events, buzzer mute option in the web page, OLED display, user list export/import.
