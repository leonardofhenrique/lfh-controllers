# Ideias para o futuro

## LFH Deck: próximos passos

A v1 do firmware (`firmware/LFH_Deck`) e a tela do Deck no configurador já existem: faders iguais aos do Fader 4, 3 camadas (canal e cor de cada uma), botão 21 trocando a camada com a cor no LED dele, nome de cada botão por camada, e LEDs 1–20 controlados pelo computador (Note On com a velocity escolhendo a cor numa paleta de 10; notas 127/126 apagam e religam tudo). O hardware real está em `docs/lfh-deck-layout.svg`: um CD74HC4067 para os botões 1–16, botões 17–21 direto no Arduino.

Ideias que ficaram para depois:

- **Tipo de mensagem por botão**: Note, CC ou Program Change, com número configurável (hoje é sempre a nota igual ao número do botão).
- **Modo alternado** (liga/desliga) além do momentâneo.
- **Mais do retorno do computador**: paleta editável pelo configurador, retorno também por CC, e um comando SysEx `20` (botão, R, G, B) para cor exata.
- **Lembrar a camada** ao desligar (gravar na EEPROM só depois de alguns segundos sem mexer, por desgaste).
- **Migração de configuração**: subir `CONFIG_VERSION` hoje apaga a configuração salva. Implementar migração antes de mudar o formato da `Config` de novo.
- **Identificação de modelo** (`08`/`09`) com nome e versões. Hoje o configurador descobre o modelo pelo byte de dispositivo (`46` ou `44`), o que já resolve.
- Nome final do produto.

## Vários modelos de controlador

- **Modelo no firmware**: o firmware responde a um pedido de identificação com código do modelo, nome (hoje "Fader 4", nome temporário) e versão. O nome fica num texto do firmware, fácil de trocar. Configuradores antigos continuam funcionando.
- **Configurador por modelo**: uma página só, que conecta, lê o modelo e mostra a interface certa (faders no Fader 4, botões no controlador de botões). Conexão, salvar e presets compartilhados entre os modelos. Se o firmware não tiver identificação:
  - **firmware LFH antigo (v1/v2)**: só existia no Fader 4, então a página mostra "firmware desatualizado" e oferece atualizar para o firmware do Fader 4;
  - **firmware desconhecido**: mostra um erro e oferece gravar o firmware, com a pessoa escolhendo o modelo numa lista.
  
  Os botões de gravar dependem da gravação pelo navegador (abaixo); até lá, a mensagem orienta a gravar pela IDE do Arduino.


## Gravar firmware sem a IDE do Arduino

- **Pelo navegador**: uma página que grava o `.hex` direto na placa pelo Chrome/Edge (Web Serial). Nas placas ATmega32u4 a página coloca a placa em modo de gravação (reset a 1200 bps); o navegador pode pedir para escolher a porta duas vezes. Para virar um link, precisa de hospedagem (GitHub Pages é grátis só para repositório público).
- **Versão desktop**: aplicativo (Electron ou Tauri) que reaproveita o configurador e já inclui o gravador. Funciona sem Chrome e grava sem pedir porta, mas precisa de instalador, é maior e mostra aviso do Windows se não for assinado. Fazer depois da versão web, se ela não for suficiente.

## Configurador

- **CC inicial**: escolher o primeiro CC dos faders (ex.: 20 → CC 20, 21, 22, 23) em vez de sempre começar no CC 1.
