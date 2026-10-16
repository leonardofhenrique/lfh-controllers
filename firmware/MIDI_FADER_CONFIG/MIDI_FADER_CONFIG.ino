/*
  MIDI_FADER_CONFIG
  Controlador MIDI USB de faders configuravel pelo navegador.

  A configuracao (quantidade de faders, pinos, canal, CC e faixa de valores)
  fica salva na EEPROM e e alterada pela pagina configurador.html via SysEx,
  sem precisar recompilar este codigo.

  Placas: ATmega32u4 (Arduino Leonardo, Micro, Pro Micro)
  Biblioteca necessaria: MIDIUSB

  Nome USB "LFH Fader 4": o nome vem da placa escolhida, nao deste codigo.
  Para o dispositivo aparecer como "LFH Fader 4", em Tools > Board escolha
  "LFH > LFH Fader 4 (Arduino Micro)" ou "LFH Fader 4 (Arduino Leonardo / Pro Micro)".
  Essas placas ficam em Documents\Arduino\hardware\LFH.
*/

#include <MIDIUSB.h>
#include <EEPROM.h>

/*********FIRMWARE*********/
const byte FW_VERSION = 1;

/*********SYSEX*********/
const byte SYSEX_ID = 0x7D;      // ID reservado para uso nao comercial
const byte SYSEX_DEVICE = 0x46;  // 'F' de Fader
const byte CMD_GET = 0x01;       // pede a configuracao
const byte CMD_CONFIG = 0x02;    // resposta com a configuracao
const byte CMD_SET = 0x03;       // grava uma nova configuracao
const byte CMD_RESET = 0x04;     // volta para a configuracao padrao
const byte CMD_ACK = 0x05;       // confirma que gravou

const byte SYSEX_MAX = 128;
byte sysexBuf[SYSEX_MAX];
byte sysexLen = 0;
bool sysexActive = false;


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
const byte MAX_FADERS = 12;
const byte FLAG_ZERO = 0x01;  // manda 0 quando o fader esta no fim do curso

struct Fader {
  byte pin;      // indice em analogPins
  byte channel;  // 0-15
  byte cc;       // 0-127
  byte minVal;   // valor MIDI no inicio do curso
  byte maxVal;   // valor MIDI no fim do curso
  byte flags;
};

const byte FADER_BYTES = 6;  // campos de Fader enviados por SysEx

struct Config {
  byte magic;
  byte version;
  byte numFaders;
  Fader faders[MAX_FADERS];
};

const byte CONFIG_MAGIC = 0x5A;
const byte CONFIG_VERSION = 1;

Config config;


/*********POTENTIOMETERS*********/
const int zeroZone = 10;   // leitura ate aqui conta como fim do curso
const int rawTop = 1020;   // leitura maxima considerada
const int varThreshold = 20;
const int potTimeout = 300;

int potCState[MAX_FADERS] = { 0 };   // estado atual da porta analogica
int potPState[MAX_FADERS] = { 0 };   // estado previo da porta analogica
byte midiState[MAX_FADERS] = { 0 };
byte midiPState[MAX_FADERS] = { 0 };
unsigned long lastPot[MAX_FADERS] = { 0 };


void setup() {

  loadConfig();
  delay(2000);
}

void loop() {

  readMidi();
  potentiometers();
}


void potentiometers() {

  for (byte i = 0; i < config.numFaders; i++) {
    Fader &f = config.faders[i];
    potCState[i] = analogRead(analogPins[f.pin]);

    if ((f.flags & FLAG_ZERO) && potCState[i] <= zeroZone) {
      midiState[i] = 0;
    }
    else {
      int from = (f.flags & FLAG_ZERO) ? zeroZone + 1 : 0;
      int lo = min(f.minVal, f.maxVal);
      int hi = max(f.minVal, f.maxVal);
      midiState[i] = constrain(map(potCState[i], from, rawTop, f.minVal, f.maxVal), lo, hi);
    }

    int potVar = abs(potCState[i] - potPState[i]);

    if (potVar > varThreshold) {
      lastPot[i] = millis();
    }

    if (millis() - lastPot[i] < (unsigned long)potTimeout) {
      if (midiState[i] != midiPState[i]) {

        controlChange(f.channel, f.cc, midiState[i]);
        MidiUSB.flush();

        midiPState[i] = midiState[i];
        potPState[i] = potCState[i];
      }
    }
  }
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, (byte)(0xB0 | channel), control, value };
  MidiUSB.sendMIDI(event);
}


/*********CONFIG / EEPROM*********/
void loadDefaults() {
  // igual ao projeto original: 4 faders em A3, A2, A1, A0 -> CC 1 a 4,
  // canal 1, faixa 42-127 e 0 no fim do curso
  config.magic = CONFIG_MAGIC;
  config.version = CONFIG_VERSION;
  config.numFaders = 4;

  for (byte i = 0; i < MAX_FADERS; i++) {
    Fader &f = config.faders[i];
    f.pin = (i < 4) ? 3 - i : i % NUM_PINS;
    f.channel = 0;
    f.cc = i + 1;
    f.minVal = (i < 4) ? 42 : 0;
    f.maxVal = 127;
    f.flags = (i < 4) ? FLAG_ZERO : 0;
  }
}

