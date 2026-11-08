# Teclado ESP32 com senhas, desafio de matemática e interface web

Projeto **recreativo** (não é uma fechadura de segurança) baseado num ESP32. O usuário digita uma senha num teclado matricial 4x4. Se a senha existir, o LCD mostra o nome da pessoa e, em seguida, propõe um desafio de matemática (ex.: `256 + 139 = ?`). Um LED RGBW dá feedback visual. As pessoas e senhas são geridas por uma página web e guardadas na flash do ESP32 (persistem após reinício e queda de energia).

> Este documento serve também de **contexto completo para uma IA** continuar o desenvolvimento. Ver a secção [Guia para quem continua o desenvolvimento](#guia-para-quem-continua-o-desenvolvimento).

---

## Funcionalidades

- Teclado de membrana 4x4 (16 teclas) lido com a biblioteca `Keypad`.
- LCD 16x2 com módulo I2C (PCF8574) a mostrar prompt, nome, desafio e resultado.
- Lista de usuários (`nome` + `senha`) guardada em NVS (`Preferences`), máximo 20 usuários.
- Página web (`http://keypad.local/` ou pelo IP) com autenticação básica HTTP para **adicionar** e **remover** usuários.
- Desafio de matemática aleatório após senha correta: soma, subtração (resultado sempre positivo) ou multiplicação.
- LED SK6812 5050 RGBW (1 unidade) com feedback não bloqueante:
  - amarelo ao pressionar qualquer tecla;
  - 3 piscadas verdes e apaga, quando a senha está correta;
  - 3 piscadas vermelhas e apaga, quando a senha está errada.
- Funciona sem WiFi (só teclado e LCD) se a ligação falhar no arranque.

---

## Hardware

| Qtd | Componente | Observações |
|---|---|---|
| 1 | ESP32 DevKit (30/38 pinos) | Testado como alvo "ESP32 Dev Module" |
| 1 | Teclado de membrana 4x4 (16 teclas) | 8 pinos: 4 linhas + 4 colunas, sem resistências externas |
| 1 | LCD 16x2 com backpack I2C (PCF8574) | Endereço `0x27` (alguns são `0x3F`) |
| 1 | LED SK6812 5050 RGBW | Modo `NEO_GRBW`, 1 pixel |
| 1 | Resistência 330 Ω | Em série no sinal DIN do LED |
| 1 | Condensador 100 nF cerâmico | Entre VCC e GND do LED, o mais perto possível |
| — | Fios, breadboard ou placa perfurada | Terra comum a todos os módulos |

### Pinagem

| Periférico | Pino do periférico | ESP32 |
|---|---|---|
| Teclado | 1, 2, 3, 4 (linhas L1–L4) | GPIO 19, 18, 5, 17 |
| Teclado | 5, 6, 7, 8 (colunas C1–C4) | GPIO 16, 4, 2, 15 |
| LCD I2C | SDA | GPIO 21 |
| LCD I2C | SCL | GPIO 22 |
| LCD I2C | VCC / GND | 5V (VIN) / GND |
| SK6812 | DIN (via 330 Ω) | GPIO 23 |
| SK6812 | VCC / GND | 5V (VIN) / GND |

### Esquema (resumo)

```
 Teclado 4x4                ESP32                        LCD I2C
 pino1 (L1) ------------- GPIO19        GND ----------- GND
 pino2 (L2) ------------- GPIO18        5V  ----------- VCC
 pino3 (L3) ------------- GPIO5         GPIO21 --------- SDA
 pino4 (L4) ------------- GPIO17        GPIO22 --------- SCL
 pino5 (C1) ------------- GPIO16
 pino6 (C2) ------------- GPIO4                          SK6812
 pino7 (C3) ------------- GPIO2         GPIO23 --[330R]-- DIN
 pino8 (C4) ------------- GPIO15        5V  ------------- VCC  (+100 nF para GND)
                                        GND ------------- GND
```

### Notas elétricas importantes

- **I2C a 5V:** o backpack PCF8574 tem pull-ups para o seu VCC (5V), logo SDA/SCL ficam a 5V enquanto o ESP32 é de 3,3V. Funciona na maioria dos casos, mas está fora de especificação. Solução mais segura: conversor de nível I2C (BSS138) ou pull-ups externos de 4,7 kΩ para 3,3V (removendo os do módulo).
- **Sinal do LED:** o SK6812 alimentado a 5V espera nível alto ≥ ~3,5V. O ESP32 dá 3,3V. Se o LED falhar ou piscar, usar um 74AHCT125 no DIN, ou alimentar o LED a ~4,3V com um díodo em série no VCC.
- **Strapping pins:** GPIO 2 e 15 são pinos de arranque. Se o ESP32 não arrancar ou não gravar com o teclado ligado, desligar o teclado ou mudar as colunas para GPIO 13, 14, 25, 26, 27, 32 ou 33.
- **Evitar** GPIO 6–11 (ligados à flash) e GPIO 34–39 (só entrada, sem pull-up interno, não servem para linhas do teclado).
- **Condensador de 100 nF:** serve de desacoplamento do controlador interno do LED. Com 1 LED, fios curtos e brilho baixo costuma funcionar sem ele, mas recomenda-se. Placas prontas de SK6812/WS2812 muitas vezes já o trazem.

---

## Software

### Ambiente

- **Arduino IDE** com o pacote de placas **esp32 by Espressif** (testado com a API clássica de `WebServer`; compatível com core 2.x e 3.x).
- Placa: `ESP32 Dev Module`. Velocidade serial: `115200`.
- O ficheiro `.ino` tem de ter o **mesmo nome da pasta** (ex.: `teclado_desafio/teclado_desafio.ino`). Alternativa PlatformIO: `src/main.cpp` com `#include <Arduino.h>` no topo.

### Bibliotecas (Gestor de Bibliotecas)

| Biblioteca | Autor |
|---|---|
| `Keypad` | Mark Stanley, Alexander Brevig |
| `LiquidCrystal I2C` | Frank de Brabander |
| `Adafruit NeoPixel` | Adafruit |

`WiFi`, `WebServer`, `ESPmDNS`, `Preferences` e `Wire` vêm com o core do ESP32.

### Configuração antes de gravar

No topo do código:

```cpp
const char* WIFI_SSID  = "SUA_REDE";
const char* WIFI_PASS  = "SUA_SENHA_WIFI";
const char* ADMIN_USER = "admin";
const char* ADMIN_PASS = "troque-isto";   // trocar!
```

Outras constantes úteis: `LED_PIN`, `LED_BRIGHTNESS` (0–255), `YELLOW_MS`, `BLINK_MS`, endereço do LCD em `LiquidCrystal_I2C lcd(0x27, 16, 2)`, `MAX_USERS`, `NAME_LEN`, `PIN_LEN`.

---

## Como funciona

### Teclas

| Tecla | Ação |
|---|---|
| `0`–`9` | Acrescenta dígito (senha mostrada como `*`, resposta do desafio em claro) |
| `#` | Confirma |
| `*` | Apaga o que foi digitado |
| `A`–`D` | Sem função (livres para extensões) |

### Máquina de estados

```
ENTER_PIN --(# com senha válida)--> GREETING (1,5 s) --> MATH --(# resposta)--> RESULT (3 s) --> ENTER_PIN
     |                                                                                ^
     +--(# com senha inválida)--> RESULT ("ERRO", 3 s) -------------------------------+
```

- `ENTER_PIN`: pede a senha. Limite de `PIN_LEN` (8) dígitos.
- `GREETING`: mostra "Bem-vindo(a): <nome>"; teclas ignoradas; ao fim de 1500 ms gera o desafio.
- `MATH`: mostra `<a> <op> <b> = ?` e aceita até `MAX_ANS_LEN` (6) dígitos.
- `RESULT`: mostra "Correto!"/"Errado!" (e a resposta certa) ou "ERRO"; ao fim de 3000 ms, ou em qualquer tecla, volta ao início.
- O nome do usuário é copiado para `currentName` ao autenticar, para o ecrã de resultado não depender do índice (que pode mudar se alguém remover um usuário pela web entretanto).

### Desafios (`newChallenge()`)

- Soma: `a, b ∈ [100, 500)`.
- Subtração: `a ∈ [200, 900)`, `b ∈ [100, a)` (resultado positivo).
- Multiplicação: `a ∈ [12, 100)`, `b ∈ [2, 10)`.

### LED (`ledUpdate()` não bloqueante)

Três modos: `LED_IDLE`, `LED_KEY` (amarelo `YELLOW_MS`), `LED_BLINK` (3 piscadas com `BLINK_MS` aceso/apagado). `ledUpdate()` é chamado em cada iteração do `loop()`. Amarelo = `(255, 160, 0, 0)` (G a 255 ficaria esverdeado). Se as cores saírem trocadas, trocar `NEO_GRBW` por `NEO_RGBW`.

### Interface web

| Rota | Método | Função |
|---|---|---|
| `/` | GET | Lista usuários e formulário de adicionar |
| `/add` | POST | Campos `name` (1–16 chars ASCII 32–126) e `pin` (1–8 dígitos). Rejeita lista cheia e senha duplicada |
| `/del` | POST | Campo `idx` (índice na lista) |

- Todas as rotas exigem autenticação básica HTTP (`ADMIN_USER` / `ADMIN_PASS`).
- Depois de `/add` ou `/del` com sucesso: redireciona (303) para `/`.
- Ao arrancar, o LCD mostra o IP durante 2,5 s; o mDNS anuncia `keypad.local`.

### Persistência

- Biblioteca `Preferences` (NVS), namespace `"keypad"`.
- Chaves: `"count"` (`uint8`, número de usuários) e `"users"` (blob binário com o array `User users[MAX_USERS]` completo).
- `struct User { char name[17]; char pin[9]; }` (26 bytes) → blob de 520 bytes com `MAX_USERS = 20`.
- `saveUsers()` só é chamado ao adicionar/remover (evita desgaste desnecessário).
- Primeira execução (count = 0): cria `Daniel/1234` e `Silvia/5678`.
- **Atenção:** alterar `MAX_USERS`, `NAME_LEN`, `PIN_LEN` ou a `struct User` muda o tamanho do blob e invalida os dados guardados. Ao mudar, apagar a NVS (`prefs.clear()` uma vez no `setup()`) ou escrever migração.

---

## Limitações e problemas conhecidos

1. **Segurança fraca (por desenho):** senhas em texto simples na flash e na página web; autenticação básica HTTP sem cifra; credenciais de admin fixas no código. Aceitável só para uso recreativo em rede local. Não usar para controlar acesso real.
2. **Sem limite de tentativas:** é possível tentar senhas indefinidamente no teclado.
3. **LED durante saudação/resultado:** `ledKeyFlash()` é chamado a qualquer tecla antes do `switch`, por isso uma tecla pressionada durante a piscada verde/vermelha **interrompe** a piscada e passa a amarelo (mesmo em estados que ignoram teclas). Correção possível: só chamar `ledKeyFlash()` quando `ledMode != LED_BLINK`, ou nos estados `ENTER_PIN`/`MATH`.
4. **LED não sinaliza o resultado do desafio:** só a senha tem verde/vermelho.
5. **Comparações de tempo:** `millis() > until` não é seguro na passagem do contador (~49 dias). Preferir `(long)(millis() - until) > 0`.
6. **WiFi:** ligação bloqueante até 15 s no `setup()`; não há reconexão automática se cair depois; sem modo AP de configuração (SSID/senha fixos no código).
7. **Nomes só ASCII:** o LCD não mostra acentos; a página rejeita caracteres fora de 32–126.
8. **Nomes duplicados** são permitidos; **senhas duplicadas** não (validado só em `/add`).
9. **Sem protecção contra pedidos GET destrutivos:** `/del` e `/add` usam POST, mas não há token CSRF.

---

## Ideias de evolução (roadmap)

- Limite de tentativas com bloqueio temporário (ex.: 3 erros → 30 s) e indicação no LCD/LED.
- Pontuação por usuário (campo `score` na `struct User`) e ranking na página web.
- Tempo limite para responder ao desafio e níveis de dificuldade (tecla `A`–`D` para escolher).
- LED a sinalizar também acerto/erro do desafio.
- Editar usuário existente na página (nome/senha) e botão para exportar/importar lista.
- Guardar senhas como hash (SHA-256 + sal) e mostrar `****` na página.
- HTTPS ou, no mínimo, trocar a autenticação básica por sessão com token.
- Publicar eventos por MQTT para o Home Assistant (login ok/erro, nome, resultado do desafio) ou migrar para ESPHome (`matrix_keypad`).
- Saída para relé/buzzer (feedback sonoro, abertura de uma tranca de brincadeira).
- WiFiManager / portal de configuração, OTA (`ArduinoOTA`) e reconexão automática.
- Suporte a OLED SSD1306 (U8g2/Adafruit_SSD1306) em vez do LCD.
- Dividir o sketch em ficheiros (`users.h/.cpp`, `led.h/.cpp`, `web.h/.cpp`, `ui.h/.cpp`).

---

## Guia para quem continua o desenvolvimento

Instruções destinadas a uma IA (ou a uma pessoa) que vá modificar este código:

**Regras de arquitetura**
- O `loop()` tem de permanecer **não bloqueante**: nada de `delay()` fora do `setup()`. Temporizações usam `millis()` (`until`, `ledNext`). O servidor web (`server.handleClient()`) e o LED (`ledUpdate()`) correm em cada iteração.
- Novo estado = novo valor em `enum State`, tratamento em `loop()` (transição por tempo) e no `switch (state)` (teclas).
- Todo o texto no LCD tem de caber em **16 colunas** e ser ASCII. `lcd.print` de texto longo é cortado/ envolvido.
- Uma tecla pressionada devolve-se uma vez (`keypad.getKey()`), sem repetição.
- Modificações na lista de usuários: alterar `users[]` e `numUsers`, depois chamar `saveUsers()`. Nunca guardar na flash dentro de laços ou a cada tecla.
- Se alterar a `struct User` ou as constantes de tamanho, tratar a migração/limpeza da NVS (ver secção Persistência).
- Strings vindas da web passam por `htmlEscape()` antes de serem postas em HTML.

**Convenções**
- Idioma da interface e dos comentários: português. Identificadores em inglês.
- Sem dependências além das listadas (preferir o que já vem com o core ESP32).
- Manter os GPIO da tabela de pinagem. Se mudar algum, atualizar o código **e** este README.

**Como testar (sem hardware, mentalmente / com Serial)**
1. Primeira gravação: usuários `Daniel/1234` e `Silvia/5678` existem.
2. `1234 #` → "Bem-vindo(a): Daniel", LED pisca 3x verde, depois aparece desafio.
3. `9999 #` → "ERRO / Senha invalida", LED pisca 3x vermelho, volta ao início em 3 s.
4. Resposta certa/errada ao desafio → "Correto!"/"Errado!" e volta ao início.
5. Na página web: adicionar usuário, reiniciar o ESP32, confirmar que continua lá; remover; senha duplicada deve dar erro.
6. Qualquer tecla deve acender o LED amarelo por ~250 ms.

**Critérios de "pronto" para novas funcionalidades**
- Compila sem avisos novos para `ESP32 Dev Module`.
- Não introduz `delay()` no `loop()`.
- Documentação (este README) atualizada: funcionalidades, pinagem, limitações e roadmap.

---


## Histórico de decisões

1. Teclado 4x4 ligado a 8 GPIOs, biblioteca `Keypad`.
2. Display: LCD 16x2 I2C (PCF8574) em SDA=21 / SCL=22.
3. Lista de nomes e senhas em array; senha digitada + `#` confirma; nome no LCD se existir, "ERRO" se não.
4. Projeto **recreativo**: após senha correta, desafio de matemática (soma, subtração, multiplicação).
5. Interface web para adicionar/remover usuários, com persistência em NVS (`Preferences`) em vez de `EEPROM.h`.
6. LED único SK6812 5050 RGBW: amarelo por tecla, 3 piscadas verdes (certa) ou vermelhas (errada), depois apaga.

## Licença

Projeto pessoal/recreativo. Defina a licença que preferir (ex.: MIT).
