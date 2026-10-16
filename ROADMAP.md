# Ideias para o futuro

## Vários modelos de controlador

- **Modelo no firmware**: o firmware responde a um pedido de identificação com código do modelo, nome (hoje "Fader 4", nome temporário) e versão. O nome fica num texto do firmware, fácil de trocar. Configuradores antigos continuam funcionando.
- **Configurador por modelo**: uma página só, que conecta, lê o modelo e mostra a interface certa (faders no Fader 4, botões no controlador de botões). Conexão, salvar e presets compartilhados entre os modelos. Se o firmware não tiver identificação:
  - **firmware LFH antigo (v1/v2)**: só existia no Fader 4, então a página mostra "firmware desatualizado" e oferece atualizar para o firmware do Fader 4;
  - **firmware desconhecido**: mostra um erro e oferece gravar o firmware, com a pessoa escolhendo o modelo numa lista.
  
  Os botões de gravar dependem da gravação pelo navegador (abaixo); até lá, a mensagem orienta a gravar pela IDE do Arduino.

- **Renomear o repositório**: quando houver mais de um modelo, trocar `lfh-fader-4` por algo como `lfh-controllers`. O GitHub mantém histórico e releases e redireciona o link antigo; depois é só atualizar o endereço do `origin` na pasta local.

## Gravar firmware sem a IDE do Arduino

- **Pelo navegador**: uma página que grava o `.hex` direto na placa pelo Chrome/Edge (Web Serial). Nas placas ATmega32u4 a página coloca a placa em modo de gravação (reset a 1200 bps); o navegador pode pedir para escolher a porta duas vezes. Para virar um link, precisa de hospedagem (GitHub Pages é grátis só para repositório público).
- **Versão desktop**: aplicativo (Electron ou Tauri) que reaproveita o configurador e já inclui o gravador. Funciona sem Chrome e grava sem pedir porta, mas precisa de instalador, é maior e mostra aviso do Windows se não for assinado. Fazer depois da versão web, se ela não for suficiente.

## Configurador

- **CC inicial**: escolher o primeiro CC dos faders (ex.: 20 → CC 20, 21, 22, 23) em vez de sempre começar no CC 1.
