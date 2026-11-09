# Design

How the device looks, sounds and reads to the people who use it. Text shown to people is in Portuguese; everything else (code, docs) is in English.

## Principles

- **Immediate feedback.** Every key press lights the LED and clicks the buzzer. Every outcome has a screen, a color and a sound.
- **Never stuck.** Every screen has a timeout back to the PIN prompt, and most also accept any key to go back.
- **Result feedback wins.** A key press never cuts a green or red blink or a success or error beep pattern short.
- **Fits the hardware.** LCD text is ASCII, at most 16 characters per line, no accents.
- **Toy-grade security is visible.** The web page does not pretend to be secure (no fake login).

## Keypad

| Key | Action |
|---|---|
| `0` to `9` | Type a digit (PIN shows `*`, answers show digits) |
| `#` | Confirm |
| `*` | Clear the input, or leave the menu |
| `D` | After `2205`: open the menu. In the menu: confirm the chosen option |
| `A`, `B`, `C` | Not used (reserved) |

Limits: PIN up to 8 digits (`USER_PIN_LEN`), answer up to 6 digits (`ANSWER_MAX_LEN`). Extra digits are ignored.

## LCD screens

| State | Line 1 | Line 2 | Leaves after |
|---|---|---|---|
| Boot | `Teclado v2.1.0` | `Iniciando...` | setup ends |
| `EnterPin` | `Digite a senha:` | one `*` per digit | half-typed input clears after 20 s |
| `Greeting` | `Bem-vindo(a):` | name | 1.5 s, then the challenge |
| `Challenge` | `256 + 139 = ?` | `> ` + typed answer | `#`, or 60 s idle |
| `Result`, correct | `Correto! :)` | name | 3 s or any key |
| `Result`, wrong | `Errado! :(` | `Era: <answer>` | 3 s or any key |
| `Result`, unknown PIN | `ERRO` | `Senha invalida` | 3 s or any key |
| `Result`, no challenge | `Bem-vindo(a):` | name | 3 s or any key |
| `Menu` | `1Hotspot 2IP  >` + chosen digit | `3Reiniciar *Sair` | `*`, `D`, or 30 s idle |
| `Notice`, IP | `IP:` | the IP | 10 s (menu) or 4 s (on connect) |
| `Notice`, no WiFi | `Sem WiFi` | `nao configurado` / `Conectando...` | 4 s or 10 s |
| `Notice`, hotspot | `Teclado-XXXX` | `192.168.4.1` | 20 s |
| `Busy` | `Atualizando...` / `Atualizado!` / `Reiniciando...` | `Nao desligue!` / `Reiniciando...` / empty | OTA end or restart |

Challenge operands: addition `a, b` in [100, 500); subtraction `a` in [200, 900), `b` in [100, a); multiplication `a` in [12, 100), `b` in [2, 10). The `x` sign is used for multiplication.

## Light and sound

| Event | LED | Buzzer (active, fixed pitch) |
|---|---|---|
| Any key | Yellow 250 ms | Click 30 ms |
| Correct PIN or answer, OTA success | 3 green blinks | Two quick beeps (70 on, 70 off, 70 on) |
| Wrong PIN or answer, OTA failure | 3 red blinks | One long beep, 600 ms |
| Menu opened | 1 green blink | Key click only |

The buzzer is active, so it cannot change pitch: sounds differ by rhythm only. Yellow is `(255, 160, 0)` because full green makes the SK6812 look green.

## Web page

One HTML page, three tabs: **Usuários**, **Rede**, **Sistema**. Details of each tab are in [web-ui.md](web-ui.md).

Layout:

- Phones: one column, full-width buttons.
- 768 px and wider: two columns (`.grid.two`, list on the left, form on the right).
- 1024 px and wider: the user list itself uses two columns.
- Touch targets of 44 to 48 px, `font-size: 16px` on inputs (no iOS zoom), safe-area insets for notched screens.

Design tokens (CSS custom properties in `src/web_page.h`):

| Token | Light | Dark | Use |
|---|---|---|---|
| `--bg` | `#f2f4f7` | `#0f1418` | Page and input background |
| `--card` | `#fff` | `#182026` | Cards, header |
| `--text` | `#1d2939` | `#e7ecf0` | Body text |
| `--muted` | `#667085` | `#98a2b3` | Hints, secondary text |
| `--line` | `#e4e7ec` | `#2a343c` | Borders, neutral tags |
| `--accent` | `#0e7c66` | `#33c3a0` | Primary buttons, active tab, focus, checkboxes |
| `--danger` | `#c4320a` | `#f97066` | Destructive buttons, error toasts |
| `--ok` / `--on-ok` | `#d1fadf` / `#05603a` | `#0c3b2a` / `#75e0a7` | Connected pill, "Com desafio" tag |

Components: `.card`, `.pill` (network status), `.tag` / `.tag.on` (challenge status), `.hint`, `.row`, `.block`, `button.primary`, `button.danger`, `label.check` (checkbox row), `#toast` (status messages, red when `.bad`).

Interaction rules:

- Destructive or disruptive actions (remove user, change WiFi, OTA, reboot) ask `confirm()` first.
- Every API result shows a toast. Errors show the server's Portuguese message.
- PINs are masked as dots in the list, with a **Mostrar** toggle.
- The status refreshes every 10 s.