// corrige valores fora da faixa; retorna false se a config nao e utilizavel
bool sanitize(Config &c) {
  if (c.numFaders < 1 || c.numFaders > MAX_FADERS) return false;

  for (byte i = 0; i < MAX_FADERS; i++) {
    Fader &f = c.faders[i];
    if (f.pin >= NUM_PINS) f.pin = 0;
    f.channel &= 0x0F;
    f.cc &= 0x7F;
    f.minVal &= 0x7F;
    f.maxVal &= 0x7F;
    f.flags &= FLAG_ZERO;
  }
  return true;
}

void loadConfig() {
  EEPROM.get(0, config);
  if (config.magic != CONFIG_MAGIC || config.version != CONFIG_VERSION || !sanitize(config)) {
    loadDefaults();
  }
  resetFaderState();
}

void saveConfig() {
  EEPROM.put(0, config);  // put so grava os bytes que mudaram
}

void resetFaderState() {
  for (byte i = 0; i < MAX_FADERS; i++) {
    potPState[i] = 0;
    midiPState[i] = 0;
    lastPot[i] = 0;
  }
}


/*********SYSEX*********/
void readMidi() {
  midiEventPacket_t rx;

  while (true) {
    rx = MidiUSB.read();
    if (rx.header == 0) break;

    // o "code index" do pacote USB diz quantos bytes de SysEx ele traz
    byte cin = rx.header & 0x0F;
    byte n = 0;
    if (cin == 0x04 || cin == 0x07) n = 3;
    else if (cin == 0x06) n = 2;
    else if (cin == 0x05) n = 1;

    byte bytes[3] = { rx.byte1, rx.byte2, rx.byte3 };
    for (byte k = 0; k < n; k++) sysexByte(bytes[k]);
  }
}

void sysexByte(byte b) {
  if (b == 0xF0) {
    sysexActive = true;
    sysexLen = 0;
    return;
  }
  if (!sysexActive) return;

  if (b == 0xF7) {
    sysexActive = false;
    handleSysex(sysexBuf, sysexLen);
  }
  else if (sysexLen < SYSEX_MAX) {
    sysexBuf[sysexLen++] = b;
  }
  else {
    sysexActive = false;  // mensagem grande demais, descarta
  }
}

// data nao inclui F0 e F7
void handleSysex(const byte *data, byte len) {
  if (len < 3 || data[0] != SYSEX_ID || data[1] != SYSEX_DEVICE) return;

  byte cmd = data[2];

  if (cmd == CMD_GET) {
    sendConfig();
  }
  else if (cmd == CMD_SET) {
    if (len < 3 + 1 + MAX_FADERS * FADER_BYTES) return;

    Config c;
    c.magic = CONFIG_MAGIC;
    c.version = CONFIG_VERSION;
    c.numFaders = data[3];
    const byte *p = data + 4;
    for (byte i = 0; i < MAX_FADERS; i++) {
      Fader &f = c.faders[i];
      f.pin = *p++;
      f.channel = *p++;
      f.cc = *p++;
      f.minVal = *p++;
      f.maxVal = *p++;
      f.flags = *p++;
    }
    if (!sanitize(c)) return;

    config = c;
    saveConfig();
    resetFaderState();

    byte ack[] = { SYSEX_ID, SYSEX_DEVICE, CMD_ACK };
    sendSysex(ack, sizeof(ack));
    sendConfig();
  }
  else if (cmd == CMD_RESET) {
    loadDefaults();
    saveConfig();
    resetFaderState();

    byte ack[] = { SYSEX_ID, SYSEX_DEVICE, CMD_ACK };
    sendSysex(ack, sizeof(ack));
    sendConfig();
  }
}

void sendConfig() {
  byte msg[7 + MAX_FADERS * FADER_BYTES];
  byte n = 0;
  msg[n++] = SYSEX_ID;
  msg[n++] = SYSEX_DEVICE;
  msg[n++] = CMD_CONFIG;
  msg[n++] = FW_VERSION;
  msg[n++] = MAX_FADERS;
  msg[n++] = NUM_PINS;
  msg[n++] = config.numFaders;
  for (byte i = 0; i < MAX_FADERS; i++) {
    Fader &f = config.faders[i];
    msg[n++] = f.pin;
    msg[n++] = f.channel;
    msg[n++] = f.cc;
    msg[n++] = f.minVal;
    msg[n++] = f.maxVal;
    msg[n++] = f.flags;
  }
  sendSysex(msg, n);
}

// envia F0 + body + F7 quebrado em pacotes USB-MIDI de 3 bytes
void sendSysex(const byte *body, byte len) {
  byte total = len + 2;
  byte chunk[3] = { 0, 0, 0 };
  byte n = 0;

  for (byte i = 0; i < total; i++) {
    bool last = (i == total - 1);
    chunk[n++] = (i == 0) ? 0xF0 : last ? 0xF7 : body[i - 1];

    if (n == 3 || last) {
      midiEventPacket_t packet;
      packet.header = !last ? 0x04 : (n == 1 ? 0x05 : n == 2 ? 0x06 : 0x07);
      packet.byte1 = chunk[0];
      packet.byte2 = (n > 1) ? chunk[1] : 0;
      packet.byte3 = (n > 2) ? chunk[2] : 0;
      MidiUSB.sendMIDI(packet);
      n = 0;
    }
  }
  MidiUSB.flush();
}
