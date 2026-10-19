#include "LFHSysex.h"

LFHSysex::LFHSysex(byte device, Handler handler, PacketHandler other)
  : device_(device), handler_(handler), other_(other) {}

void LFHSysex::read() {
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

    if (n == 0) {
      if (other_) other_(rx);
      continue;
    }

    byte bytes[3] = { rx.byte1, rx.byte2, rx.byte3 };
    for (byte k = 0; k < n; k++) byteIn(bytes[k]);
  }
}

void LFHSysex::byteIn(byte b) {
  if (b == 0xF0) {
    active_ = true;
    len_ = 0;
    return;
  }
  if (!active_) return;

  if (b == 0xF7) {
    active_ = false;
    if (len_ >= 3 && buf_[0] == LFH_SYSEX_ID && buf_[1] == device_) {
      handler_(buf_[2], buf_ + 3, len_ - 3);
    }
  }
  else if (len_ < LFH_SYSEX_MAX) {
    buf_[len_++] = b;
  }
  else {
    active_ = false;  // mensagem grande demais, descarta
  }
}

// envia F0 + cabecalho + data + F7 quebrado em pacotes USB-MIDI de 3 bytes
void LFHSysex::send(byte cmd, const byte *data, byte len) {
  byte total = len + 5;  // F0 7D <dispositivo> <cmd> ... F7
  byte chunk[3] = { 0, 0, 0 };
  byte n = 0;

  for (byte i = 0; i < total; i++) {
    bool last = (i == total - 1);
    byte b;
    if (i == 0) b = 0xF0;
    else if (last) b = 0xF7;
    else if (i == 1) b = LFH_SYSEX_ID;
    else if (i == 2) b = device_;
    else if (i == 3) b = cmd;
    else b = data[i - 4];
    chunk[n++] = b;

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
