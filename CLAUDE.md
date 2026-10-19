# LFH Controllers

Linha de controladores MIDI USB (Arduino ATmega32u4), configuráveis pelo navegador via SysEx. Eles não controlam o computador diretamente: enviam MIDI (Control Change dos faders, notas dos botões), e o programa que recebe decide o que fazer.

Modelos: **LFH Fader 4** (4 faders) e **LFH Deck** (4 faders + 21 botões com LED). Tudo fica num repositório só. Ao mexer no código, o que é comum aos modelos vai para `libraries/LFHCore`; o que é de um modelo fica no firmware dele.

## Estrutura

- `firmware/LFH_Fader_4/LFH_Fader_4.ino`: firmware do Fader 4. Bibliotecas: MIDIUSB, LFHCore.
- `firmware/LFH_Deck/LFH_Deck.ino`: firmware do Deck. Bibliotecas: MIDIUSB, FastLED, LFHCore.
- `libraries/LFHCore/`: biblioteca Arduino com o código comum (`LFHSysex`: transporte SysEx; `LFHFaders`: leitura dos faders, CC e modo identificar). Na máquina de desenvolvimento, `Documentos\Arduino\libraries\LFHCore` é uma junção apontando para cá.
- `configurador/configurador.html`: página única que conecta por Web MIDI, descobre o modelo e mostra a tela certa (Chrome/Edge).
- `hardware/LFH/avr/`: definição das placas para a IDE do Arduino (`boards.txt`, `platform.txt`). Faz o USB aparecer como "LFH Fader 4" ou "LFH Deck".
- `case/fader4/`: case impresso do Fader 4 (CadQuery).
- `docs/lfh-deck-layout.svg`: painel e ligações do Deck.
- `build/` (fora do git): `fader4/` e `deck/` com `.hex`, configuradores congelados e notas de release. `build/deck/backup/` guarda a memória lida da placa do Deck antes do primeiro firmware LFH (sketch `arduino_midi_v5`).
- `ROADMAP.md`: ideias futuras.

## Arquitetura do firmware

`LFHCore`:

1. **LFHSysex**: `read()` lê a USB e entrega ao firmware só as mensagens do próprio dispositivo (`handler(cmd, data, len)`, sem o cabeçalho); `send(cmd, data, len)` monta `F0 7D <dispositivo> <cmd> ... F7`. Buffer de 128 bytes entre F0 e F7.
2. **LFHFaders**: `struct Fader` (formato da EEPROM), `lfhFaderDefaults`, `lfhSanitizeFader`, `lfhEncodeFader`/`lfhDecodeFader` (18 bytes) e a classe `LFHFaders` (`update`, `reset`, `setMonitor`, `monitorTick`). Recebe a lista de entradas analógicas que os faders podem usar.

Cada firmware tem a própria `struct Config` (com `CONFIG_MAGIC` e `CONFIG_VERSION`), `loadConfig`, `saveConfig`, `sanitize`, `loadDefaults` e o `handleSysex` dos comandos.

## Protocolo SysEx

Mensagem: `F0 7D <dispositivo> <comando> ... F7`. `7D` = ID de uso não comercial. Dispositivo: `46` ('F') = Fader 4, `44` ('D') = Deck. Cada firmware só responde ao próprio dispositivo; o configurador descobre o modelo perguntando com cada um.

| Cmd | Nome | Dados |
|---|---|---|
| 01 | Pedir configuração | — |
| 02 | Configuração | versão do firmware, nº de faders, nº de entradas, 18 bytes por fader (+ Deck) |
| 03 | Gravar configuração | 18 bytes por fader (+ Deck) |
| 04 | Restaurar padrão | — |
| 05 | Confirmação | — |
| 06 | Modo identificar | `1` liga, `0` desliga |
| 07 | Leitura das entradas | 1 byte (0–127) por entrada |
| 10 | Pedir nomes (Deck) | página 0–5 |
| 11 | Nomes (Deck) | página + 10 nomes de 12 bytes |
| 12 | Gravar nomes (Deck) | página + 10 nomes; responde `05` |
| 13 | Camada atual (Deck) | camada 0–2; enviado quando o botão 21 é apertado |

Fader (18 bytes): entrada, canal, CC, mínimo, máximo, flags (`01` = 0 no fim do curso, `02` = invertido), 12 bytes de apelido ASCII.

Deck, depois dos faders: no `02`, nº de botões (21), nº de camadas (3), camada atual, brilho, 4 bytes por camada (canal, R, G, B de 0 a 127); no `03`, brilho e 4 bytes por camada. Página de nomes `p` = camada `p / 2`, botões `(p % 2) * 10 + 1` a `+ 10`.

Todos os dados do SysEx têm de ficar entre 0 e 127 (bit mais alto zerado). Uma mensagem inteira (sem F0/F7) cabe em 128 bytes.

