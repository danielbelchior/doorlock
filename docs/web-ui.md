# Web page and HTTP API

The web page is a single page stored in the firmware. Its text is in Portuguese. It works in any modern browser and adapts to the screen:

- **Phones:** one column, full-width buttons.
- **Tablets (768 px and wider):** two columns, for example the user list next to the add/edit form.
- **Large screens (1024 px and wider):** the user list itself uses two columns.

Buttons and fields are at least 44 to 48 px tall for touch use. The page follows the system light or dark mode.

![Web UI on a tablet](web-ui-tablet.png)

## Opening the web page

| Situation | Address |
|---|---|
| Device connected to your network | `http://keypad.local/`, or the IP shown on the LCD (keypad menu option 2) |
| Setup hotspot (`Teclado-XXXX`, open, started with keypad menu option 1) | Opens by itself on most phones and tablets; otherwise `http://192.168.4.1/` |

There is no login. Anyone on your network, or connected to the hotspot, can use the page.

> `keypad.local` relies on mDNS. It works on macOS, iOS, Linux and recent Windows versions. On Android, use the IP address.

## Tabs

### Usuários (users)

- The list shows every user with a masked PIN and a **Com desafio** or **Sem desafio** tag. **Mostrar** reveals the PIN, **Editar** loads the user into the form, and **Remover** deletes the user after a confirmation.
- The form adds a new user, or saves changes while editing. **Cancelar** leaves edit mode.
- **Fazer o desafio de matemática** (checked by default) decides whether the user must answer a math question after typing the PIN. When it is unchecked, the keypad greets the user by name for 3 s and goes back to the PIN prompt.
- Rules: the name has 1 to 16 characters without accents; the PIN has 1 to 8 digits and must be different from every other user's PIN. Up to 20 users.

### Rede (network)

- **Estado da conexão** shows whether the device is connected, the network name, IP, signal strength and, when it is open, the setup hotspot. A note reminds you that the hotspot is opened from the keypad menu.
- **Configurar WiFi:** tap **Procurar redes** to scan, tap a network to fill in its name, type its password (leave it empty for an open network) and tap **Salvar e conectar**. The device saves the network and reconnects right away. If its IP changes, the page stops responding. Read the new IP on the LCD (keypad menu option 2). If you saved the network while connected to the hotspot, the hotspot closes about a minute after the device connects.

### Sistema (system)

- **Dispositivo:** firmware version, network name, uptime, free memory and user count.
- **Atualizar firmware:** choose a `firmware.bin` and tap **Enviar e instalar**. A progress bar shows the upload. The device reboots into the new firmware, and the page reloads about 10 s later.
- **Reiniciar:** reboots the device. Users and settings are kept.

## Firmware updates (OTA)

1. Build the firmware: `pio run` creates `.pio/build/esp32dev/firmware.bin`. In the Arduino IDE, use **Sketch > Export Compiled Binary**.
2. Upload it from the Sistema tab, or from a terminal:

   ```sh
   curl -F "firmware=@.pio/build/esp32dev/firmware.bin" http://keypad.local/api/update
   ```

The device checks the image header and writes it to the inactive app slot. It only switches to the new slot if the whole upload succeeded, so a failed or interrupted upload leaves the current firmware running. The first flash of v2 must be over USB, because it installs the partition table with larger app slots.

## HTTP API

No route requires a login. POST bodies are form-encoded (`application/x-www-form-urlencoded`), except the firmware upload, which is `multipart/form-data`.

Successful writes return `{"ok":true}`. Errors return a 4xx or 5xx status with `{"error":"<message in Portuguese>"}`.

| Method | Path | Parameters | Response |
|---|---|---|---|
| GET | `/` | | The web page |
| GET | `/api/status` | | Device, WiFi and user summary (see below) |
| GET | `/api/users` | | `{"max":20,"users":[{"name":"Daniel","pin":"1234","challenge":true}]}` |
| POST | `/api/users` | `name`, `pin`, optional `challenge` | Adds a user. `challenge` defaults to on |
| POST | `/api/users/update` | `index`, `name`, `pin`, optional `challenge` | Replaces the user at position `index` (0-based). Without `challenge`, the current setting is kept |
| POST | `/api/users/delete` | `index` | Removes the user at position `index` |
| POST | `/api/wifi/scan` | | Starts a WiFi scan in the background |
| GET | `/api/wifi/scan` | | `{"scanning":true}` while scanning, then `{"scanning":false,"networks":[{"ssid":"Casa","rssi":-58,"secure":true}]}` |
| POST | `/api/wifi` | `ssid`, `pass` | Saves the network and reconnects |
| POST | `/api/reboot` | | Reboots after about 1 s |
| POST | `/api/update` | file field (any name) | Installs the firmware and reboots after about 1.5 s |

`challenge` accepts `1`, `true` or `on` to turn the math challenge on, and `0`, `false` or `off` to turn it off. Any other value counts as missing.

Example `/api/status` response:

```json
{
  "version": "2.1.0",
  "uptime": 93784,
  "heap": 182340,
  "hostname": "keypad",
  "users": { "count": 2, "max": 20 },
  "wifi": {
    "configured": true,
    "connected": true,
    "ssid": "Casa",
    "ip": "192.168.1.42",
    "rssi": -58,
    "ap": false,
    "apSsid": "Teclado-1A2B",
    "apIp": ""
  }
}
```

More examples:

```sh
# List users
curl http://keypad.local/api/users

# Add a user
curl -d name=Ana -d pin=4321 http://keypad.local/api/users

# Add a user who skips the math challenge
curl -d name=Bia -d pin=1111 -d challenge=0 http://keypad.local/api/users

# Change the PIN of the first user (the challenge setting is kept)
curl -d index=0 -d name=Daniel -d pin=2580 http://keypad.local/api/users/update
```

In hotspot mode, any request for an unknown host is redirected to `http://192.168.4.1/`. This is what makes phones and tablets show the setup page automatically.
