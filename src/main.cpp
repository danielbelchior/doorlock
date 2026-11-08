#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

// ---------- Configuração ----------
const char* WIFI_SSID  = "SUA_REDE";
const char* WIFI_PASS  = "SUA_SENHA_WIFI";
const char* ADMIN_USER = "admin";
const char* ADMIN_PASS = "troque-isto";

// ---------- LED SK6812 RGBW ----------
#define LED_PIN        23
#define LED_BRIGHTNESS 80      // 0-255
const unsigned long YELLOW_MS = 250;   // duração do amarelo a cada tecla
const unsigned long BLINK_MS  = 250;   // tempo aceso / apagado nos piscas

Adafruit_NeoPixel led(1, LED_PIN, NEO_GRBW + NEO_KHZ800);

const uint32_t COL_YELLOW = Adafruit_NeoPixel::Color(255, 160, 0, 0);
const uint32_t COL_RED    = Adafruit_NeoPixel::Color(255, 0, 0, 0);
const uint32_t COL_GREEN  = Adafruit_NeoPixel::Color(0, 255, 0, 0);

enum LedMode { LED_IDLE, LED_KEY, LED_BLINK };
LedMode ledMode = LED_IDLE;
uint32_t blinkColor = 0;
byte blinkStep = 0;
unsigned long ledNext = 0;

void ledSet(uint32_t c) {
  led.setPixelColor(0, c);
  led.show();
}

void ledKeyFlash() {
  ledMode = LED_KEY;
  ledSet(COL_YELLOW);
  ledNext = millis() + YELLOW_MS;
}

void ledBlink3(uint32_t c) {      // 3 piscas e apaga
  ledMode = LED_BLINK;
  blinkColor = c;
  blinkStep = 0;
  ledSet(c);
  ledNext = millis() + BLINK_MS;
}

void ledUpdate() {
  if (ledMode == LED_IDLE) return;
  if ((long)(millis() - ledNext) < 0) return;

  if (ledMode == LED_KEY) {
    ledSet(0);
    ledMode = LED_IDLE;
    return;
  }

  // LED_BLINK: passos 0,2,4 = aceso; 1,3 = apagado; 5 = fim
  blinkStep++;
  if (blinkStep >= 5) {
    ledSet(0);
    ledMode = LED_IDLE;
    return;
  }
  ledSet((blinkStep % 2) ? 0 : blinkColor);
  ledNext = millis() + BLINK_MS;
}

// ---------- Utilizadores (persistentes) ----------
const int MAX_USERS = 20;
const int NAME_LEN  = 16;
const int PIN_LEN   = 8;

struct User {
  char name[NAME_LEN + 1];
  char pin[PIN_LEN + 1];
};

User users[MAX_USERS];
int numUsers = 0;
Preferences prefs;

void saveUsers() {
  prefs.putUChar("count", numUsers);
  prefs.putBytes("users", users, sizeof(users));
}

void loadUsers() {
  numUsers = prefs.getUChar("count", 0);
  if (numUsers > MAX_USERS) numUsers = 0;
  if (numUsers > 0) {
    prefs.getBytes("users", users, sizeof(users));
  } else {
    strcpy(users[0].name, "Daniel"); strcpy(users[0].pin, "1234");
    strcpy(users[1].name, "Silvia"); strcpy(users[1].pin, "5678");
    numUsers = 2;
    saveUsers();
  }
}

int findPin(const String& pin) {
  for (int i = 0; i < numUsers; i++)
    if (pin == users[i].pin) return i;
  return -1;
}

// ---------- Teclado ----------
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {19, 18, 5, 17};
byte colPins[COLS] = {16, 4, 2, 15};
Keypad keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- Estado ----------
enum State { ENTER_PIN, GREETING, MATH, RESULT };
State state = ENTER_PIN;

const byte MAX_ANS_LEN = 6;
String input = "";
unsigned long until = 0;

char currentName[NAME_LEN + 1] = "";
long answer = 0;
String question = "";

// ---------- Web ----------
WebServer server(80);

String htmlEscape(const String& s) {
  String o;
  for (char c : s) {
    if (c == '&') o += "&amp;";
    else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;";
    else o += c;
  }
  return o;
}

String renderPage(const String& msg = "") {
  String h = F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
               "<meta name='viewport' content='width=device-width,initial-scale=1'>"
               "<title>Teclado ESP32</title><style>"
               "body{font-family:sans-serif;max-width:480px;margin:2em auto;padding:0 1em}"
               "table{width:100%;border-collapse:collapse}"
               "td,th{padding:.5em;border-bottom:1px solid #ccc;text-align:left}"
               "input{padding:.5em;margin:.2em 0;width:100%;box-sizing:border-box}"
               "button{padding:.5em 1em}.msg{background:#ffe9a8;padding:.6em;margin:1em 0}"
               "</style></head><body><h2>Usu&aacute;rios</h2>");
  if (msg.length()) h += "<div class='msg'>" + htmlEscape(msg) + "</div>";

  h += F("<table><tr><th>Nome</th><th>Senha</th><th></th></tr>");
  for (int i = 0; i < numUsers; i++) {
    h += "<tr><td>" + htmlEscape(users[i].name) + "</td><td>" + htmlEscape(users[i].pin) + "</td>";
    h += "<td><form method='POST' action='/del' onsubmit=\"return confirm('Remover?')\">"
         "<input type='hidden' name='idx' value='" + String(i) + "'>"
         "<button>Remover</button></form></td></tr>";
  }
  h += "</table>";

  h += F("<h3>Adicionar</h3><form method='POST' action='/add'>"
         "<input name='name' placeholder='Nome (m&aacute;x 16)' maxlength='16' required>"
         "<input name='pin' placeholder='Senha (1-8 d&iacute;gitos)' maxlength='8' "
         "pattern='[0-9]{1,8}' inputmode='numeric' required>"
         "<button>Adicionar</button></form>");
  h += "<p><small>" + String(numUsers) + "/" + String(MAX_USERS) + " usu&aacute;rios</small></p>";
  h += "</body></html>";
  return h;
}

