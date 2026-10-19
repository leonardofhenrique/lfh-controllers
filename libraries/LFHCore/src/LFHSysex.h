#pragma once

#include <Arduino.h>
#include <MIDIUSB.h>

// Mensagem: F0 7D <dispositivo> <comando> ... F7
const byte LFH_SYSEX_ID = 0x7D;   // ID reservado para uso nao comercial
const byte LFH_SYSEX_MAX = 128;   // bytes entre F0 e F7

// Dispositivos
const byte LFH_DEVICE_FADER4 = 0x46;  // 'F' de Fader
const byte LFH_DEVICE_DECK = 0x44;    // 'D' de Deck

// Comandos comuns a todos os modelos
const byte LFH_CMD_GET = 0x01;      // pede a configuracao
const byte LFH_CMD_CONFIG = 0x02;   // resposta com a configuracao
const byte LFH_CMD_SET = 0x03;      // grava uma nova configuracao
const byte LFH_CMD_RESET = 0x04;    // volta para a configuracao padrao
const byte LFH_CMD_ACK = 0x05;      // confirma que gravou
const byte LFH_CMD_MONITOR = 0x06;  // liga/desliga o modo identificar
const byte LFH_CMD_PINS = 0x07;     // leitura de todas as entradas (modo identificar)

class LFHSysex {
public:
  // cmd e os dados que vem depois dele (sem F0, ID, dispositivo e F7)
  typedef void (*Handler)(byte cmd, const byte *data, byte len);
  // pacotes USB-MIDI que nao sao SysEx (notas, CC...)
  typedef void (*PacketHandler)(const midiEventPacket_t &packet);

  LFHSysex(byte device, Handler handler, PacketHandler other = nullptr);

  // le tudo o que chegou pela USB; chamar a cada volta do loop
  void read();

  // envia F0 7D <dispositivo> <cmd> <data> F7; data pode ter ate 123 bytes
  void send(byte cmd, const byte *data = nullptr, byte len = 0);
  void ack() { send(LFH_CMD_ACK); }

private:
  void byteIn(byte b);

  byte device_;
  Handler handler_;
  PacketHandler other_;
  byte buf_[LFH_SYSEX_MAX];
  byte len_ = 0;
  bool active_ = false;
};
