#pragma once

#include <Arduino.h>

#define FIRMWARE_VERSION "2.2.0"

// ---------------------------------------------------------------------------
// Pins (ESP32 DevKit). Read docs/hardware.md before changing any of these:
// avoid the strapping pins (0, 2, 12, 15), the flash pins (6-11) and the
// input-only pins (34-39).
// ---------------------------------------------------------------------------
constexpr uint8_t KEYPAD_ROWS = 4;
constexpr uint8_t KEYPAD_COLS = 4;

constexpr uint8_t PIN_KEYPAD_ROW_1 = 19;  // keypad connector pin 1
constexpr uint8_t PIN_KEYPAD_ROW_2 = 18;  // pin 2
constexpr uint8_t PIN_KEYPAD_ROW_3 = 5;   // pin 3
constexpr uint8_t PIN_KEYPAD_ROW_4 = 17;  // pin 4
constexpr uint8_t PIN_KEYPAD_COL_1 = 16;  // pin 5
constexpr uint8_t PIN_KEYPAD_COL_2 = 4;   // pin 6
constexpr uint8_t PIN_KEYPAD_COL_3 = 25;  // pin 7 (GPIO2 in v1)
constexpr uint8_t PIN_KEYPAD_COL_4 = 26;  // pin 8 (GPIO15 in v1)

constexpr uint8_t PIN_I2C_SDA = 21;
constexpr uint8_t PIN_I2C_SCL = 22;
constexpr uint8_t PIN_LED = 23;
constexpr uint8_t PIN_BUZZER = 33;  // MH-FMD I/O

// ---------------------------------------------------------------------------
// 0.96" 128x64 SSD1306 OLED on I2C
// ---------------------------------------------------------------------------
constexpr uint8_t DISPLAY_I2C_ADDRESS = 0x3C;  // some modules use 0x3D
constexpr uint32_t DISPLAY_I2C_CLOCK_HZ = 400000;
constexpr uint8_t DISPLAY_CONTRAST = 160;  // 0-255; lower slows burn-in
constexpr bool DISPLAY_ROTATE_180 = false;  // true if the module is mounted upside down

// The screen turns off from DISPLAY_OFF_HOUR to DISPLAY_ON_HOUR (local time)
// while the PIN prompt is idle, to save the OLED from burn-in. A key wakes it;
// it turns off again DISPLAY_NIGHT_WAKE_MS after the last key. Needs the
// clock from NTP: until it is set, the screen stays on. Equal hours disable it.
constexpr uint8_t DISPLAY_OFF_HOUR = 0;  // 00:00
constexpr uint8_t DISPLAY_ON_HOUR = 8;   // 08:00
constexpr uint32_t DISPLAY_NIGHT_WAKE_MS = 30000;

// POSIX TZ string for the clock. Amsterdam: CET (UTC+1) / CEST (UTC+2) com DST automático.
constexpr const char* TIME_ZONE = "CET-1CEST,M3.5.0,M10.5.0/3";
constexpr const char* NTP_SERVER_1 = "pool.ntp.org";
constexpr const char* NTP_SERVER_2 = "time.google.com";

// ---------------------------------------------------------------------------
// SK6812 RGBW status LED
// ---------------------------------------------------------------------------
constexpr uint8_t LED_BRIGHTNESS = 80;  // 0-255
constexpr uint32_t LED_KEY_FLASH_MS = 250;
constexpr uint32_t LED_BLINK_MS = 250;
constexpr uint8_t LED_BLINK_COUNT = 3;

// ---------------------------------------------------------------------------
// MH-FMD active buzzer module. Power it from 3V3, not 5V (see
// docs/hardware.md). Set BUZZER_ACTIVE_LOW to false for a module or circuit
// that switches the buzzer on with a HIGH input.
// ---------------------------------------------------------------------------
constexpr bool BUZZER_ACTIVE_LOW = true;
constexpr uint16_t BUZZER_KEY_MS = 30;  // key click length

// ---------------------------------------------------------------------------
// Keypad game
// ---------------------------------------------------------------------------
constexpr uint32_t GREETING_MS = 1500;
constexpr uint32_t RESULT_MS = 3000;
constexpr uint32_t NOTICE_MS = 4000;
constexpr uint32_t MENU_INFO_MS = 10000;     // IP screen from the menu
constexpr uint32_t HOTSPOT_INFO_MS = 20000;  // hotspot screen from the menu
constexpr uint32_t MENU_TIMEOUT_MS = 30000;
constexpr uint32_t PIN_ENTRY_TIMEOUT_MS = 20000;
constexpr uint32_t CHALLENGE_TIMEOUT_MS = 60000;
constexpr uint8_t ANSWER_MAX_LEN = 6;

// Typing this code and then D opens the keypad menu. Must be digits only and
// at most USER_PIN_LEN long. A user with the same PIN still logs in with #.
constexpr const char* MENU_CODE = "2205";

// ---------------------------------------------------------------------------
// Users. Changing any of these changes the stored blob size: the stored list
// is then discarded and the default users are restored on the next boot.
// ---------------------------------------------------------------------------
constexpr uint8_t MAX_USERS = 20;
constexpr uint8_t USER_NAME_LEN = 16;
constexpr uint8_t USER_PIN_LEN = 8;

// ---------------------------------------------------------------------------
// Network. WiFi credentials are normally set from the web UI and stored in
// flash; these are only first-boot defaults.
// ---------------------------------------------------------------------------
constexpr const char* WIFI_HOSTNAME = "keypad";  // http://keypad.local/
constexpr const char* DEFAULT_WIFI_SSID = "";
constexpr const char* DEFAULT_WIFI_PASS = "";

// Open setup hotspot (no password), started only from the keypad menu.
constexpr const char* SETUP_AP_PREFIX = "Teclado";  // hotspot name: Teclado-XXXX

constexpr uint32_t WIFI_RETRY_MS = 15000;          // retry interval without hotspot
constexpr uint32_t WIFI_RETRY_WITH_AP_MS = 30000;  // retries disturb hotspot clients
constexpr uint32_t WIFI_AP_LINGER_MS = 60000;      // hotspot stays up after a newly saved network connects