bool requireAuth() {
  if (!server.authenticate(ADMIN_USER, ADMIN_PASS)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

void handleRoot() {
  if (!requireAuth()) return;
  server.send(200, "text/html", renderPage());
}

void handleAdd() {
  if (!requireAuth()) return;
  String name = server.arg("name"); name.trim();
  String pin  = server.arg("pin");  pin.trim();

  bool pinOk = pin.length() >= 1 && pin.length() <= PIN_LEN;
  for (char c : pin) if (c < '0' || c > '9') pinOk = false;

  bool nameOk = name.length() >= 1 && name.length() <= NAME_LEN;
  for (char c : name) if (c < 32 || c > 126) nameOk = false;

  String err = "";
  if (!nameOk) err = "Nome inválido (1-16 caracteres ASCII).";
  else if (!pinOk) err = "Senha inválida (1-8 dígitos).";
  else if (numUsers >= MAX_USERS) err = "Lista cheia.";
  else if (findPin(pin) >= 0) err = "Essa senha já existe.";

  if (err.length()) {
    server.send(400, "text/html", renderPage(err));
    return;
  }

  strncpy(users[numUsers].name, name.c_str(), NAME_LEN);
  users[numUsers].name[NAME_LEN] = '\0';
  strncpy(users[numUsers].pin, pin.c_str(), PIN_LEN);
  users[numUsers].pin[PIN_LEN] = '\0';
  numUsers++;
  saveUsers();

  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDel() {
  if (!requireAuth()) return;
  int idx = server.arg("idx").toInt();
  if (idx < 0 || idx >= numUsers) {
    server.send(400, "text/html", renderPage("Índice inválido."));
    return;
  }
  for (int i = idx; i < numUsers - 1; i++) users[i] = users[i + 1];
  numUsers--;
  memset(&users[numUsers], 0, sizeof(User));
  saveUsers();

  server.sendHeader("Location", "/");
  server.send(303);
}

// ---------- Ecrãs ----------
void showPrompt() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Digite a senha:");
  lcd.setCursor(0, 1);
  for (byte i = 0; i < input.length(); i++) lcd.print('*');
}

void showMath() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(question);
  lcd.print(" = ?");
  lcd.setCursor(0, 1);
  lcd.print("> ");
  lcd.print(input);
}

void showMessage(const char* l1, String l2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

// ---------- Lógica ----------
void newChallenge() {
  long a, b;
  switch (random(3)) {
    case 0:
      a = random(100, 500); b = random(100, 500);
      answer = a + b;
      question = String(a) + " + " + String(b);
      break;
    case 1:
      a = random(200, 900); b = random(100, a);
      answer = a - b;
      question = String(a) + " - " + String(b);
      break;
    default:
      a = random(12, 100); b = random(2, 10);
      answer = a * b;
      question = String(a) + " x " + String(b);
      break;
  }
}

void checkPin() {
  int i = findPin(input);
  input = "";
  if (i >= 0) {
    strcpy(currentName, users[i].name);
    showMessage("Bem-vindo(a):", currentName);
    ledBlink3(COL_GREEN);
    state = GREETING;
    until = millis() + 1500;
  } else {
    showMessage("ERRO", "Senha invalida");
    ledBlink3(COL_RED);
    state = RESULT;
    until = millis() + 3000;
  }
}

void checkAnswer() {
  long given = input.toInt();
  if (given == answer) showMessage("Correto! :)", currentName);
  else showMessage("Errado! :(", "Era: " + String(answer));
  input = "";
  state = RESULT;
  until = millis() + 3000;
}

void resetToStart() {
  input = "";
  currentName[0] = '\0';
  state = ENTER_PIN;
  showPrompt();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  randomSeed(esp_random());

  led.begin();
  led.setBrightness(LED_BRIGHTNESS);
  led.clear();
  led.show();

  prefs.begin("keypad", false);
  loadUsers();

  showMessage("A ligar WiFi...", "");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(250);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(WiFi.localIP());
    MDNS.begin("keypad");
    server.on("/", HTTP_GET, handleRoot);
    server.on("/add", HTTP_POST, handleAdd);
    server.on("/del", HTTP_POST, handleDel);
    server.begin();
    showMessage("IP:", WiFi.localIP().toString());
    delay(2500);
  } else {
    showMessage("Sem WiFi", "so teclado");
    delay(1500);
  }
  showPrompt();
}

void loop() {
  server.handleClient();
  ledUpdate();

  if (state == GREETING && millis() > until) {
    newChallenge();
    state = MATH;
    showMath();
  }
  if (state == RESULT && millis() > until) resetToStart();

  char key = keypad.getKey();
  if (!key) return;

  ledKeyFlash();   // amarelo a cada tecla (checkPin() pode sobrepor com verde/vermelho)

  switch (state) {
    case ENTER_PIN:
      if (key == '#') {
        if (input.length() > 0) checkPin();
      } else if (key == '*') {
        input = ""; showPrompt();
      } else if (key >= '0' && key <= '9' && input.length() < PIN_LEN) {
        input += key; showPrompt();
      }
      break;

    case MATH:
      if (key == '#') {
        if (input.length() > 0) checkAnswer();
      } else if (key == '*') {
        input = ""; showMath();
      } else if (key >= '0' && key <= '9' && input.length() < MAX_ANS_LEN) {
        input += key; showMath();
      }
      break;

    case RESULT:
      resetToStart();
      break;

    case GREETING:
      break;
  }
}