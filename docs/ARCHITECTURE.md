# Architecture

## System view

```mermaid
flowchart LR
    subgraph Device[ESP32]
        Main[main.cpp<br/>setup and loop]
        Game[Game<br/>state machine]
        Display[Display]
        Led[StatusLed]
        Buzzer[Buzzer]
        Users[UserStore]
        Settings[Settings]
        Wifi[WifiService]
        Web[WebUi]
        NVS[(NVS flash)]
    end
    Keypad[[4x4 keypad]] --> Main
    Main --> Game
    Game --> Display --> OLED[[OLED 128x64]]
    Main -->|clock, network, sleep| Display
    Game --> Led --> LEDHW[[SK6812]]
    Game --> Buzzer --> BuzzHW[[MH-FMD buzzer]]
    Game --> Users
    Web --> Users
    Web --> Settings
    Web --> Wifi
    Wifi --> Settings
    Users --> NVS
    Settings --> NVS
    Browser[[Browser]] <-->|HTTP :80| Web
    Wifi <-->|STA / AP + DNS + mDNS| Router[[WiFi]]
```

Each module owns one piece of hardware or one concern. Only `main.cpp` knows about all of them; it creates the objects as globals, wires callbacks and runs the loop.

## Modules and dependencies

| Module | Owns | Depends on |
|---|---|---|
| `main.cpp` | Object creation, keypad reading, menu actions, WiFi event reactions, OTA callbacks, header clock and network icon, night screen-off | everything, plus `configTzTime()` from the ESP32 core |
| `Game` | Keypad state machine, menu, all screen content | `Display`, `StatusLed`, `Buzzer`, `UserStore` |
| `Display` | OLED: header, body layouts, footer, power save; sends a frame only when something changed | `U8g2` |
| `StatusLed` | One SK6812 pixel: key flash and N blinks | `Adafruit_NeoPixel` |
| `Buzzer` | Active buzzer on/off patterns | Arduino `digitalWrite` |
| `UserStore` | Users and their challenge option in NVS | `Preferences` |
| `Settings` | WiFi credentials in NVS | `Preferences` |
| `WifiService` | Station mode, endless retries, setup hotspot, captive DNS, mDNS, events | `WiFi`, `DNSServer`, `ESPmDNS`, `Settings` |
| `WebUi` | HTTP routes, JSON, OTA upload, delayed reboot | `WebServer`, `Update`, `UserStore`, `Settings`, `WifiService` |
| `web_page.h` | The single-page UI as a PROGMEM string | none |
| `util.h` | `timeReached()` overflow-safe deadline check | none |

Rules that keep this shape:

- `Game` never touches WiFi or the web server. It reports menu choices through the `onMenuAction` callback and `main.cpp` acts on them.
- `WebUi` never touches the display or the buzzer. It reports OTA progress through `onOtaStart` / `onOtaEnd` and other outcomes through `onEvent`, and `main.cpp` updates the display, LED and buzzer.
- `Display` knows nothing about WiFi, time or game states. `Game` gives it a `Screen`; `main.cpp` gives it the network status, the clock text and the sleep flag.
- `WifiService` reports connection changes as events (`takeEvent()`), polled by `main.cpp`.

## The main loop

```text
setup():  Serial, display (starts I2C), led, buzzer, random seed,
          settings, users, wifi (starts TCP/IP), NTP (configTzTime),
          web callbacks, web, game

loop():   wifi.update()          reconnects, hotspot DNS, events
          web.update()           one HTTP client, delayed reboot
          led.update()           advances blink phases
          buzzer.update()        advances beep steps
          game.update()          screen timeouts
          handleWifiEvents()     shows the IP when connected and idle
          updateDisplayStatus()  once a second: network icon, clock, night screen-off
          restart check          menu option 3
          keypad.getKey()        on a key: display.setAsleep(false), led.keyFlash(),
                                 buzzer.play(Key), game.handleKey()
```

Every `update()` is cheap and returns at once when nothing is due. Time is compared with `timeReached(deadline)` from `util.h`, which survives the `millis()` wrap after about 49 days. The only blocking moment is an OTA upload: `WebServer` handles the whole request inside `web.update()`, so the keypad freezes for a few seconds and `Buzzer::stop()` is called first so no beep gets stuck on.

