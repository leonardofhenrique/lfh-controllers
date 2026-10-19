#pragma once

#include <Arduino.h>
#include "LFHSysex.h"

const byte LFH_NAME_LEN = 12;        // apelido: ate 12 caracteres ASCII
const byte LFH_FADER_ZERO = 0x01;    // manda 0 quando o fader esta no fim do curso
const byte LFH_FADER_INVERT = 0x02;  // inverte o sentido do fader
const byte LFH_MAX_FADERS = 8;

// Formato gravado na EEPROM: nao mude a ordem nem o tamanho dos campos
// sem subir o CONFIG_VERSION do firmware.
struct Fader {
  byte pin;      // indice na lista de entradas analogicas do firmware
  byte channel;  // 0-15
  byte cc;       // 0-127
  byte minVal;   // valor MIDI no inicio do curso
  byte maxVal;   // valor MIDI no fim do curso
  byte flags;
  char name[LFH_NAME_LEN];
};

const byte LFH_FADER_BYTES = 6 + LFH_NAME_LEN;  // campos de Fader enviados por SysEx

// padrao: A3, A2, A1, A0 -> CC 1 a 4, canal 1, faixa 42-127, 0 no fim do curso
void lfhFaderDefaults(Fader *faders, byte count);
// corrige valores fora da faixa
void lfhSanitizeFader(Fader &f, byte numPins);
// so ASCII visivel; depois do primeiro caractere invalido, zera
void lfhSanitizeName(char *name, byte len);
// 18 bytes por fader; devolvem o ponteiro para o proximo
byte *lfhEncodeFader(const Fader &f, byte *out);
const byte *lfhDecodeFader(Fader &f, const byte *in);

class LFHFaders {
public:
  // pins: entradas analogicas que os faders podem usar (indice 0 = pins[0])
  LFHFaders(Fader *faders, byte count, const byte *pins, byte numPins);

  // le os faders e manda CC; nao faz nada com o modo identificar ligado
  void update();
  // esquece as ultimas leituras (depois de mudar a configuracao)
  void reset();

  // Modo identificar: liga o pull-up em todas as entradas para que as soltas
  // fiquem paradas em 1023; so as que tem fader ligado variam quando mexidas.
  // Enquanto ligado, os faders nao mandam CC. Desliga sozinho se a pagina
  // parar de pedir (fechou ou desconectou).
  void setMonitor(bool on);
  bool monitorOn() const { return monitorOn_; }
  // manda a leitura de cada entrada (0-127) de tempos em tempos
  void monitorTick(LFHSysex &sysex);

  byte numPins() const { return numPins_; }

private:
  Fader *faders_;
  byte count_;
  const byte *pins_;
  byte numPins_;

  int potPState_[LFH_MAX_FADERS] = { 0 };   // leitura anterior da porta analogica
  byte midiPState_[LFH_MAX_FADERS] = { 0 }; // ultimo valor MIDI enviado
  unsigned long lastPot_[LFH_MAX_FADERS] = { 0 };

  bool monitorOn_ = false;
  unsigned long monitorKeepAlive_ = 0;
  unsigned long monitorSent_ = 0;
};
