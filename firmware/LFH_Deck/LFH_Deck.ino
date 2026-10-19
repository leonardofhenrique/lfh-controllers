/*
  LFH_Deck - LFH Deck (v1)
  Controlador MIDI USB com 4 faders e 21 botoes com LED, configuravel pelo navegador.

  - Faders: iguais aos do Fader 4 (entrada, faixa, inversao, apelido; um canal so).
  - Botoes 1-20: mandam Note On/Off com a nota igual ao numero do botao, no canal
    da camada atual. Cada camada tem o proprio canal MIDI e a propria cor.
  - Botao 21: troca a camada (1 -> 2 -> 3 -> 1); o LED dele mostra a cor da camada.
  - Os nomes dos botoes (por camada) ficam na EEPROM para o configurador mostrar.

  Ligacoes (ver docs/lfh-deck-layout.svg):
  - Faders em A3, A2, A1, A0 (da esquerda para a direita)
  - Botoes 1-16 no multiplexador CD74HC4067: SIG no pino 4 (A6), S0-S3 nos pinos 9, 8, 7, 6
  - Botoes 17-20 nos pinos 10, 16, 14, 15; botao 21 no pino 5
  - LEDs WS2812 no pino 3, 21 em cadeia

  Placa: Arduino Leonardo / Micro / Pro Micro (ATmega32u4)
  Bibliotecas: MIDIUSB, FastLED e LFHCore (pasta libraries/ deste repositorio)
  Para o dispositivo aparecer como "LFH Deck", escolha a placa "LFH > LFH Deck (...)".
*/

#include <MIDIUSB.h>
#include <EEPROM.h>
#include <FastLED.h>
#include <LFHCore.h>

/*********FIRMWARE*********/
const byte FW_VERSION = 1;

// Comandos so do Deck (os comuns ficam em LFHSysex.h)
const byte CMD_GET_NAMES = 0x10;  // pede uma pagina de nomes: <pagina>
const byte CMD_NAMES = 0x11;      // pagina de nomes: <pagina> + 10 nomes de 12 bytes
const byte CMD_SET_NAMES = 0x12;  // grava uma pagina de nomes: <pagina> + 10 nomes
const byte CMD_LAYER = 0x13;      // avisa a camada atual: <camada 0-2>


/*********HARDWARE*********/
const byte MUX_SIG = 4;                         // = A6
const byte MUX_SEL[4] = { 9, 8, 7, 6 };         // S0, S1, S2, S3
const byte DIRECT_PINS[4] = { 10, 16, 14, 15 }; // botoes 17-20
const byte LAYER_PIN = 5;                       // botao 21
const byte LED_PIN = 3;

const byte NUM_KEYS = 21;     // botoes fisicos
const byte NUM_BUTTONS = 20;  // botoes que mandam nota (o 21 so troca a camada)
const byte LAYER_KEY = 20;    // indice do botao 21

// canal do multiplexador de cada botao 1-16 (desce pela coluna)
const byte MUX_CHANNEL[16] = { 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 };
// posicao na cadeia de LEDs de cada botao 1-21 (linha por linha, da esquerda para a direita)
const byte LED_OF[NUM_KEYS] = { 0, 1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 13, 15, 16, 17, 18, 4, 9, 14, 19, 20 };

// entradas que os faders podem usar (A6-A11 estao ocupadas pelo multiplexador e botoes)
const byte analogPins[] = { A0, A1, A2, A3, A4, A5 };
const byte NUM_PINS = sizeof(analogPins);

const unsigned long DEBOUNCE_MS = 10;
const unsigned int LED_MAX_MA = 400;  // a USB entrega ~500 mA para tudo


/*********CONFIGURACAO*********/
const byte NUM_FADERS = 4;
const byte NUM_LAYERS = 3;

struct Layer {
  byte channel;  // 0-15
  byte r, g, b;  // cor do LED da camada, 0-127
};

struct Config {
  byte magic;
  byte version;
  byte brightness;  // 0-127
  Layer layers[NUM_LAYERS];
  Fader faders[NUM_FADERS];
};

const byte CONFIG_MAGIC = 0x44;
const byte CONFIG_VERSION = 1;

// Nomes dos botoes: 3 camadas x 20 botoes x 12 bytes = 720 bytes, direto na
// EEPROM (nao cabem na RAM). Vao em 6 paginas de 10 nomes pelo SysEx.
const int NAMES_ADDR = 128;
const byte NAMES_PER_PAGE = 10;
const byte NAME_PAGES = NUM_LAYERS * NUM_BUTTONS / NAMES_PER_PAGE;

Config config;
byte layer = 0;  // camada atual (nao fica salva: liga sempre na camada 1)


void handleSysex(byte cmd, const byte *data, byte len);

