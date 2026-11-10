# Testing

There are no automated tests and no CI. Quality comes from a clean build, quick static checks, and a manual checklist on real hardware.

## Levels

| Level | Needs | Catches |
|---|---|---|
| 1. Static checks | Node.js, clang (no ESP32 toolchain) | JavaScript syntax errors, C++ syntax and type errors in hardware-independent files |
| 2. Build | PlatformIO with the ESP32 platform (downloads on first use) | Everything the compiler sees, `-Wall` warnings, flash size |
| 3. API smoke test | A running device and `curl` | Validation, JSON shapes, storage |
| 4. Manual checklist | The wired device | Real behavior: keypad, display, LED, buzzer, WiFi, OTA |

Run the levels that match the change. A documentation-only change needs none.

## 1. Static checks without the toolchain

Use these when the ESP32 toolchain is not installed and must not be downloaded.

**Web page JavaScript.** Extract the `<script>` block from `src/web_page.h` and run `node --check` on it:

```sh
python3 - <<'PY'
src = open('src/web_page.h', encoding='utf-8').read()
open('/tmp/page.js', 'w').write(src.split('<script>')[1].split('</script>')[0])
PY
node --check /tmp/page.js && echo JS_OK
```

**C++ syntax with stub headers.** `buzzer.cpp`, `users.cpp`, `game.cpp` and `display.cpp` only need small fakes of the Arduino API, so they can be checked with the host compiler:

```sh
clang++ -std=gnu++17 -fsyntax-only -Wall -Wextra -I/tmp/doorlock_stubs src/buzzer.cpp
clang++ -std=gnu++17 -fsyntax-only -Wall -Wextra -I/tmp/doorlock_stubs src/users.cpp
clang++ -std=gnu++17 -fsyntax-only -Wall -Wextra -I/tmp/doorlock_stubs src/game.cpp
clang++ -std=gnu++17 -fsyntax-only -Wall -Wextra -I/tmp/doorlock_stubs src/display.cpp
```

`/tmp/doorlock_stubs/` holds throwaway headers that are not part of the repository; recreate them when needed:

| Stub | Must provide |
|---|---|
| `Arduino.h` | `String` (construct from `const char*` and integers, `length`, `[]`, `+=`, `+`, `==`, `c_str`, `toInt`, `trim`, `reserve`, `substring`, `indexOf(char, from)`), `millis`, `random`, `pinMode`, `digitalWrite`, `LOW`, `HIGH`, `OUTPUT`, `Serial` with `println`/`printf`, `byte` |
| `Preferences.h` | `begin`, `isKey`, `getBytesLength`, `getBytes`, `putBytes`, `getUChar`, `putUChar` |
| `U8g2lib.h` | class `U8G2_SSD1306_128X64_NONAME_F_HW_I2C` with constructor `(rotation, reset, clock, data)`; `U8G2_R0`, `U8G2_R2`, `U8X8_PIN_NONE`; the fonts used in `display.cpp` (`u8g2_font_helvB14_tr`, `u8g2_font_helvB12_tr`, `u8g2_font_helvB10_tr`, `u8g2_font_helvB08_tr`, `u8g2_font_helvR10_tr`, `u8g2_font_helvR08_tr`, `u8g2_font_5x8_tr`); methods `setI2CAddress`, `setBusClock`, `begin`, `setContrast`, `setFontMode`, `setPowerSave`, `clearBuffer`, `sendBuffer`, `setDrawColor`, `setFont`, `getAscent`, `getStrWidth`, `drawStr`, `drawHLine`, `drawBox`, `drawFrame`, `drawRBox`, `drawDisc`, `drawLine` |
| `Adafruit_NeoPixel.h` | `NEO_GRBW`, `NEO_KHZ800`, constructor `(n, pin, type)`, `begin`, `setBrightness`, `setPixelColor`, `show` |

`main.cpp`, `web_ui.cpp` and `wifi_service.cpp` depend on `WiFi`, `WebServer`, `Update`, `DNSServer` and `Keypad`; checking them needs the real ESP32 core, so review them by reading. Say so when reporting results.

## 2. Build

```sh
pio run
```

Pass criteria: success, no new warnings from `src/`, and the size report fits the 1.9 MB app slot.

## 3. API smoke test

With the device on the network (replace the host if needed):

```sh
H=http://keypad.local
curl -s $H/api/status
curl -s $H/api/users
curl -s -d name=Teste -d pin=9090 -d challenge=0 $H/api/users     # {"ok":true}
curl -s -d name=Outro -d pin=9090 $H/api/users                    # duplicate PIN, 400
curl -s -d name=Ação -d pin=1 $H/api/users                        # invalid name, 400
curl -s -d index=abc $H/api/users/delete                          # not found, 400
curl -s $H/api/users                                              # Teste has "challenge":false
```

Then remove the test user with its index. Expected messages are in [API.md](API.md#errors).

## 4. Manual checklist (hardware)

Boot and network:

1. Fresh device (`pio run -t erase`, then flash): the display shows `Sem WiFi` / `Nao configurado`; no hotspot appears.
2. `2205 D` opens the menu; `*` closes it, and so do 30 s idle. `1234 D` does nothing.
3. Menu `1 D`: the display shows `Teclado-XXXX` / `192.168.4.1`; joining the open hotspot opens the page.
4. Save WiFi in **Rede**: device connects, the display shows the IP, `http://keypad.local/` works, hotspot closes about a minute later.
5. Menu `2 D` shows the IP; menu `3 D` shows `Reiniciando...` and restarts.
6. Router off for a few minutes: no hotspot; router on: reconnects by itself.

Game, light and sound:

7. Any key: yellow LED for about 250 ms and a short click.
8. `1234 #`: `Bem-vindo(a)!` / `Daniel` with a check icon, 3 green blinks, two quick beeps, then a challenge.
9. Correct answer: `Correto!`, green blinks, two beeps. Wrong answer: `Errado!` and `Resposta: <answer>`, red blinks, one long beep.
10. `9999 #`: `Acesso negado` / `Senha invalida`, red blinks, one long beep, back to the prompt after 3 s.
11. Keys pressed during a blink or a beep pattern do not cut it.
12. The buzzer is silent between sounds, also right after boot.

Users and challenge option:

13. Add, edit and remove users; a duplicate PIN is rejected; changes survive a reboot.
14. Uncheck **Fazer o desafio de matemática** for a user: the list shows **Sem desafio**; their PIN shows the welcome for 3 s and returns to the prompt with no question. Check it again: the question returns. The option survives a reboot and moves correctly when a user above is removed.

Updates:

15. Upload a new `firmware.bin`: progress bar, `Atualizando...`, reboot into the new version (check **Sistema**).
16. Upload a random non-firmware file: error toast, red blinks, long beep, the old firmware keeps running.
17. Upgrade from v2.0 with existing users: every user shows **Com desafio**.

Display:

18. The header clock (`HH:MM`) appears a few seconds after WiFi connects and matches local time. Without WiFi there is no clock.
19. The network icon shows filled bars when connected, outlined bars with a slash when offline, and an `AP` badge while the hotspot is on (menu `1 D`).
20. Set `DISPLAY_OFF_HOUR` and `DISPLAY_ON_HOUR` around the current hour (for example 14 and 15 at 14:20) and flash. At the empty PIN prompt the screen turns off after 30 s without a key. A key wakes it and is typed. The screen does not turn off while a PIN is half-typed or on another screen.
21. Long names shrink to a smaller font; no screen shows accented or broken characters.

## Reporting

When you report a change, list which levels you ran, what passed, what failed and what you skipped (for example "not built: toolchain not installed").
