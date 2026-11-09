# Hardware

## Parts list

| Qty | Part | Notes |
|---|---|---|
| 1 | ESP32 DevKit (30 or 38 pins) | Build target "ESP32 Dev Module" (`esp32dev`). Prefer a WROOM module; see the GPIO 16/17 note below. |
| 1 | 4x4 membrane keypad (16 keys) | 8-pin connector: 4 rows and 4 columns. No external resistors needed. |
| 1 | 16x2 LCD with I2C backpack (PCF8574) | Address `0x27` (some are `0x3F`). |
| 1 | SK6812 5050 RGBW LED | One pixel, `NEO_GRBW` color order. |
| 1 | 330 Ω resistor | In series with the LED data line (DIN). |
| 1 | 100 nF ceramic capacitor | Between LED VCC and GND, as close to the LED as possible. |
| - | Wires, breadboard or perfboard | All parts share a common ground. |

## Wiring diagram

![Wiring diagram](wiring.svg)

## Pin table

| Part | Part pin | Signal | ESP32 pin | Changed from v1 |
|---|---|---|---|---|
| Keypad | 1 | Row R1 | GPIO 19 | |
| Keypad | 2 | Row R2 | GPIO 18 | |
| Keypad | 3 | Row R3 | GPIO 5 | |
| Keypad | 4 | Row R4 | GPIO 17 | |
| Keypad | 5 | Column C1 | GPIO 16 | |
| Keypad | 6 | Column C2 | GPIO 4 | |
| Keypad | 7 | Column C3 | **GPIO 25** | was GPIO 2 |
| Keypad | 8 | Column C4 | **GPIO 26** | was GPIO 15 |
| LCD backpack | SDA | I2C data | GPIO 21 | |
| LCD backpack | SCL | I2C clock | GPIO 22 | |
| LCD backpack | VCC | Power | 5V (VIN) | |
| LCD backpack | GND | Ground | GND | |
| SK6812 | DIN | LED data, through 330 Ω | GPIO 23 | |
| SK6812 | VCC | Power | 5V (VIN) | |
| SK6812 | GND | Ground | GND | |

All pin numbers live in `src/config.h`. If you change one, update this table and both diagrams.

## Board pinout

The diagram below shows where each wire lands on the common 30-pin DevKit V1. Other DevKit boards order their pins differently, so always check the labels printed on your board.

![ESP32 DevKit pinout](esp32-devkit-pinout.svg)

## Electrical notes

- **I2C at 5V.** The PCF8574 backpack pulls SDA and SCL up to its own VCC (5V), while the ESP32 runs at 3.3V. This usually works but is outside the ESP32 specification. The safer options are an I2C level shifter (BSS138 type), or removing the backpack pull-ups and adding 4.7 kΩ pull-ups to 3.3V.
- **LED data level.** An SK6812 powered from 5V expects a high level of about 3.5V or more, and the ESP32 outputs 3.3V. If the LED stays dark or flickers, add a 74AHCT125 buffer on DIN, or power the LED from about 4.3V by putting a diode in series with its VCC.
- **100 nF capacitor.** It decouples the LED's internal driver. One LED on short wires at low brightness often works without it, but it is recommended. Many ready-made SK6812/WS2812 boards already include one.
- **Strapping pins.** GPIO 0, 2, 12 and 15 decide how the ESP32 boots. Keep them free. v2 moved the keypad columns off GPIO 2 and 15 for this reason. GPIO 5 is also a strapping pin, but a keypad row is safe there because it is only connected to something while a key is pressed.
- **Pins to avoid.** GPIO 6 to 11 are wired to the flash chip. GPIO 34 to 39 are input-only and have no internal pull-ups, so they cannot drive a keypad.
- **GPIO 16 and 17.** On WROVER modules (the ones with PSRAM) these two pins are used by the PSRAM. On such a board, move keypad R4 and C1 to free pins (for example GPIO 27 and GPIO 32) and update `src/config.h`.
- **Power.** Everything runs from the USB 5V through the DevKit's VIN pin. At brightness 80 the single LED draws only a few milliamps.

## Troubleshooting

| Symptom | Likely cause and fix |
|---|---|
| LCD backlight is on but no text | Turn the contrast potentiometer on the backpack. If that does not help, the address is probably `0x3F`: change `DISPLAY_I2C_ADDRESS` in `src/config.h`. |
| LCD shows nothing at all | Check 5V, GND, SDA (GPIO 21) and SCL (GPIO 22). |
| Keys give the wrong digits | The keypad connector is reversed or the rows and columns are swapped. Pin 1 is usually on the left when the keys face you. |
| Some keys do nothing | A loose row or column wire. Each row and each column serves 4 keys, so the pattern of dead keys tells you which wire it is. |
| LED colors are wrong (for example red shows as green) | Your LED uses a different color order. Change `NEO_GRBW` to `NEO_RGBW` in `src/led.cpp`. |
| LED stays dark or flickers | See "LED data level" above. Also check the 330 Ω resistor and the ground connection. |
| Board does not boot or flash with the keypad connected | A keypad wire is on a strapping pin. Use the pins in the table above. |
