# Fundamentals

Start here. This page explains what Doorlock is, what it is not, and the few ideas you need before reading the rest of the documentation.

## What it is

Doorlock is a toy for a home: an ESP32 with a 4x4 keypad, a 0.96" OLED display, an RGBW LED and an active buzzer. People type a PIN. If the PIN belongs to someone, the display greets them by name. Users who have the math challenge turned on then answer a random math question. Everything is managed from a web page served by the ESP32 itself.

- Firmware version: `2.2.0` (`FIRMWARE_VERSION` in `src/config.h`).
- Board: ESP32 DevKit ("ESP32 Dev Module", PlatformIO env `esp32dev`).
- Framework: Arduino on the ESP32 core, built with PlatformIO (Arduino IDE also works).
- Language: C++17 for the firmware; HTML, CSS and plain JavaScript for the web page, embedded in the firmware.

## What it is not

- **Not a security device.** PINs are stored and shown in plain text, the web page has no login, and the setup hotspot is open. It does not control a real lock. See [DECISIONS.md](DECISIONS.md#d1-no-login-and-plain-text-pins).
- **Not cloud connected.** Nothing leaves the local network. There is no account, telemetry or remote service.

## The user's journey

1. The display shows `Digite a senha` and 8 empty slots.
2. The person types digits (shown as dots) and presses `#`.
3. Unknown PIN: `Acesso negado` / `Senha invalida`, red blinks, one long beep, then back to step 1.
4. Known PIN: `Bem-vindo(a)!` and the name, green blinks, two quick beeps.
   - Challenge on: after 1.5 s the display asks something like `256 + 139`. The answer is typed and confirmed with `#`. Correct gives `Correto!`, wrong gives `Errado!` and the right answer.
   - Challenge off: the welcome stays for 3 s, then back to step 1.

Between 00:00 and 08:00 the screen turns off after 30 s without a key at the prompt, to slow OLED burn-in. Any key wakes it.

An admin manages users and WiFi from the web page, and reaches a service menu on the keypad by typing `2205` and `D`.

## Core concepts

| Concept | Meaning |
|---|---|
| User | A name (1 to 16 ASCII characters), a PIN (1 to 8 digits, unique) and a math challenge option. Up to 20. |
| Math challenge | A random addition, subtraction or multiplication asked after login, per user option. |
| Menu code | `2205` then `D` opens the keypad service menu (hotspot, IP, restart). |
| Setup hotspot | Open WiFi network `Teclado-XXXX` with a captive portal, started only from the keypad menu. |
| NVS | The ESP32 flash key-value store where users and WiFi credentials live. |
| OTA | Installing a new `firmware.bin` from the web page, without USB. |
| Night screen-off | The display turns off from 00:00 to 08:00 while idle, using the time from NTP. |
| Non-blocking loop | Every module advances with `millis()` deadlines; nothing calls `delay()` in `loop()`. |

## Hardware in one table

| Part | ESP32 pins |
|---|---|
| 4x4 keypad, rows R1 to R4 | GPIO 19, 18, 5, 17 |
| 4x4 keypad, columns C1 to C4 | GPIO 16, 4, 25, 26 |
| OLED 0.96" SSD1306 I2C (`0x3C`) | SDA 21, SCL 22, **3V3** |
| SK6812 RGBW LED | DIN 23 through 330 Ω, 5V |
| MH-FMD active buzzer (LOW = on) | I/O 33, **3V3** |

Details: [hardware.md](hardware.md).

## Documentation map

| Document | Read it when you want to |
|---|---|
| [FUNDAMENTALS.md](FUNDAMENTALS.md) | Get the big picture (this page) |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Understand modules, the main loop and data flow |
| [DESIGN.md](DESIGN.md) | See the OLED screens, feedback and web page design |
| [CODEBASE.md](CODEBASE.md) | Find which file and class does what |
| [DEVELOPMENT.md](DEVELOPMENT.md) | Build, flash, configure and extend the firmware |
| [API.md](API.md) | Call the HTTP API |
| [DEPLOYMENT.md](DEPLOYMENT.md) | Flash a device, update it over the air, upgrade versions |
| [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | Fix hardware, network, build or web problems |
| [TESTING.md](TESTING.md) | Check a change without and with hardware |
| [STYLEGUIDE.md](STYLEGUIDE.md) | Write code and docs that match the project |
| [DECISIONS.md](DECISIONS.md) | Learn why things are the way they are |
| [AI.md](AI.md) | Brief an AI assistant before it changes the project |
| [hardware.md](hardware.md) | Parts list, wiring diagrams, electrical notes |
| [firmware.md](firmware.md) | Configuration table, state machine, storage, LED and buzzer details |
| [web-ui.md](web-ui.md) | The web page tabs and the OTA flow |
