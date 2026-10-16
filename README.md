# LFH Fader 4

Controlador MIDI USB de 4 faders feito com Arduino (ATmega32u4), configurável pelo navegador, sem precisar abrir a IDE do Arduino nem mexer em código.

- **Firmware** (`firmware/MIDI_FADER_CONFIG`): grava uma vez na placa. A configuração fica salva na EEPROM.
- **Configurador** (`configurador/configurador.html`): página que se conecta ao controlador por MIDI e altera a configuração.
- **Placas LFH** (`hardware/LFH`): definição de placa para a IDE do Arduino que faz o dispositivo aparecer como "LFH Fader 4" no computador.

## O que dá para configurar

- Em qual entrada analógica cada fader está ligado, de três jeitos:
  - **Mexa para associar**: clique no fader na tela e mexa o fader físico
  - **Identificar entradas**: a entrada acende quando você mexe o fader; arraste ela até o fader na tela
  - lista **Entrada** de cada fader
- Ordem dos faders na tela (arraste o ⠿)
- Apelido de cada fader (até 12 caracteres, gravado no controlador)
- Canal MIDI e número de CC
- Valores mínimo e máximo (alças arrastáveis no trilho)
- Inverter o sentido do fader
- Enviar 0 quando o fader chega ao fim do curso

## Placas compatíveis

Arduino Micro, Arduino Leonardo e Pro Micro (ATmega32u4). Biblioteca necessária: **MIDIUSB**.

## Como gravar o firmware

1. Copie a pasta `hardware/LFH` para `Documentos\Arduino\hardware\` (crie a pasta `hardware` se não existir) e reabra a IDE do Arduino.
2. Instale a biblioteca **MIDIUSB** pelo gerenciador de bibliotecas.
3. Abra `firmware/MIDI_FADER_CONFIG/MIDI_FADER_CONFIG.ino`.
4. Em **Tools → Board → LFH**, escolha **LFH Fader 4 (Arduino Micro)** ou **LFH Fader 4 (Arduino Leonardo / Pro Micro)**.
5. Escolha a porta e clique em **Upload**.

Ao atualizar da v1 para a v2, a configuração salva volta ao padrão (o formato da memória mudou).

## Como usar o configurador

1. Feche o DAW ou outros programas de música (no Windows só um programa por vez usa o controlador).
2. Abra `configurador/configurador.html` no **Google Chrome** ou no **Microsoft Edge**.
3. Clique em **Conectar controlador** e permita o acesso a dispositivos MIDI.
4. Ajuste e clique em **Salvar no controlador**.

O configurador v2 só funciona com o firmware v2. Para o firmware v1, use o configurador da release v1.0.0.

## Protocolo (v2)

O configurador conversa com o firmware por SysEx: `F0 7D 46 <comando> ... F7` (`7D` é o ID reservado para uso não comercial).

| Comando | Byte | Dados |
|---|---|---|
| Pedir configuração | `01` | — |
| Configuração | `02` | versão, nº de faders (4), nº de entradas, e 18 bytes por fader |
| Gravar configuração | `03` | 18 bytes por fader |
| Restaurar padrão | `04` | — |
| Confirmação | `05` | — |
| Modo identificar | `06` | `1` liga, `0` desliga (desliga sozinho após 3 s sem renovação) |
| Leitura das entradas | `07` | um byte (0–127) por entrada analógica |

Cada fader usa 18 bytes: entrada, canal, CC, mínimo, máximo, flags (`01` = 0 no fim do curso, `02` = invertido) e 12 bytes de apelido em ASCII.

No modo identificar, o firmware liga o pull-up interno de todas as entradas para que as que estão soltas fiquem paradas, e para de enviar CC.

## Licença

[CC BY-NC 4.0](LICENSE): pode usar, modificar e compartilhar dando crédito a LFH, mas não para fins comerciais.