## Keypad state machine

```mermaid
stateDiagram-v2
    [*] --> EnterPin
    EnterPin --> Greeting: # known PIN, challenge on
    EnterPin --> Result: # known PIN, challenge off
    EnterPin --> Result: # unknown PIN
    Greeting --> Challenge: after 1.5 s
    Challenge --> Result: # with an answer
    Challenge --> EnterPin: 60 s idle
    Result --> EnterPin: 3 s or any key
    EnterPin --> Menu: 2205 then D
    Menu --> EnterPin: * or 30 s idle
    Menu --> Notice: option 1 or 2
    Menu --> Busy: option 3 (restart)
    EnterPin --> Notice: WiFi connected while idle
    Notice --> EnterPin: timeout or any key
    EnterPin --> Busy: OTA starts
    Busy --> EnterPin: OTA failed
```

States are `Game::State`: `EnterPin`, `Greeting`, `Challenge`, `Result`, `Notice`, `Busy`, `Menu`. Screen texts per state are in [DESIGN.md](DESIGN.md#oled-screens).

## Data flows

**Key press.** `keypad.getKey()` returns a char. `main.cpp` flashes the LED yellow and plays the key click, then `Game::handleKey()` acts by state. On `#` in `EnterPin`, `Game::submitPin()` asks `UserStore::findByPin()`. On success it copies the name, reads `UserStore::challenge(index)` and chooses `Greeting` or `Result`. Result feedback (`StatusLed::blink`, `Buzzer::play(Welcome/AccessDenied/Correct/Wrong)`) replaces the key click. A key that does nothing in the current state plays `Sound::Invalid`.

**Clock and night screen-off.** `setup()` calls `configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2)` after `wifi.begin()`; the ESP32 core syncs the time in the background once WiFi is up. Once a second, `updateDisplayStatus()` sets the header network icon and, after the first sync, the `HH:MM` clock. Between `DISPLAY_OFF_HOUR` and `DISPLAY_ON_HOUR` it turns the panel off while `Game::restingFor(DISPLAY_NIGHT_WAKE_MS)` is true (empty PIN prompt, no key for 30 s). A key turns the panel on in `loop()` and is still passed to `Game::handleKey()`. Before the first sync the clock is hidden and the panel never turns off.

**Web request.** The browser loads `/` (the embedded page), then calls `/api/*` with `fetch`. `WebUi` validates input through `UserStore` or `Settings`, which save to NVS and return a result code. `WebUi` turns it into JSON and reports a `WebEvent` through `onEvent`, which `main.cpp` turns into a sound. See [API.md](API.md).

**OTA.** `/api/update` receives a multipart upload in chunks (`handleOtaUpload`), writes it with `Update` into the inactive app slot, then `handleOtaDone` answers and schedules a reboot 1.5 s later.

**WiFi.** `WifiService::update()` retries the stored network every 15 s (30 s while the hotspot is up). On connect it starts mDNS (`keypad.local`) and pushes `Event::Connected`. A hotspot started from the menu stays up until a network saved while it was up connects, plus 60 s.

## Storage

| NVS namespace | Key | Content |
|---|---|---|
| `keypad` | `count` | `uint8` number of users |
| `keypad` | `users` | 520-byte blob, `User[20]` of `{char name[17]; char pin[9];}` (v1 layout) |
| `keypad` | `challenge` | 20-byte blob, one byte per user, 1 = challenge on (added in v2.1) |
| `settings` | `wifi_ssid`, `wifi_pass` | WiFi credentials |

Writes happen only on add, edit or remove. Blobs are written before `count`. More in [firmware.md](firmware.md#storage).

## Memory and flash

- Partition table `min_spiffs.csv`: two 1.9 MB app slots (OTA) and a small SPIFFS that the firmware does not use.
- The web page is a PROGMEM string served with `send_P`, so it does not use heap.
- JSON is built with Arduino `String` and `reserve()`; responses are small (at most 20 users).
