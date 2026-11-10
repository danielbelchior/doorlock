# Decisions

Short records of choices that shape the project. Each entry says what was decided, why, and what it costs. Add new entries at the end; do not rewrite old ones, mark them superseded instead.

## D1. No login and plain-text PINs

- **Decision:** the web page and API have no authentication; PINs are stored and returned in plain text.
- **Why:** it is a toy on a trusted home network. Admins want to see and share PINs. v1 had a hard-coded admin login that added friction without real security.
- **Cost:** anyone on the network (or the open hotspot) can change users, WiFi and firmware. Documented as a known limitation; never use it for a real lock.

## D2. Non-blocking main loop

- **Decision:** no `delay()` in the loop path; every module has an `update()` driven by `millis()` deadlines via `timeReached()`.
- **Why:** keypad, LCD, LED, buzzer, WiFi and web server must work at the same time.
- **Cost:** small state machines instead of straight-line code. The OTA upload is the one accepted exception.

## D3. Keep the v1 storage layout

- **Decision:** the NVS namespace `keypad`, keys `count` and `users`, and `struct User { char name[17]; char pin[9]; }` stay exactly as in v1.
- **Why:** users survive firmware upgrades without a migration.
- **Cost:** new per-user data cannot go into `User`; it needs parallel arrays under new keys (see D9).

## D4. Moved keypad columns off GPIO 2 and 15 (v2)

- **Decision:** keypad pins 7 and 8 moved to GPIO 25 and 26.
- **Why:** GPIO 2 and 15 are boot strapping pins; a pressed key at reset could stop booting or flashing.
- **Cost:** a one-time rewire for v1 builders.

## D5. `min_spiffs.csv` partition table

- **Decision:** two 1.9 MB app slots instead of the default 1.25 MB.
- **Why:** OTA needs two slots, and WiFi + web server + `Update` leave little room in 1.25 MB.
- **Cost:** the first v2 install must be over USB. SPIFFS is tiny and unused.

## D6. Setup hotspot only on demand

- **Decision:** the open `Teclado-XXXX` hotspot starts only from keypad menu option 1, never automatically.
- **Why:** an automatic open hotspot during a router outage would expose the device to anyone nearby. Physical access to the keypad is required.
- **Cost:** first-time setup needs the menu code (`2205 D`).

## D7. Embedded single-page UI, no frameworks

- **Decision:** the page is a PROGMEM raw string in `web_page.h`, with plain CSS and JavaScript.
- **Why:** works offline on the hotspot, no filesystem upload step, OTA updates the page together with the firmware.
- **Cost:** editing HTML inside a C++ header; no JS tooling beyond `node --check`.

## D8. Hand-built JSON

- **Decision:** JSON is built with `String` and `jsonString()`; no JSON library.
- **Why:** the payloads are tiny and fixed; avoids a dependency (see D11).
- **Cost:** every new field needs care with escaping and commas.

## D9. Per-user math challenge as a separate NVS blob (v2.1)

- **Decision:** the challenge option is a 20-byte blob under key `challenge`, parallel to `users`. Missing or wrong-size blob means "challenge on" for everyone.
- **Why:** keeps D3, upgrades from v2.0 keep the old behavior, and the default for new users is the classic game.
- **Cost:** two blobs must stay in sync in `add`, `update`, `remove` and `seedDefaults`. A power cut between the two writes during a remove could misalign one flag; accepted for a toy.

## D10. API `challenge` parameter is optional

- **Decision:** missing `challenge` means on for add and unchanged for update; unknown values count as missing.
- **Why:** older scripts and `curl` commands keep working, and an edit of name or PIN cannot silently reset the option.
- **Cost:** a typo such as `challenge=no` is ignored instead of rejected.

## D11. No libraries beyond the three in `platformio.ini`

- **Decision:** only Keypad, LiquidCrystal_I2C and Adafruit NeoPixel; everything else comes from the ESP32 core.
- **Why:** fewer downloads, fewer version conflicts, the Arduino IDE setup stays short.
- **Cost:** small helpers (JSON, buzzer patterns) are written by hand.
- **Superseded by D16 (v2.2):** LiquidCrystal_I2C was replaced by U8g2. The rule is now Keypad, U8g2 and Adafruit NeoPixel.

## D12. Active buzzer with on/off patterns (v2.1)

- **Decision:** the MH-FMD module in use carries an active buzzer, so the firmware drives it with `digitalWrite()` and sounds differ by rhythm (click, two quick beeps, one long beep).
- **Supersedes:** the first v2.1 draft, which assumed a passive buzzer and played melodies through the ESP-IDF LEDC driver.
- **Why:** an active buzzer has its own oscillator; PWM only makes it crackle.
- **Cost:** no pitch changes.

## D13. Buzzer module on 3V3, idle HIGH

- **Decision:** power the MH-FMD from 3V3 and hold I/O HIGH when silent (`BUZZER_ACTIVE_LOW = true`). The pin is written HIGH before it becomes an output.
- **Why:** the module uses a PNP transistor with its emitter at VCC; on 5V the 3.3V HIGH cannot switch it off. Writing the level first avoids a chirp at boot.
- **Cost:** the buzzer is quieter at 3.3V. A louder setup needs an NPN driver and `BUZZER_ACTIVE_LOW = false`.

## D14. Portuguese UI, English code

- **Decision:** people-facing text in Portuguese; code, comments, logs and docs in English.
- **Why:** the users are Portuguese speakers; contributors and tools work best in English.
- **Cost:** two languages to keep consistent; LCD text without accents.
- **Note (v2.2):** the LCD is now an OLED (D16). Its fonts have no accents either, so display text stays ASCII.

## D15. User edits by list position

- **Decision:** the API addresses users by `index`, not by an ID.
- **Why:** no ID in the v1 layout (D3); simple for 20 users.
- **Cost:** concurrent edits can hit a stale position. The page reloads the list after every change.

## D16. U8g2 for the 0.96" OLED (v2.2)

- **Decision:** the 16x2 LCD with a PCF8574 backpack is replaced by a 0.96" 128x64 SSD1306 OLED, driven by the U8g2 library with a full frame buffer (`U8G2_SSD1306_128X64_NONAME_F_HW_I2C`). U8g2 replaces LiquidCrystal_I2C, so the library rule of D11 becomes: only Keypad, U8g2 and Adafruit NeoPixel.
- **Why:** one library brings the drivers for both SSD1306 and SH1106 (the 1.3" modules) and a set of fonts, so a module swap is a one-line change. The full buffer (1 KB) lets the firmware draw a whole screen and send it once, without flicker. The OLED runs on 3V3, which removes the 5V I2C pull-up issue of the LCD backpack.
- **Cost:** about 1 KB of RAM for the buffer and some flash for the fonts. The fonts have no accents, so screen text stays ASCII. OLED pixels can burn in (see D17).

## D17. Screen off at night via NTP time (v2.2)

- **Decision:** from `DISPLAY_OFF_HOUR` to `DISPLAY_ON_HOUR` (00:00 to 08:00) the panel turns off when the PIN prompt is empty and no key was pressed for 30 s. Local time comes from NTP through `configTzTime()` in the ESP32 core; no RTC module and no extra library.
- **Why:** an OLED that shows the same static prompt all night burns in. Nobody uses the keypad at night, so the screen can be dark then.
- **Cost:** the clock needs WiFi. Without it (or before the first sync) the screen stays on all night and the header has no clock. At night the first key both wakes the screen and is typed as a digit.
