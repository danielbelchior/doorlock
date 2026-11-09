# Troubleshooting

Start with the serial log (`pio device monitor`, 115200 baud). Module prefixes are listed in [DEVELOPMENT.md](DEVELOPMENT.md#serial-log).

## Hardware

| Symptom | Cause and fix |
|---|---|
| LCD backlight on, no text | Turn the contrast potentiometer. If still blank, set `DISPLAY_I2C_ADDRESS` to `0x3F`. |
| LCD completely dark | Check 5V, GND, SDA (21), SCL (22). |
| Keys give wrong digits | Keypad connector reversed, or rows and columns swapped. Pin 1 is usually on the left. |
| A group of keys is dead | One loose row or column wire; the pattern of dead keys points to it. |
| Board does not boot or flash with the keypad connected | A wire is on a strapping pin (0, 2, 12, 15). Use the pins in `src/config.h`. |
| LED colors wrong | Change `NEO_GRBW` to `NEO_RGBW` in `src/led.cpp`. |
| LED dark or flickering | 3.3V data into a 5V LED; see [hardware.md](hardware.md#electrical-notes). Check the 330 Ω resistor and ground. |
| Buzzer silent | VCC must be 3V3, I/O on GPIO 33, common GND. Try `BUZZER_ACTIVE_LOW = false` if your module switches on with HIGH. |
| Buzzer beeps all the time | VCC on 5V instead of 3V3 (the 3.3V HIGH cannot switch the PNP off), or `BUZZER_ACTIVE_LOW` wrong. |
| Buzzer too quiet | Normal for an active buzzer at 3.3V. Use a 5V buzzer through an NPN transistor and `BUZZER_ACTIVE_LOW = false`. |
| Buzzer crackles instead of beeping | It is being driven with PWM or `tone()`. An active buzzer needs plain on/off. |
| WROVER board misbehaves on keypad R4 / C1 | GPIO 16 and 17 are used by PSRAM; move them (for example to 27 and 32). |

## Keypad behavior

| Symptom | Cause and fix |
|---|---|
| No math question after a correct PIN | The user has the challenge turned off. Check the tag in the **Usuários** tab. |
| `2205 D` does nothing | Something was typed before; press `*` and try again. Only `MENU_CODE` followed by `D` opens the menu. |
| Typed PIN disappears | 20 s without a key clears it (`PIN_ENTRY_TIMEOUT_MS`). |
| Keypad frozen for a few seconds | A firmware upload is running (LCD shows `Atualizando...`). |
| Default users came back | Flash was erased, or `MAX_USERS` / `USER_NAME_LEN` / `USER_PIN_LEN` changed. Log shows `stored list does not match this firmware`. |

## Network

| Symptom | Cause and fix |
|---|---|
| `Sem WiFi / nao configurado` | No network saved. Open the hotspot (menu 1) and configure WiFi. |
| `Sem WiFi / Conectando...` forever | Wrong password, out of range, or a 5 GHz-only network (the ESP32 is 2.4 GHz only). Re-enter it from the hotspot. |
| No hotspot appears | By design: it only starts from menu option 1. |
| Captive page does not open | Browse to `http://192.168.4.1/`. Some phones need mobile data turned off. |
| `keypad.local` does not resolve | mDNS not supported (Android, some Windows). Use the IP from menu option 2. |
| Page stops responding after saving WiFi | The device moved to the new network and has a new IP. Read it on the LCD. |
| Hotspot closes too early | It closes 60 s after a network saved from it connects (`WIFI_AP_LINGER_MS`). |

## Web page and API

| Symptom | Cause and fix |
|---|---|
| `Usuário não encontrado. Recarregue a página.` | The list changed (another tab or a delete). Reload. |
| `Essa senha já está em uso por outro usuário.` | PINs must be unique. |
| Changes do not show | Hard reload; the page is served with `no-cache`. |
| `Falha na atualização: ...` | Wrong file (not an ESP32 app image), image larger than 1.9 MB, or the device was flashed with the default partition table. Flash once over USB. |
| Upload works but old version still runs | The upload was interrupted or failed the image check. Check the `[ota]` log. |

## Build

| Symptom | Cause and fix |
|---|---|
| `Keypad.h` macro conflicts (`OPEN`, `CLOSED`) | `<Keypad.h>` must stay the last include in `main.cpp`. |
| Libraries or platform missing | PlatformIO downloads them on the first build; it needs internet once. Offline machines cannot build. |
| `Sketch too big` / does not fit | Select the Minimal SPIFFS partition scheme (Arduino IDE) or keep `board_build.partitions = min_spiffs.csv`. |
| Arduino IDE cannot find `main` | Rename `main.cpp` to `doorlock.ino` inside a folder named `doorlock`. |
