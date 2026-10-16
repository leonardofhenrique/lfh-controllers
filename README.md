# LFH Fader 4

Controlador MIDI USB de faders feito com Arduino (ATmega32u4), configurável pelo navegador, sem precisar abrir a IDE do Arduino nem mexer em código.

- **Firmware** (`firmware/MIDI_FADER_CONFIG`): grava uma vez na placa. A configuração fica salva na EEPROM.
- **Configurador** (`configurador/configurador.html`): página que se conecta ao controlador por MIDI e altera a configuração.
- **Placas LFH** (`hardware/LFH`): definição de placa para a IDE do Arduino que faz o dispositivo aparecer como "LFH Fader 4" no computador.

## O que dá para configurar

- Quantidade de faders (até 12) e em qual entrada analógica cada um está ligado
- Canal MIDI e número de CC de cada fader
- Valores mínimo e máximo (mínimo maior que o máximo inverte o fader)
- Enviar 0 quando o fader chega ao fim do curso

## Placas compatíveis

Arduino Micro, Arduino Leonardo e Pro Micro (ATmega32u4). Biblioteca necessária: **MIDIUSB**.

## Como gravar o firmware

1. Copie a pasta `hardware/LFH` para `Documentos\Arduino\hardware\` (crie a pasta `hardware` se não existir) e reabra a IDE do Arduino.
2. Instale a biblioteca **MIDIUSB** pelo gerenciador de bibliotecas.
3. Abra `firmware/MIDI_FADER_CONFIG/MIDI_FADER_CONFIG.ino`.
4. Em **Tools → Board → LFH**, escolha **LFH Fader 4 (Arduino Micro)** ou **LFH Fader 4 (Arduino Leonardo / Pro Micro)**.
5. Escolha a porta e clique em **Upload**.

## Como usar o configurador

1. Feche o DAW ou outros programas de música (no Windows só um programa por vez usa o controlador).
2. Abra `configurador/configurador.html` no **Google Chrome** ou no **Microsoft Edge**.
3. Clique em **Conectar controlador** e permita o acesso a dispositivos MIDI.
4. Ajuste e clique em **Salvar no controlador**.

## Protocolo

O configurador conversa com o firmware por SysEx: `F0 7D 46 <comando> ... F7` (`7D` é o ID reservado para uso não comercial).

| Comando | Byte | Dados |
|---|---|---|
| Pedir configuração | `01` | — |
| Configuração | `02` | versão, máx. faders, nº de entradas, nº de faders, e 6 bytes por fader (entrada, canal, CC, mín., máx., flags) |
| Gravar configuração | `03` | nº de faders e 6 bytes por fader |
| Restaurar padrão | `04` | — |
| Confirmação | `05` | — |

## Licença

[CC BY-NC 4.0](LICENSE): pode usar, modificar e compartilhar dando crédito a LFH, mas não para fins comerciais.