## Regras de negócio que não podem quebrar

- Um único canal MIDI para os 4 faders. O CC segue a **posição** do fader na tela: 1º = CC 1, 2º = CC 2, etc. Reordenar faders muda os CCs.
- Valores MIDI: 0–127. Leitura analógica: 0–1023; `zeroZone` = 10 (fim do curso), `rawTop` = 1020.
- Padrão de fábrica dos faders: A3, A2, A1, A0 → CC 1 a 4, canal 1, faixa 42–127, "0 no fim do curso".
- `sanitize()` deve ser aplicado a toda configuração recebida ou lida da EEPROM.
- Alterar o formato da `Config` de um modelo exige subir o `CONFIG_VERSION` dele. Hoje isso **apaga a configuração do usuário** (volta ao padrão). Se possível, implemente migração em vez de zerar. A `struct Fader` é usada pelos dois modelos.
- Firmware do Fader 4 v2 só conversa com configurador v2 ou mais novo. Mudança incompatível no protocolo = nova versão maior.

## LFH Deck

Painel e ligações em `docs/lfh-deck-layout.svg`.

- Botões numerados por coluna: 1–4 / 5–8 / 9–12 / 13–16 nas colunas 1–4 (linhas 1–4), 17–20 na 5ª coluna, 21 no fim da 4ª linha.
- Botões 1–16 no multiplexador CD74HC4067: SIG no pino 4 (= A6), S0–S3 nos pinos 9, 8, 7, 6, EN no GND. Canal do mux = coluna × 4 + linha (botão 1 = C0, 2 = C4, 5 = C1...).
- Botões 17–20 nos pinos 10, 16, 14, 15; botão 21 no pino 5. Todos com pull-up, apertado = LOW.
- Faders em A3, A2, A1, A0 (esquerda para a direita). Os faders do Deck só podem usar A0–A5 (A6–A11 estão ocupadas).
- LEDs WS2812 no pino 3, 21 em cadeia, linha por linha da esquerda para a direita (ordem testada na placa).
- 3 camadas, cada uma com canal e cor. Botões 1–20 mandam Note On (vel. 127) / Note On vel. 0 com nota = número do botão, no canal da camada atual. Botão 21 só troca a camada; o LED dele mostra a cor da camada. A camada não é salva: liga sempre na 1.
- LEDs 1–20: **só o computador controla** (apertar o botão não muda o LED; decisão do usuário). Note On recebido no canal de uma camada: nota = botão, velocity = índice da paleta (0–9, a mesma do `arduino_midi_v5`; acima de 9 ou Note Off apaga). Cores guardadas por camada, só na RAM. Nota 127 (vel. > 0, qualquer canal) apaga tudo, inclusive o LED 21, sem esquecer as cores; nota 126 ou trocar de camada religa.
- Nomes dos botões: 3 × 20 × 12 bytes direto na EEPROM a partir do endereço 128 (não ficam na RAM).
- Corrente: o firmware limita os LEDs a 400 mA (`FastLED.setMaxPowerInVoltsAndMilliamps`) e o brilho é configurável.

## Placas e gravação

- Placas: Arduino Micro, Leonardo e Pro Micro (ATmega32u4). VID/PID: Micro `0x2341/0x8037`, Leonardo `0x2341/0x8036`.
- O nome USB vem do `boards.txt` (`build.usb_product`), não do código.
- Fluxo: copiar `hardware/LFH` para `Documentos\Arduino\hardware\` e `libraries/LFHCore` para `Documentos\Arduino\libraries\` (ou junção), instalar MIDIUSB (e FastLED para o Deck), escolher a placa LFH, gravar.
- Compilar pela linha de comando: `arduino-cli compile --fqbn LFH:avr:lfh_deck_leonardo firmware/LFH_Deck` (o `arduino-cli` vem com a IDE 2, em `resources\app\lib\backend\resources\`).

## Versões e releases

- Tags com o modelo na frente: `fader4-vX.Y.Z`, `deck-vX.Y.Z`. As tags antigas `v1.0.0`, `v2.0.0`, `v2.1.0` são do Fader 4. Branches `main` e `develop`.
- Para cada release: `.hex` de cada placa em `build/<modelo>/`, configurador congelado (`configurador-vX.Y.Z.html`) e notas dizendo o que mudou **e se o firmware mudou**.
- Quando só o configurador muda, a nota deve dizer qual firmware continua valendo.

## Convenções

- Comentários e textos da interface em português.
- Licença CC BY-NC 4.0: não usar em contexto comercial sem autorização.
- Não commitar `.elf`/`.eep`/`.hex`; `build/` fica fora do git.
- Alterar o protocolo? Atualize aqui, no `README.md` e no configurador no mesmo commit.
