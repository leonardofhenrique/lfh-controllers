# LFH Controllers

Controladores MIDI USB feitos com Arduino (ATmega32u4), configuráveis pelo navegador, sem precisar abrir a IDE do Arduino nem mexer em código.

| Modelo | O que tem | Firmware |
|---|---|---|
| **LFH Fader 4** | 4 faders | `firmware/LFH_Fader_4` |
| **LFH Deck** | 4 faders + 21 botões com LED (3 camadas) | `firmware/LFH_Deck` |

- **Firmware**: grava uma vez na placa. A configuração fica salva na EEPROM.
- **Configurador** (`configurador/configurador.html`): uma página só para os dois modelos. Ela reconhece o controlador conectado e mostra a tela certa.
- **LFHCore** (`libraries/LFHCore`): código comum aos firmwares (SysEx e leitura dos faders).
- **Placas LFH** (`hardware/LFH`): definição de placa para a IDE do Arduino que faz o dispositivo aparecer como "LFH Fader 4" ou "LFH Deck" no computador.
- **Case** (`case/fader4`): modelo 3D impresso do Fader 4.
- **Documentação** (`docs/`): desenho e ligações do Deck.

## O que dá para configurar

Faders (nos dois modelos):

- Em qual entrada analógica cada fader está ligado, de três jeitos:
  - **Mexa para associar**: clique no fader na tela e mexa o fader físico
  - **Identificar entradas**: as entradas dos faders mexidos aparecem nos 4 quadrados; arraste uma até o fader na tela
  - **tocar e tocar** (funciona no celular): toque numa entrada e depois no fader
- Ordem dos faders na tela (arraste o ⠿)
- Apelido de cada fader (até 12 caracteres, gravado no controlador)
- Canal MIDI (um só para os 4 faders). O CC segue a posição do fader: o 1º manda CC 1, o 2º manda CC 2, e assim por diante
- Valores mínimo e máximo (alças arrastáveis no trilho)
- Inverter o sentido do fader
- Enviar 0 quando o fader chega ao fim do curso

Botões (só no Deck):

- Botões numerados pela posição no painel, linha por linha da esquerda para a direita: 1–5, 6–10, 11–15, 16–20 e o 21 no fim da 4ª linha
- 3 camadas. Cada camada tem um canal MIDI e uma cor. O botão 21 troca a camada (1 → 2 → 3 → 1) e o LED dele mostra a cor da camada atual
- Os botões 1 a 20 mandam Note On/Off com a nota igual ao número do botão, no canal da camada atual
- Os LEDs dos botões 1 a 20 são controlados pelo computador (veja abaixo)
- Nome de cada botão em cada camada (até 12 caracteres, gravado no controlador)
- Brilho dos LEDs (o firmware também limita a corrente dos LEDs a 400 mA, para não passar do que a USB entrega)

### LEDs do Deck: retorno do computador

Apertar um botão não muda o LED: quem acende e apaga é o programa, mandando de volta para o Deck um **Note On** no **canal da camada**, com a **nota = número do botão** (1 a 20) e a **velocity = cor**:

| Velocity | Cor | | Velocity | Cor |
|---|---|---|---|---|
| 0 | apagado | | 5 | verde-água |
| 1 | branco | | 6 | laranja |
| 2 | vermelho | | 7 | amarelo-limão |
| 3 | azul | | 8 | azul-claro |
| 4 | verde | | 9 | rosa |

Velocity acima de 9 e Note Off também apagam o LED. Cada camada guarda as próprias cores: ao trocar de camada com o botão 21, os LEDs mostram o que o computador mandou para aquela camada. As cores não ficam salvas quando o Deck é desligado.

Notas especiais (Note On com velocity maior que 0, em qualquer canal):

- **Nota 127**: apaga os LEDs dos botões 1 a 20. O LED 21 continua mostrando a camada, e as cores continuam guardadas.
- **Nota 126**: religa todos com as cores que estavam. Trocar de camada também religa.

## Placas compatíveis

Arduino Micro, Arduino Leonardo e Pro Micro (ATmega32u4).

## Como gravar o firmware

