# HTTP API

The web server listens on port 80. There is no authentication and no CORS handling: the API is meant for the embedded page and for `curl` on a trusted network.

- Base URL: `http://keypad.local` or `http://<device IP>`; in hotspot mode `http://192.168.4.1`.
- Request bodies: `application/x-www-form-urlencoded` (use `curl -d`), except the firmware upload (`multipart/form-data`, `curl -F`).
- Responses: JSON with `Cache-Control: no-store`, except `/` (HTML).
- Success for writes: `200 {"ok":true}`.
- Errors: `{"error":"<Portuguese message>"}` with status 400, 500 or 503.

## Routes

| Method | Path | Parameters | Success response |
|---|---|---|---|
| GET | `/` | | The web page |
| GET | `/api/status` | | Status object (below) |
| GET | `/api/users` | | `{"max":20,"users":[User]}` |
| POST | `/api/users` | `name`, `pin`, `challenge`? | `{"ok":true}` |
| POST | `/api/users/update` | `index`, `name`, `pin`, `challenge`? | `{"ok":true}` |
| POST | `/api/users/delete` | `index` | `{"ok":true}` |
| POST | `/api/wifi/scan` | | `{"ok":true}`, scan runs in the background |
| GET | `/api/wifi/scan` | | `{"scanning":true}` or `{"scanning":false,"networks":[Network]}` |
| POST | `/api/wifi` | `ssid`, `pass` | `{"ok":true}`, then reconnects |
| POST | `/api/reboot` | | `{"ok":true}`, reboots about 1 s later |
| POST | `/api/update` | one file field (any name) | `{"ok":true}`, reboots about 1.5 s later |
| any | other paths | | `404 Not found` (text); in hotspot mode, unknown hosts get `302` to `http://192.168.4.1/` |

## Objects

**User**

```json
{"name": "Daniel", "pin": "1234", "challenge": true}
```

Users are addressed by their 0-based position in the list (`index`). Positions shift after a delete, so reload the list before editing.

**Network** (scan result; hidden networks are skipped, duplicates are not merged)

```json
{"ssid": "Casa", "rssi": -58, "secure": true}
```

**Status**

```json
{
  "version": "2.3.0",
  "uptime": 93784,
  "heap": 182340,
  "hostname": "keypad",
  "users": {"count": 2, "max": 20},
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

`uptime` is in seconds, `heap` in bytes. `ip` is empty and `rssi` is 0 when not connected; `apIp` is empty when the hotspot is off.

## Parameters and validation

| Parameter | Rule |
|---|---|
| `name` | Trimmed; 1 to 16 printable ASCII characters (32 to 126) |
| `pin` | Trimmed; 1 to 8 digits; unique among users (its own PIN is ignored on update) |
| `challenge` | `1`, `true`, `on` = on; `0`, `false`, `off` = off. Missing or other values: on for add, unchanged for update |
| `index` | 1 to 3 digits, must point at an existing user |
| `ssid` | 1 to 32 characters |
| `pass` | Empty (open network) or 8 to 63 characters |

## Errors

| Status | Message | Cause |
|---|---|---|
| 400 | `Nome inválido: use 1 a 16 caracteres, sem acentos.` | Bad `name` |
| 400 | `Senha inválida: use 1 a 8 dígitos.` | Bad `pin` |
| 400 | `Essa senha já está em uso por outro usuário.` | Duplicate `pin` |
| 400 | `A lista de usuários está cheia.` | 20 users already |
| 400 | `Usuário não encontrado. Recarregue a página.` | Bad or stale `index` |
| 500 | `Erro ao gravar na memória do dispositivo.` | NVS write failed (users or WiFi) |
| 400 | `O nome da rede deve ter de 1 a 32 caracteres.` | Bad `ssid` |
| 400 | `A senha WiFi deve ter de 8 a 63 caracteres (ou ficar vazia em redes abertas).` | Bad `pass` |
| 503 | `Não foi possível procurar redes agora. Tente novamente.` | Scan could not start |
| 503 | `A procura de redes falhou. Tente novamente.` | Scan failed |
| 400 | `Nenhum arquivo recebido.` | `/api/update` without a file |
| 500 | `Falha na atualização: <reason>` | Bad image, flash error or interrupted upload |

## Examples

```sh
curl http://keypad.local/api/status
curl http://keypad.local/api/users

# Add users
curl -d name=Ana -d pin=4321 http://keypad.local/api/users
curl -d name=Bia -d pin=1111 -d challenge=0 http://keypad.local/api/users

# Edit the first user; the challenge option is kept
curl -d index=0 -d name=Daniel -d pin=2580 http://keypad.local/api/users/update

# Turn the challenge off for the second user
curl -d index=1 -d name=Silvia -d pin=5678 -d challenge=0 http://keypad.local/api/users/update

# Delete the third user
curl -d index=2 http://keypad.local/api/users/delete

# Scan and save WiFi
curl -X POST http://keypad.local/api/wifi/scan
curl http://keypad.local/api/wifi/scan
curl -d ssid=Casa -d pass=minhasenha http://keypad.local/api/wifi

# Firmware update and reboot
curl -F "firmware=@.pio/build/esp32dev/firmware.bin" http://keypad.local/api/update
curl -X POST http://keypad.local/api/reboot
```