LFHSysex sysex(LFH_DEVICE_DECK, handleSysex);
LFHFaders faders(config.faders, NUM_FADERS, analogPins, NUM_PINS);

CRGB leds[NUM_KEYS];
bool ledsDirty = true;

bool keyDown[NUM_KEYS];
unsigned long keyChanged[NUM_KEYS];
byte keyChannel[NUM_KEYS];  // canal em que a nota foi ligada (para desligar no mesmo)


void setup() {
  for (byte s = 0; s < 4; s++) pinMode(MUX_SEL[s], OUTPUT);
  pinMode(MUX_SIG, INPUT_PULLUP);
  for (byte k = 0; k < 4; k++) pinMode(DIRECT_PINS[k], INPUT_PULLUP);
  pinMode(LAYER_PIN, INPUT_PULLUP);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_KEYS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MA);

  loadConfig();
  updateLeds();
  delay(2000);
}

void loop() {

  sysex.read();
  buttons();

  if (faders.monitorOn()) faders.monitorTick(sysex);
  else faders.update();

  updateLeds();
}


/*********BOTOES*********/
bool readKey(byte k) {
  if (k < 16) {
    byte ch = MUX_CHANNEL[k];
    for (byte s = 0; s < 4; s++) digitalWrite(MUX_SEL[s], (ch >> s) & 1);
    delayMicroseconds(5);  // tempo para o multiplexador trocar de canal
    return digitalRead(MUX_SIG) == LOW;
  }
  if (k < NUM_BUTTONS) return digitalRead(DIRECT_PINS[k - 16]) == LOW;
  return digitalRead(LAYER_PIN) == LOW;
}

void buttons() {
  for (byte k = 0; k < NUM_KEYS; k++) {
    bool down = readKey(k);
    if (down == keyDown[k] || millis() - keyChanged[k] < DEBOUNCE_MS) continue;

    keyDown[k] = down;
    keyChanged[k] = millis();
    ledsDirty = true;

    if (k == LAYER_KEY) {
      if (down) setLayer((layer + 1) % NUM_LAYERS);
      continue;
    }
    if (down) keyChannel[k] = config.layers[layer].channel;
    noteOn(keyChannel[k], k + 1, down ? 127 : 0);
  }
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t event = { 0x09, (byte)(0x90 | channel), pitch, velocity };
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}

void setLayer(byte l) {
  layer = l;
  ledsDirty = true;
  sysex.send(CMD_LAYER, &layer, 1);
}


/*********LEDS*********/
CRGB layerColor(byte l) {
  const Layer &c = config.layers[l];
  return CRGB(c.r * 2, c.g * 2, c.b * 2);
}

// LED 21: cor da camada; botoes 1-20: acendem na cor da camada enquanto apertados
void updateLeds() {
  if (!ledsDirty) return;
  ledsDirty = false;

  CRGB color = layerColor(layer);
  for (byte k = 0; k < NUM_BUTTONS; k++) leds[LED_OF[k]] = keyDown[k] ? color : CRGB::Black;
  leds[LED_OF[LAYER_KEY]] = color;
  FastLED.setBrightness(config.brightness * 2);
  FastLED.show();
}


/*********CONFIG / EEPROM*********/
void loadDefaults() {
  config.magic = CONFIG_MAGIC;
  config.version = CONFIG_VERSION;
  config.brightness = 20;

  // camada 1 laranja no canal 1, camada 2 azul no canal 2, camada 3 verde no canal 3
  const byte colors[NUM_LAYERS][3] = { { 117, 47, 1 }, { 38, 90, 122 }, { 37, 119, 85 } };
  for (byte l = 0; l < NUM_LAYERS; l++) {
    config.layers[l].channel = l;
    config.layers[l].r = colors[l][0];
    config.layers[l].g = colors[l][1];
    config.layers[l].b = colors[l][2];
  }

  // faders em A3, A2, A1, A0 -> CC 1 a 4, canal 1, faixa 42-127, 0 no fim do curso
  lfhFaderDefaults(config.faders, NUM_FADERS);
}

void sanitize(Config &c) {
  c.brightness &= 0x7F;
  for (byte l = 0; l < NUM_LAYERS; l++) {
    c.layers[l].channel &= 0x0F;
    c.layers[l].r &= 0x7F;
    c.layers[l].g &= 0x7F;
    c.layers[l].b &= 0x7F;
  }
  for (byte i = 0; i < NUM_FADERS; i++) lfhSanitizeFader(c.faders[i], NUM_PINS);
}

void loadConfig() {
  EEPROM.get(0, config);
  if (config.magic != CONFIG_MAGIC || config.version != CONFIG_VERSION) {
    loadDefaults();
  }
  sanitize(config);
  faders.reset();
  ledsDirty = true;
}

