/*
  LFHCore - codigo comum dos controladores LFH

  - LFHSysex: recebe e envia as mensagens F0 7D <dispositivo> <comando> ... F7
  - LFHFaders: le os faders, manda CC e cuida do modo identificar

  Cada firmware (Fader 4, Deck) guarda a propria configuracao na EEPROM e
  decide o que fazer com cada comando.
*/
#pragma once

#include "LFHSysex.h"
#include "LFHFaders.h"
