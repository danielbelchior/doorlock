# Deployment

"Deploying" Doorlock means getting a firmware image onto an ESP32 and getting that ESP32 onto a WiFi network. There is no server, container or cloud part.

## First install (USB)

The first flash must be over USB, because it writes the partition table with two 1.9 MB app slots (`min_spiffs.csv`) that OTA needs.

1. Wire the parts ([hardware.md](hardware.md)).
2. `pio run -t upload` (or Arduino IDE with **Partition Scheme > Minimal SPIFFS (1.9MB APP with OTA)**).
3. `pio device monitor` and check for `[boot] Teclado v2.2.0` and `[users] default users created` on a fresh device.

If the upload does not start, hold **BOOT** while it connects.

## Network provisioning

1. On the keypad: `2205`, `D`, `1`, `D`. The display shows `Teclado-XXXX` and `192.168.4.1`.
2. Join the open hotspot from a phone. The captive portal opens the page (or browse to `http://192.168.4.1/`).
3. **Rede** tab: **Procurar redes**, pick the network, type the password, **Salvar e conectar**.
4. The display shows the new IP, and the header clock appears once the time is synced. The hotspot closes about 60 s after the connection.
5. From then on: `http://keypad.local/` or the IP (menu option 2 shows it again).

Alternative: set `DEFAULT_WIFI_SSID` / `DEFAULT_WIFI_PASS` in `src/config.h` before the first flash. They only apply while nothing is stored.

## Updates over WiFi (OTA)

1. Build: `pio run` creates `.pio/build/esp32dev/firmware.bin` (Arduino IDE: **Sketch > Export Compiled Binary**).
2. Upload from the **Sistema** tab, or:

   ```sh
   curl -F "firmware=@.pio/build/esp32dev/firmware.bin" http://keypad.local/api/update
   ```

3. The display shows `Atualizando...`, then `Atualizado!` / `Reiniciando...` with green blinks and two beeps. The device reboots after about 1.5 s.
4. Check the version in the **Sistema** tab or `GET /api/status`.

Safety: the image is written to the inactive slot and only activated if the whole upload and the image check succeed. A failed upload leaves the running firmware in place (red blinks, long beep, display back to the prompt).

Rollback: there is no automatic rollback to the previous version. Upload the older `firmware.bin` the same way. Keep a copy of each released binary.

## What survives what

| Action | Users and challenge options | WiFi credentials | Partition table |
|---|---|---|---|
| Reboot or power loss | kept | kept | kept |
| OTA update | kept | kept | kept |
| USB flash (`pio run -t upload`) | kept | kept | rewritten (same NVS address) |
| `pio run -t erase` | erased (defaults return) | erased | erased |
| Changing `MAX_USERS` / name or PIN length | reset to defaults | kept | kept |

## Version upgrade paths

| From | To | Steps |
|---|---|---|
| v1 | v2.x | Move keypad pin 7 to GPIO 25 and pin 8 to GPIO 26, flash over USB (new partition table), set up WiFi from the hotspot. Users are kept. The v1 admin login is gone. |
| v2.0 | v2.1 | Optional: wire the buzzer (I/O 33, VCC 3V3, GND). Update over OTA. Every existing user keeps the challenge on. |
| v2.1 | v2.2 | Replace the LCD with a 0.96" SSD1306 OLED: SDA 21, SCL 22, VCC **3V3**, GND. Update over OTA or USB (same partition table). Users and WiFi are kept. Check `TIME_ZONE` for the clock. |

## Release checklist

1. Bump `FIRMWARE_VERSION`.
2. `pio run` without new warnings.
3. Flash a test device over USB and run [TESTING.md](TESTING.md).
4. Test OTA from the previous release to the new one.
5. Keep the `firmware.bin` and note the version.