void saveConfig() {
  EEPROM.put(0, config);  // put so grava os bytes que mudaram
}

int nameAddr(byte layerIndex, byte button) {
  return NAMES_ADDR + (layerIndex * NUM_BUTTONS + button) * LFH_NAME_LEN;
}

void clearNames() {
  for (int a = NAMES_ADDR; a < nameAddr(NUM_LAYERS, 0); a++) EEPROM.update(a, 0);
}


/*********SYSEX*********/
// data sao os bytes depois do comando
void handleSysex(byte cmd, const byte *data, byte len) {

  if (cmd == LFH_CMD_GET) {
    sendConfig();
  }
  else if (cmd == LFH_CMD_SET) {
    // mesma ordem do sendConfig, sem os campos fixos
    if (len < NUM_FADERS * LFH_FADER_BYTES + 1 + NUM_LAYERS * 4) return;

    Config c;
    c.magic = CONFIG_MAGIC;
    c.version = CONFIG_VERSION;
    const byte *p = data;
    for (byte i = 0; i < NUM_FADERS; i++) p = lfhDecodeFader(c.faders[i], p);
    c.brightness = *p++;
    for (byte l = 0; l < NUM_LAYERS; l++) {
      c.layers[l].channel = *p++;
      c.layers[l].r = *p++;
      c.layers[l].g = *p++;
      c.layers[l].b = *p++;
    }
    sanitize(c);

    config = c;
    saveConfig();
    faders.reset();
    ledsDirty = true;
    sysex.ack();
    sendConfig();
  }
  else if (cmd == LFH_CMD_RESET) {
    loadDefaults();
    saveConfig();
    clearNames();
    faders.reset();
    ledsDirty = true;
    sysex.ack();
    sendConfig();
  }
  else if (cmd == LFH_CMD_MONITOR) {
    if (len >= 1) faders.setMonitor(data[0] != 0);
  }
  else if (cmd == CMD_GET_NAMES) {
    if (len >= 1 && data[0] < NAME_PAGES) sendNames(data[0]);
  }
  else if (cmd == CMD_SET_NAMES) {
    if (len < 1 + NAMES_PER_PAGE * LFH_NAME_LEN || data[0] >= NAME_PAGES) return;
    saveNames(data[0], data + 1);
    sysex.ack();
  }
}

// configuracao: os primeiros campos sao iguais aos do Fader 4
// versao, n de faders, n de entradas, 18 bytes por fader,
// n de botoes, n de camadas, camada atual, brilho, 4 bytes por camada (canal, R, G, B)
void sendConfig() {
  byte msg[3 + NUM_FADERS * LFH_FADER_BYTES + 4 + NUM_LAYERS * 4];
  byte *p = msg;
  *p++ = FW_VERSION;
  *p++ = NUM_FADERS;
  *p++ = NUM_PINS;
  for (byte i = 0; i < NUM_FADERS; i++) p = lfhEncodeFader(config.faders[i], p);
  *p++ = NUM_KEYS;
  *p++ = NUM_LAYERS;
  *p++ = layer;
  *p++ = config.brightness;
  for (byte l = 0; l < NUM_LAYERS; l++) {
    *p++ = config.layers[l].channel;
    *p++ = config.layers[l].r;
    *p++ = config.layers[l].g;
    *p++ = config.layers[l].b;
  }
  sysex.send(LFH_CMD_CONFIG, msg, p - msg);
}

// pagina 0 = camada 1, botoes 1-10; pagina 1 = camada 1, botoes 11-20; pagina 2 = camada 2...
void sendNames(byte page) {
  byte msg[1 + NAMES_PER_PAGE * LFH_NAME_LEN];
  msg[0] = page;
  int addr = nameAddr(page / 2, (page % 2) * NAMES_PER_PAGE);
  for (byte n = 0; n < NAMES_PER_PAGE; n++) {
    char *name = (char *)msg + 1 + n * LFH_NAME_LEN;
    for (byte k = 0; k < LFH_NAME_LEN; k++) name[k] = EEPROM.read(addr++);
    lfhSanitizeName(name, LFH_NAME_LEN);  // EEPROM nova vem com 0xFF
  }
  sysex.send(CMD_NAMES, msg, sizeof(msg));
}

void saveNames(byte page, const byte *data) {
  int addr = nameAddr(page / 2, (page % 2) * NAMES_PER_PAGE);
  for (byte n = 0; n < NAMES_PER_PAGE; n++) {
    char name[LFH_NAME_LEN];
    memcpy(name, data + n * LFH_NAME_LEN, LFH_NAME_LEN);
    lfhSanitizeName(name, LFH_NAME_LEN);
    for (byte k = 0; k < LFH_NAME_LEN; k++) EEPROM.update(addr++, name[k]);
  }
}
