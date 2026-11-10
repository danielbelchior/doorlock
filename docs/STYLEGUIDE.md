# Style guide

Match the code around you. These rules describe what the code base already does.

## Languages

- Code, identifiers, comments, logs and documentation: **English**.
- Text people see on the display and the web page, and API error messages: **Portuguese (pt-BR)**.
- Display text: ASCII only, no accents, because the fonts have none (`Senha invalida`, `Nao desligue!`). Keep it short: the display shrinks long text to a smaller font and clips it if it still does not fit.
- Web page and API messages: proper Portuguese with accents (`Usuário`, `inválido`).

## C++

**Formatting**

- 2-space indent, no tabs. Lines up to about 100 characters.
- Braces on the same line. `public:` / `private:` indented by one space.
- Short single-statement `if` may stay on one line: `if (!ok) return;`.
- `#pragma once` in every header.

**Includes**, in this order with a blank line between groups:

1. The file's own header (in `.cpp` files).
2. Arduino and system headers (`<Arduino.h>`, `<WiFi.h>`, `<functional>`).
3. Project headers in alphabetical order (`"config.h"`, `"util.h"`).

`<Keypad.h>` is the exception: last include in `main.cpp` only.

**Naming**

| Kind | Style | Example |
|---|---|---|
| Classes, structs, enums | `PascalCase` | `UserStore`, `MenuAction` |
| Enum values | `PascalCase`, `enum class` only | `Sound::Success`, `State::EnterPin` |
| Functions and methods | `camelCase` | `findByPin()`, `startHotspot()` |
| Local variables, parameters | `camelCase` | `userIndex`, `withChallenge` |
| Private members | `camelCase_` with trailing underscore | `users_`, `nextMs_` |
| Constants | `UPPER_SNAKE_CASE` `constexpr` | `PIN_BUZZER`, `RESULT_MS` |
| Time values | suffix `Ms` / `_MS` | `deadlineMs_`, `GREETING_MS` |

**Practices**

- `constexpr` for constants; the only macro is `FIRMWARE_VERSION` (it is concatenated with string literals).
- File-local helpers and constants go in an anonymous namespace.
- No `delay()` outside `setup()`. Deadlines: `nextMs_ = millis() + x;` then `timeReached(nextMs_)`.
- No dynamic allocation beyond Arduino `String`. Call `reserve()` before building larger strings.
- Prefer `uint8_t` / `uint16_t` / `uint32_t` with the size that fits; use `static_cast` for conversions.
- Validate input in the owning module (`UserStore`, `Settings`) and return a result code or message; the web layer only translates.
- Callbacks are `std::function`, registered with `onSomething(handler)`.
- Log with a module prefix: `Serial.printf("[wifi] connected to '%s'\n", ...)`.

**Comments**

Explain why, not what: hidden constraints, invariants, hardware quirks and workarounds. Examples already in the code: why `<Keypad.h>` is last, why blobs are written before `count`, why the buzzer idle level is HIGH. Do not narrate code or mention tasks or tickets.

## Web page (`src/web_page.h`)

- One file: CSS in `<style>`, markup, plain JavaScript in `<script>` with `'use strict'`. No frameworks, no build step, no external resources (the page must work on the offline hotspot).
- CSS: compact one-line rules, colors only through the custom properties in `:root` (light and dark).
- JavaScript: `const`/`let`, arrow functions, `async`/`await`, single quotes, semicolons. DOM helpers `$` and `$$`.
- Every value from the device goes through `esc()` before it is put in HTML. The server also escapes with `jsonString()`.
- Talk to the device only through `api()`; show results with `toast()`.

## Documentation

- Markdown, sentence-case headings, short sentences, active voice.
- Tables for reference data; numbered lists for procedures.
- Inline code for file names, identifiers, keys, display text and values.
- Use commas, parentheses or separate sentences instead of em dashes.
- Keep facts in one place when possible and link to it; when a value is repeated (pins, timings, version), update every copy.
- Diagrams: Mermaid in Markdown; wiring as hand-written SVG in `docs/`.

## Commits

Short, imperative, lower-case subject lines (the history uses messages such as `refactor`). One logical change per commit when possible.