1. Copie a pasta `hardware/LFH` para `Documentos\Arduino\hardware\` (crie a pasta `hardware` se não existir).
2. Copie a pasta `libraries/LFHCore` para `Documentos\Arduino\libraries\`. Se o repositório estiver dentro de `Documentos\Arduino`, dá para criar um atalho que acompanha as mudanças, no PowerShell:
   ```powershell
   New-Item -ItemType Junction -Path "$HOME\Documents\Arduino\libraries\LFHCore" -Target "<pasta do repositório>\libraries\LFHCore"
   ```
3. Instale pelo gerenciador de bibliotecas: **MIDIUSB** (os dois modelos) e **FastLED** (só o Deck). Reabra a IDE do Arduino.
4. Abra `firmware/LFH_Fader_4/LFH_Fader_4.ino` ou `firmware/LFH_Deck/LFH_Deck.ino`.
5. Em **Tools → Board → LFH**, escolha o modelo e a placa, por exemplo **LFH Deck (Arduino Leonardo / Pro Micro)**.
6. Escolha a porta e clique em **Upload**.

Fader 4: ao atualizar da v1 para a v2, a configuração salva volta ao padrão (o formato da memória mudou).

## Como usar o configurador

1. Feche o DAW ou outros programas de música (no Windows só um programa por vez usa o controlador).
2. Abra `configurador/configurador.html` no **Google Chrome** ou no **Microsoft Edge**.
3. Na tela inicial, clique em **Conectar controlador** e permita o acesso a dispositivos MIDI. Os controladores LFH ligados aparecem como botões.
4. Clique no controlador que quer configurar. Para ir para outro, use **Trocar controlador** (ou a lista ao lado dele, quando há mais de um ligado).
5. Ajuste e clique em **Salvar no controlador**.

O configurador só funciona com o firmware v2 do Fader 4 (para o firmware v1, use o configurador da release v1.0.0) e com o firmware v1 do Deck.

## Protocolo

O configurador conversa com o firmware por SysEx: `F0 7D <dispositivo> <comando> ... F7`. `7D` é o ID reservado para uso não comercial; o dispositivo é `46` ('F') no Fader 4 e `44` ('D') no Deck. Cada controlador só responde ao próprio dispositivo, e é assim que o configurador descobre qual está conectado.

### Comandos comuns

| Comando | Byte | Dados |
|---|---|---|
| Pedir configuração | `01` | — |
| Configuração | `02` | versão, nº de faders (4), nº de entradas, e 18 bytes por fader (+ dados do Deck, abaixo) |
| Gravar configuração | `03` | 18 bytes por fader (+ dados do Deck, abaixo) |
| Restaurar padrão | `04` | — |
| Confirmação | `05` | — |
| Modo identificar | `06` | `1` liga, `0` desliga (desliga sozinho após 3 s sem renovação) |
| Leitura das entradas | `07` | um byte (0–127) por entrada analógica |

Cada fader usa 18 bytes: entrada, canal, CC, mínimo, máximo, flags (`01` = 0 no fim do curso, `02` = invertido) e 12 bytes de apelido em ASCII.

No modo identificar, o firmware liga o pull-up interno de todas as entradas dos faders para que as que estão soltas fiquem paradas, e para de enviar CC.

### Só no Deck

Depois dos faders, a **configuração** (`02`) traz: nº de botões (21), nº de camadas (3), camada atual, brilho (0–127) e 4 bytes por camada (canal, R, G, B, com cada cor de 0 a 127). **Gravar** (`03`) leva o brilho e os 4 bytes por camada, sem os três primeiros campos.

| Comando | Byte | Dados |
|---|---|---|
| Pedir nomes | `10` | página (0–5) |
| Nomes | `11` | página + 10 nomes de 12 bytes |
| Gravar nomes | `12` | página + 10 nomes de 12 bytes; responde com `05` |
| Camada atual | `13` | camada (0–2); o Deck manda quando o botão 21 é apertado |

Páginas de nomes: 0 = camada 1, botões 1–10; 1 = camada 1, botões 11–20; 2 = camada 2, botões 1–10; e assim por diante.

## Versões

As tags levam o modelo na frente, como `fader4-v2.2.0` e `deck-v1.0.0`. As tags antigas (`v1.0.0`, `v2.0.0`, `v2.1.0`) são do Fader 4.

## Licença

[CC BY-NC 4.0](LICENSE): pode usar, modificar e compartilhar dando crédito a LFH, mas não para fins comerciais.
