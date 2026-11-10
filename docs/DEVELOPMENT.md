# Development

## Requirements

- [PlatformIO](https://platformio.org/) (CLI or the VS Code extension). The first build downloads the `espressif32` platform, the toolchain and the three libraries in `platformio.ini`.
- Or the Arduino IDE with the **esp32 by Espressif Systems** core (2.x or 3.x) and the libraries **Keypad**, **U8g2** and **Adafruit NeoPixel**. Steps in [firmware.md](firmware.md#arduino-ide).
- A USB cable for the first flash. Later updates can go over WiFi ([DEPLOYMENT.md](DEPLOYMENT.md)).

## Everyday commands

```sh
pio run                      # build
pio run -t upload            # build and flash over USB
pio device monitor           # serial log, 115200 baud
pio run -t upload && pio device monitor
pio run -t erase             # wipe flash, including users and WiFi (fresh device)
```

The firmware image is `.pio/build/esp32dev/firmware.bin`.

## Serial log

Every module logs with a prefix, which makes the log easy to filter:

| Prefix | Source |
|---|---|
| `[boot]` | `setup()` |
| `[users]` | `UserStore` (loaded, defaults created, save failed) |
| `[wifi]` | `WifiService` (connecting, connected, lost, hotspot, mDNS) |
| `[web]` | `WebUi` (server started, restarting) |
| `[ota]` | Firmware upload |

## Configuration

All settings are `constexpr` values in `src/config.h`. See the table in [firmware.md](firmware.md#configuration). Things worth knowing:

- Pins: keep away from GPIO 0, 2, 12, 15 (strapping), 6 to 11 (flash) and 34 to 39 (input only).
- `BUZZER_ACTIVE_LOW`: `true` for the MH-FMD module.
- `DISPLAY_I2C_ADDRESS`: `0x3C` for most OLED modules, `0x3D` for some. A 1.3" SH1106 module also needs another driver class in `src/display.h`.
- `TIME_ZONE`: a POSIX TZ string for the header clock (`"<-03>3"` is Brasilia time). `DISPLAY_OFF_HOUR` / `DISPLAY_ON_HOUR` set the night screen-off; equal hours disable it.
- `MENU_CODE`: digits only, at most 8.
- `MAX_USERS`, `USER_NAME_LEN`, `USER_PIN_LEN`: changing them discards stored users on the next boot.
- `DEFAULT_WIFI_SSID` / `DEFAULT_WIFI_PASS`: only used until a network is saved from the web page.

## How to make common changes

### Add or change a sound

1. Add a value to `enum class Sound` in `src/buzzer.h` (only if it is a new sound).
2. Add a `Step` array at the top of `src/buzzer.cpp` and a `case` in `Buzzer::play()`.
3. Call `buzzer_.play(Sound::X)` from `Game`, or `buzzer.play(...)` from `main.cpp`.
4. The buzzer is active: use rhythm, not frequency. Do not use `delay()` or `tone()`.
5. Update the tables in [DESIGN.md](DESIGN.md#light-and-sound) and [firmware.md](firmware.md#buzzer).

### Add a keypad state

1. Add it to `Game::State`.
2. Handle its timeout in `Game::update()` and its keys in `Game::handleKey()`.
3. Build a `Screen` with `messageScreen()` (or fill a `Screen` for the `Pin` or `Menu` layout) and call `display_.show()`. Text in Portuguese, ASCII only; keep it short, because long text shrinks and is then clipped.
4. Update the diagram in [ARCHITECTURE.md](ARCHITECTURE.md#keypad-state-machine), the table in [DESIGN.md](DESIGN.md#oled-screens) and [firmware.md](firmware.md#keypad-state-machine).

### Add a per-user option

The `User` struct is frozen (v1 blob layout). Follow the pattern of the `challenge` option:

1. Add a parallel `uint8_t option_[MAX_USERS]` array and a getter in `UserStore`.
2. Store it under its own NVS key in namespace `keypad`; on boot, use a safe default if the key is missing or has the wrong size.
3. Keep it in sync in `add()`, `update()`, `remove()` (shift) and `seedDefaults()`; write it in `save()` before `count`.
4. Expose it in `handleUsersList()` and accept it in `handleUserAdd()` / `handleUserUpdate()` (keep the current value when the parameter is missing on update).
5. Add the field to the form and list in `src/web_page.h`.
6. Document it in [API.md](API.md), [firmware.md](firmware.md#storage) and [web-ui.md](web-ui.md).

### Add an HTTP route

1. Declare a `handleX()` in `src/web_ui.h` and register it in `WebUi::begin()`.
2. Reply with `sendJson`, `sendOk` or `sendError`. Pass every string through `jsonString()`.
3. On the page, call it with `api(path, data)` and show the result with `toast()`. Escape with `esc()` before inserting HTML.
4. Document it in [API.md](API.md).

### Change a pin

Update `src/config.h`, [hardware.md](hardware.md), both SVG diagrams in `docs/`, the README wiring table and [FUNDAMENTALS.md](FUNDAMENTALS.md#hardware-in-one-table).

### Release a version

1. Bump `FIRMWARE_VERSION` in `src/config.h` (semantic versioning).
2. Update the version shown in [API.md](API.md) and [web-ui.md](web-ui.md) examples and in the wiring diagram title.
3. Add upgrade notes to the README if wiring, storage or the partition table changed.
4. Run the checks in [TESTING.md](TESTING.md).

## Definition of done

- Builds for `esp32dev` without new warnings (`-Wall` is on for `src/`).
- No `delay()` in the loop path; new timers use `timeReached()`.
- No new library unless agreed (see [DECISIONS.md](DECISIONS.md)).
- The manual checks that touch the change pass ([TESTING.md](TESTING.md)).
- Documentation updated where the change is visible.
