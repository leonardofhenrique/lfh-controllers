#include "LFHFaders.h"

static const int zeroZone = 10;   // leitura ate aqui conta como fim do curso
static const int rawTop = 1020;   // leitura maxima considerada
static const int varThreshold = 20;
static const int potTimeout = 300;
static const unsigned long monitorTimeout = 3000;
static const unsigned long monitorInterval = 30;

void lfhFaderDefaults(Fader *faders, byte count) {
  for (byte i = 0; i < count; i++) {
    Fader &f = faders[i];
    f.pin = 3 - i;
    f.channel = 0;
    f.cc = i + 1;
    f.minVal = 42;
    f.maxVal = 127;
    f.flags = LFH_FADER_ZERO;
    memset(f.name, 0, LFH_NAME_LEN);
  }
}

void lfhSanitizeName(char *name, byte len) {
  bool ended = false;
  for (byte k = 0; k < len; k++) {
    if (name[k] < 32 || name[k] > 126) ended = true;
    if (ended) name[k] = 0;
  }
}

void lfhSanitizeFader(Fader &f, byte numPins) {
  if (f.pin >= numPins) f.pin = 0;
  f.channel &= 0x0F;
  f.cc &= 0x7F;
  f.minVal &= 0x7F;
  f.maxVal &= 0x7F;
  f.flags &= LFH_FADER_ZERO | LFH_FADER_INVERT;
  lfhSanitizeName(f.name, LFH_NAME_LEN);
}

byte *lfhEncodeFader(const Fader &f, byte *out) {
  *out++ = f.pin;
  *out++ = f.channel;
  *out++ = f.cc;
  *out++ = f.minVal;
  *out++ = f.maxVal;
  *out++ = f.flags;
  for (byte k = 0; k < LFH_NAME_LEN; k++) *out++ = f.name[k];
  return out;
}

const byte *lfhDecodeFader(Fader &f, const byte *in) {
  f.pin = *in++;
  f.channel = *in++;
  f.cc = *in++;
  f.minVal = *in++;
  f.maxVal = *in++;
  f.flags = *in++;
  for (byte k = 0; k < LFH_NAME_LEN; k++) f.name[k] = *in++;
  return in;
}


LFHFaders::LFHFaders(Fader *faders, byte count, const byte *pins, byte numPins)
  : faders_(faders), count_(min(count, LFH_MAX_FADERS)), pins_(pins), numPins_(numPins) {}

static void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, (byte)(0xB0 | channel), control, value };
  MidiUSB.sendMIDI(event);
}

void LFHFaders::update() {
  if (monitorOn_) return;

  for (byte i = 0; i < count_; i++) {
    Fader &f = faders_[i];
    int raw = analogRead(pins_[f.pin]);
    if (f.flags & LFH_FADER_INVERT) raw = 1023 - raw;

    byte midi;
    if ((f.flags & LFH_FADER_ZERO) && raw <= zeroZone) {
      midi = 0;
    }
    else {
      int from = (f.flags & LFH_FADER_ZERO) ? zeroZone + 1 : 0;
      int lo = min(f.minVal, f.maxVal);
      int hi = max(f.minVal, f.maxVal);
      midi = constrain(map(raw, from, rawTop, f.minVal, f.maxVal), lo, hi);
    }

    if (abs(raw - potPState_[i]) > varThreshold) {
      lastPot_[i] = millis();
    }

    if (millis() - lastPot_[i] < (unsigned long)potTimeout) {
      if (midi != midiPState_[i]) {
        controlChange(f.channel, f.cc, midi);
        MidiUSB.flush();

        midiPState_[i] = midi;
        potPState_[i] = raw;
      }
    }
  }
}

void LFHFaders::reset() {
  for (byte i = 0; i < count_; i++) {
    potPState_[i] = 0;
    midiPState_[i] = 0;
    lastPot_[i] = 0;
  }
}

void LFHFaders::setMonitor(bool on) {
  monitorKeepAlive_ = millis();
  if (on == monitorOn_) return;

  monitorOn_ = on;
  for (byte p = 0; p < numPins_; p++) {
    pinMode(pins_[p], on ? INPUT_PULLUP : INPUT);
  }
  if (!on) reset();
}

void LFHFaders::monitorTick(LFHSysex &sysex) {
  if (!monitorOn_) return;
  if (millis() - monitorKeepAlive_ > monitorTimeout) {
    setMonitor(false);
    return;
  }
  if (millis() - monitorSent_ < monitorInterval) return;
  monitorSent_ = millis();

  byte values[16];
  byte n = min(numPins_, (byte)sizeof(values));
  for (byte p = 0; p < n; p++) {
    values[p] = analogRead(pins_[p]) >> 3;  // 0-127
  }
  sysex.send(LFH_CMD_PINS, values, n);
}
