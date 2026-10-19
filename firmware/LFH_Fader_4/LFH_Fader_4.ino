/*
  LFH_Fader_4 - LFH Fader 4 (v2)
  Controlador MIDI USB de 4 faders configuravel pelo navegador.

  A configuracao (pino, canal, CC, faixa de valores, inversao e apelido de cada
  fader) fica salva na EEPROM e e alterada pela pagina configurador.html via
  SysEx, sem precisar recompilar este codigo.

  Placas: ATmega32u4 (Arduino Leonardo, Micro, Pro Micro)
  Bibliotecas necessarias: MIDIUSB e LFHCore (pasta libraries/ deste repositorio)

  Nome USB "LFH Fader 4": o nome vem da placa escolhida, nao deste codigo.
  Para o dispositivo aparecer como "LFH Fader 4", em Tools > Board escolha
  "LFH > LFH Fader 4 (Arduino Micro)" ou "LFH Fader 4 (Arduino Leonardo / Pro Micro)".
  Essas placas ficam em Documents\Arduino\hardware\LFH.
*/

#include <MIDIUSB.h>
#include <EEPROM.h>
#include <LFHCore.h>

/*********FIRMWARE*********/
const byte FW_VERSION = 2;


/*********PINOS ANALOGICOS*********/
// indice 0 = A0, 1 = A1, ...
const byte analogPins[] = {
  A0, A1, A2, A3,
#ifdef PIN_A4
  A4,
#endif
#ifdef PIN_A5
  A5,
#endif
#ifdef PIN_A6
  A6,
#endif
#ifdef PIN_A7
  A7,
#endif
#ifdef PIN_A8
  A8,
#endif
#ifdef PIN_A9
  A9,
#endif
#ifdef PIN_A10
  A10,
#endif
#ifdef PIN_A11
  A11,
#endif
};
const byte NUM_PINS = sizeof(analogPins);


/*********CONFIGURACAO*********/
const byte NUM_FADERS = 4;

struct Config {
  byte magic;
  byte version;
  Fader faders[NUM_FADERS];
};

const byte CONFIG_MAGIC = 0x5A;
const byte CONFIG_VERSION = 2;

Config config;


void handleSysex(byte cmd, const byte *data, byte len);

LFHSysex sysex(LFH_DEVICE_FADER4, handleSysex);
LFHFaders faders(config.faders, NUM_FADERS, analogPins, NUM_PINS);


void setup() {

  loadConfig();
  delay(2000);
}

void loop() {

  sysex.read();

  if (faders.monitorOn()) faders.monitorTick(sysex);
  else faders.update();
}


/*********CONFIG / EEPROM*********/
void loadDefaults() {
  // igual ao projeto original: faders em A3, A2, A1, A0 -> CC 1 a 4,
  // canal 1, faixa 42-127 e 0 no fim do curso
  config.magic = CONFIG_MAGIC;
  config.version = CONFIG_VERSION;
  lfhFaderDefaults(config.faders, NUM_FADERS);
}

void sanitize(Config &c) {
  for (byte i = 0; i < NUM_FADERS; i++) lfhSanitizeFader(c.faders[i], NUM_PINS);
}

void loadConfig() {
  EEPROM.get(0, config);
  if (config.magic != CONFIG_MAGIC || config.version != CONFIG_VERSION) {
    loadDefaults();
  }
  sanitize(config);
  faders.reset();
}

void saveConfig() {
  EEPROM.put(0, config);  // put so grava os bytes que mudaram
}


/*********SYSEX*********/
// data sao os bytes depois do comando
void handleSysex(byte cmd, const byte *data, byte len) {

  if (cmd == LFH_CMD_GET) {
    sendConfig();
  }
  else if (cmd == LFH_CMD_SET) {
    if (len < NUM_FADERS * LFH_FADER_BYTES) return;

    Config c;
    c.magic = CONFIG_MAGIC;
    c.version = CONFIG_VERSION;
    const byte *p = data;
    for (byte i = 0; i < NUM_FADERS; i++) p = lfhDecodeFader(c.faders[i], p);
    sanitize(c);

    config = c;
    saveConfig();
    faders.reset();
    sysex.ack();
    sendConfig();
  }
  else if (cmd == LFH_CMD_RESET) {
    loadDefaults();
    saveConfig();
    faders.reset();
    sysex.ack();
    sendConfig();
  }
  else if (cmd == LFH_CMD_MONITOR) {
    if (len >= 1) faders.setMonitor(data[0] != 0);
  }
}

void sendConfig() {
  byte msg[3 + NUM_FADERS * LFH_FADER_BYTES];
  byte n = 0;
  msg[n++] = FW_VERSION;
  msg[n++] = NUM_FADERS;
  msg[n++] = NUM_PINS;
  byte *p = msg + n;
  for (byte i = 0; i < NUM_FADERS; i++) p = lfhEncodeFader(config.faders[i], p);
  sysex.send(LFH_CMD_CONFIG, msg, p - msg);
}
