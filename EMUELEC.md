# Tank 1990 no console de TV (EmuELEC / GameStick Y6)

> 🇬🇧 Resumo em inglês no [README.en.md](README.en.md#-tv-console-emuelec--gamestick-y6)

O GameStick / Powkiddy **Y6** (e outros sticks e TV boxes com chip Amlogic) vem com o
**EmuELEC**, um Linux para jogos retrô. Além dos emuladores, o EmuELEC tem a seção **Ports**:
jogos nativos para Linux ARM, que rodam direto no stick, sem emulador. O Tank 1990 roda ali,
completo: campanha de 1 a 4 jogadores, duelo e sobrevivência.

## Por que um "port" e não uma ROM de NES

Uma ROM `.nes` não sai deste código. O NES tem uma CPU de 8 bits (6502) a 1,79 MHz e 2 KB de
RAM, e não roda C++ nem SDL2. Uma ROM seria outro jogo, escrito do zero em assembly ou cc65.
Já o EmuELEC é um Linux de 64 bits (aarch64) com SDL2. Por isso o jogo compila para ele e roda
nativo, com a mesma qualidade do PC.

## O que precisa

- Um aparelho com **EmuELEC 4.3 ou mais novo**, em **aarch64**. O Y6 tem um Amlogic S905X2 e
  vem com o EmuELEC 4.3.
- O cartão de memória e um PC com Linux e leitor de cartão, ou o stick na mesma rede do PC (ver
  [Levar para o stick](#levar-para-o-stick): só descompactar no cartão não basta).
- O pacote `Tank1990-emuelec`. Para conseguir, escolha uma das duas formas:
  - **Baixar pronto:** no GitHub, abra *Actions → build*, entre na execução mais recente
    (verde) e baixe o artefato **Tank1990-emuelec** no fim da página.
  - **Compilar:** num Linux ou no WSL, com o Docker instalado:

    ```bash
    make emuelec                # gera build/emuelec/Tank1990-emuelec.zip
    sh tools/emuelec/smoke.sh   # opcional: abre o binário num emulador de ARM (qemu-user)
    ```

O pacote tem duas pastas:

```
ports_scripts/Tank1990.sh     o atalho que aparece em "Ports"
ports_scripts/images/         a arte: imagem principal, capa, logo e o vídeo (Tank1990-*)
ports/tank1990/               o jogo: binário aarch64, mapas, imagem, fonte e sons, e a
                              entrada do menu (gamelist-entry.xml, ver abaixo)
```

Nenhum arquivo do pacote tem o nome de um arquivo de outro jogo: o atalho e a arte começam
com `Tank1990`, e o resto fica em `ports/tank1990/`. Por isso dá para descompactar por cima
de uma pasta de ROMs que já tem outros ports (o CharyRick, por exemplo) sem apagar nada.

## O que o menu do EmuELEC mostra

Com o jogo selecionado em **Ports**, antes de abrir, o EmuELEC mostra:

- o nome **Tank 1990 Remake**;
- a imagem principal: um campo de batalha com os 4 tanques dos jogadores, cada um na sua cor,
  e o logo **TANK 1990** em tijolos com a faixa dourada **REMAKE**;
- a capa (nos temas que mostram caixa) e o logo (nos temas que mostram o logo do jogo);
- um vídeo de 27 s, com som: as partidas de demonstração do menu (campanha, duelo 2 contra 2 e
  sobrevivência com 4 jogadores), todos os tanques na IA. Nos temas com vídeo, ele começa a
  tocar alguns segundos depois de o jogo ficar selecionado. O som do vídeo pode ser desligado
  no EmuELEC, em *Start → Sound Settings → Enable Video Audio*;
- a descrição: a campanha, os modos Duelo e Sobrevivência e os poderes novos;
- o criador (Gabryel Lima da Silva), o gênero (Action / Shooter), de 1 a 4 jogadores e a data
  em que o remake começou (julho de 2025).

Tudo isso vem do `ports_scripts/gamelist.xml` e das imagens em `ports_scripts/images/`. O que
aparece depende do tema do EmuELEC: alguns mostram a imagem principal, outros a capa ou o logo.

### Como a entrada chega ao `gamelist.xml`

O EmuELEC lê **um único** `gamelist.xml` por sistema, com a entrada de todos os ports. Se o
pacote trouxesse o seu próprio `ports_scripts/gamelist.xml`, descompactá-lo por cima apagaria
a descrição e a arte dos outros ports (e um port instalado depois apagaria a nossa). Um zip só
sabe substituir arquivos, não juntar o conteúdo deles.

Por isso a entrada do jogo vai dentro da pasta dele, em `ports/tank1990/gamelist-entry.xml`, e
o atalho `Tank1990.sh` a junta ao `gamelist.xml` toda vez que abre o jogo
(`ports/tank1990/gamelist-merge.sh`):

- só a entrada do Tank 1990 é trocada; as dos outros jogos ficam como estavam, e o arquivo é
  criado se ainda não existir;
- o que você marcou ou jogou (favorito, oculto, número de partidas, última vez, tempo de jogo)
  passa para a entrada nova;
- a fusão é feita pelo próprio EmulationStation, o menu do EmuELEC, pela API local dele
  (`127.0.0.1:1234/addgames/ports`, presente desde o EmuELEC 4.3). Editar o arquivo por fora
  não bastaria: com o menu aberto, ao sair ele regrava a entrada do jogo que acabou de rodar a
  partir do que tem na memória, e a descrição sumiria. Se a API não responder, o script edita
  o arquivo direto;
- não refaz nada se a entrada já estiver lá e for a mesma desta versão do pacote. Um
  `gamelist.xml` que não dá para entender fica como está (o motivo vai para
  `ports/tank1990/gamelist.log`).

O `make emuelec-card` e o `make emuelec-install` fazem a mesma fusão logo depois de copiar, e a entrada já está lá desde
o começo. Copiado pela pasta compartilhada, na primeira vez o jogo aparece na lista com o nome
do arquivo (**Tank1990**) e sem arte. Abra-o uma vez: ao fechar, o menu já mostra o nome, a
descrição e a arte.

Se você juntou o bloco `<game>` à mão numa versão antiga, não precisa fazer nada: a entrada é
trocada pela nova, sem duplicar.

A arte é gerada pelo `tools/emuelec/art/make_art.py` (Pillow), só com os sprites e a fonte
do jogo. Ajuste o script e rode-o de novo para mudar a arte.

O vídeo é gravado pelo `tools/emuelec/art/record_video.sh` (Linux, com Xvfb, xdotool e
ffmpeg). Ele roda o `tools/attract.cpp` (`make attract`): as partidas de `Demo::modes()`,
sem o menu por cima, sem escurecer e com som, cada modo por 9 s. O ffmpeg grava a imagem, e
o driver "disk" do SDL grava o som. Sai em H.264 + AAC, com 640 px de largura e cerca de
1,3 MB, um formato que o VLC do EmuELEC toca. Modo novo em `Demo::modes()` entra no vídeo
na próxima gravação.

## Créditos dentro do jogo

O menu principal tem a opção **Credits**: o remake (Gabryel Lima da Silva, Rio de Janeiro,
github.com/Gabryel-lima), a base do motor (Tanks, de Krystian Kałużny, 2015, MIT) e o jogo
original (Battle City, Namco, 1985). B/Back ou **Back** voltam ao menu.

## Levar para o stick

As duas pastas vão para a **raiz da pasta de ROMs** do EmuELEC, a que já tem `nes/`, `snes/`,
`psx/` etc. Se já existirem `ports/` e `ports_scripts/`, junte as pastas: não apague o que
já está lá.

**Não basta descompactar o pacote no cartão.** No EmuELEC 4.3 isso não funciona para o
`ports_scripts/`. No boot, o EmuELEC monta uma pasta dele por cima de `roms/ports_scripts`: um
overlay cuja camada de baixo são os ports que vêm com o sistema (`/usr/bin/ports`) e a de cima é
`.config/emuelec/ports`, na partição **STORAGE** do sistema. O que está no `ports_scripts/` da
partição das ROMs fica embaixo dessa montagem, escondido: os arquivos estão no cartão, mas o
EmuELEC nunca os lista. (O `ports/` não tem montagem por cima; só o atalho em `ports_scripts/`
some.) Algumas imagens de stick vêm com todos os ports do sistema removidos: a camada de cima
fica cheia de "whiteouts" do overlay, arquivos que no PC aparecem como dispositivos de
caractere com o nome de cada port (`2048.sh`, `Doom.sh`, ..., `gamelist.xml`).

Então o atalho precisa ir para a camada de cima do overlay, ou ser copiado com o stick ligado.
Escolha um dos três jeitos abaixo.

### Opção A: cartão num PC com Linux

A partição STORAGE é ext4, que o Linux lê e grava (o Windows não). Desligue o stick (*Start →
Quit → Shutdown*), ponha o cartão no PC e abra as partições dele no gerenciador de arquivos
(elas montam em `/media/<você>`: a das ROMs, a `STORAGE` e a de boot). Depois, no repositório:

```sh
make emuelec-card                       # ou: sh tools/emuelec/card-install.sh [pacote.zip] [/media/<você>]
```

Ele pede a sua senha (`sudo`), porque a STORAGE é do root. Ele copia:

- `ports/tank1990/` para a partição das ROMs;
- o atalho e as imagens para `STORAGE/.config/emuelec/ports/` (a camada de cima do overlay),
  trocando um whiteout de mesmo nome, se houver, e sem mexer nos arquivos dos outros ports.
  Copia também para o `ports_scripts/` da partição das ROMs, que é o que vale nas versões do
  EmuELEC sem o overlay (4.5 em diante);
- a entrada do menu para o `gamelist.xml` dos dois, sem mexer na dos outros ports.

`ZIP=...` escolhe outro pacote (o baixado do GitHub) e `CARD=...` outra pasta de montagem.
Ejete as partições, devolva o cartão ao stick e ligue: o jogo está em **Ports**.

Para os dois jeitos abaixo, o stick precisa estar na rede (Wi-Fi ou cabo, *Start → Network
Settings*); anote o IP dele.

### Opção B: script de instalação (SSH)

No Linux, macOS ou WSL, no repositório:

```sh
make emuelec-install HOST=192.168.0.42      # o IP do stick; ou: sh tools/emuelec/install.sh <IP> [pacote.zip]
```

Ele manda o pacote (por padrão o que o `make emuelec` gerou; `ZIP=...` escolhe outro, como o
baixado do GitHub) para `/storage/roms` e junta a entrada do menu ao `gamelist.xml` dos Ports
pela API do EmulationStation. O jogo aparece na hora, com nome e arte, sem reiniciar. O SSH
precisa estar ligado (*Start → Network Settings → Enable SSH*); a senha é `emuelec`, se você
não a trocou.

### Opção C: pasta compartilhada

1. No PC, abra a pasta compartilhada do stick:
   - no Windows, `\\EMUELEC\roms` (ou `\\<IP>\roms`) no Explorador de Arquivos;
   - no Linux ou macOS, `smb://<IP>/roms`.

   O compartilhamento costuma abrir sem senha. Se pedir uma, use a do sistema: usuário
   `root`, senha `emuelec` (o padrão, se você não a trocou).
2. Copie para dentro dela as pastas `ports_scripts` e `ports` do pacote.
3. Reinicie o EmulationStation (*Start → Quit → Restart EmulationStation*) ou o stick.

## Jogar

1. Abra o sistema **Ports** e escolha **Tank 1990 Remake**. Instalado pela pasta
   compartilhada, na primeira vez o nome que aparece é o do arquivo, **Tank1990**, sem arte: a
   entrada do menu entra quando o jogo abre (ver acima).
2. Os controles do stick funcionam como no PC (ver *Controles* no [README](README.md#-controles)):
   direcional ou analógico esquerdo para andar, qualquer botão frontal (A, B, X, Y) para atirar,
   LB para o poder guardado, Start para pausar. No menu, A/Start confirma e B/Back volta. Os
   dois controles do Y6 são os jogadores 1 e 2; com mais controles (USB), até 4 na campanha.
3. Para sair, use **Exit** no menu principal do jogo: o EmuELEC volta sozinho.

## Problemas

- **A arte e a descrição não aparecem.** Abra o jogo uma vez e feche: é aí que a entrada
  entra no `gamelist.xml`. Se ainda faltar, veja o que aconteceu em
  `ports/tank1990/gamelist.log`, e confira se a pasta `images/` está dentro de
  `ports_scripts/`, ao lado do `Tank1990.sh`.
- **"Ports" não aparece, ou o Tank1990 não está na lista, mas os arquivos estão no cartão.**
  O mais provável é que tenham sido copiados com o cartão no PC: o EmuELEC esconde esse
  `ports_scripts/` atrás de uma montagem dele (ver [Levar para o stick](#levar-para-o-stick)).
  Instale de novo com o `make emuelec-card` ou pela rede. Confira também o caminho: `ports_scripts/Tank1990.sh`, na raiz das
  ROMs, e não dentro de outra pasta.
- **Instalado com o `make emuelec-card` ou pela rede e ainda fora da lista.** Se a opção *Parse gamelists only* do
  EmulationStation estiver ligada (algumas imagens de stick vêm assim, para abrir mais rápido
  com milhares de jogos), o menu só mostra o que já está no `gamelist.xml` e nunca procura
  arquivo novo. Os dois scripts já põem a entrada no `gamelist.xml`; se ainda faltar, desligue a
  opção e reinicie o EmulationStation.
- **Abre e volta direto para o menu, ou fica tela preta.** O jogo grava o que aconteceu em
  `ports/tank1990/log.txt`. Abra esse arquivo no PC (ou pela rede) e veja a mensagem de erro.
  `Permission denied` (código de saída 126 no log do EmulationStation) quer dizer que a partição
  das ROMs, que é FAT, está montada sem permissão de executar programas, e o `chmod` não muda
  isso. O lançador já contorna: roda uma cópia do binário numa pasta que executa
  (`/storage/.tmp`, na partição STORAGE, que é ext4; depois `/tmp` e `/dev/shm`), com a pasta do
  jogo como pasta de trabalho. O `log.txt` registra cada pasta que falhou e o erro dela, e depois
  uma linha `launcher: <caminho>` com a que rodou. Se a última linha for `launcher: ./Tanks`, todas
  as cópias falharam. Os dados (mapas, sons, fonte, textura) ficam no cartão: o jogo entra na
  pasta que a variável `TANK_DATA_DIR` aponta (o lançador a define) em vez da pasta do executável.
  Sem ela, a cópia em `/tmp` rodava sem arquivo nenhum: tela preta e `Erro ao carregar som [...]:
  Mix_LoadWAV_RW with NULL src` no `log.txt`. `Argument list too long` no `mkdir`, no `cp` ou no jogo quer dizer que o
  ambiente ficou grande demais: o Linux limita uma variável de ambiente a 128 KiB, e o
  `gamecontrollerdb.txt` inteiro passa disso. O lançador exporta só os mapeamentos dos controles
  ligados (os GUIDs vêm no argumento `--controllers` do EmulationStation) e os de Linux, até 100000
  bytes.
- **O menu mostra nome, descrição e vídeo, mas na primeira vez não havia nada.** No firmware do
  Y6, o menu guarda a lista de cada sistema num `gamelist.db` e remove o `gamelist.xml` que
  encontra (na camada de cima do overlay ele aparece como um "whiteout", um dispositivo de
  caractere). A entrada entra quando o lançador roda o `gamelist-merge.sh`, que a entrega ao menu
  pela API local dele; por isso a arte aparece depois da primeira execução, mesmo que o jogo em si
  não tenha aberto.
- **O controle não responde.** O jogo usa os mapeamentos de controle que o EmuELEC já conhece
  (`/storage/.config/SDL-GameControllerDB/gamecontrollerdb.txt`). Cada controle reconhecido
  aparece como `Controle conectado: ...` no `log.txt`. Se o seu não aparecer, falta o mapeamento
  dele. Gere a linha num PC com o mesmo controle (o `padprobe` mostra o GUID, ver CONTROLES.md;
  ou use o [SDL2 Gamepad Tool](https://generalarcade.com/gamepadtool/)) e acrescente-a ao
  arquivo.
- **Sem som.** Confira o volume em *Start → Sound Settings* do EmuELEC: o jogo usa a saída de
  áudio do sistema.

## Como a build funciona (para quem mexe no código)

- `tools/emuelec/build.sh` baixa os fontes das bibliotecas, conferindo o sha256 de cada um. A
  compilação roda num container `dockcross/linux-arm64-lts`, fixado por digest e sem rede.
  `inside.sh` compila as bibliotecas (`deps.sh`), depois o jogo com o mesmo `Makefile` do PC,
  e monta o pacote.
- **glibc 2.27:** o binário não pode pedir uma glibc mais nova que a do stick (a do
  EmuELEC 4.3 é a 2.29).
- **SDL 2.0.9:** é a versão que o EmuELEC 4.3 traz, presa com patches para a GPU Mali. O jogo
  usa o `libSDL2` do próprio stick e é linkado contra um SDL 2.0.9 que não vai no pacote. Com
  `-Wl,--no-undefined`, uma função mais nova que a 2.0.9 quebra a build aqui, e não no stick.
  Código que usa algo mais novo fica atrás de `SDL_VERSION_ATLEAST`, como o `NetPad`.
- **SDL2_image, SDL2_mixer e SDL2_ttf** entram estáticos, com decodificadores embutidos
  (stb_image, stb_vorbis, FreeType). `libstdc++` e `libgcc` também vão dentro do binário. O
  binário só depende do `libSDL2-2.0.so.0` e da glibc do stick.
- **Tela cheia:** o `Tank1990.sh` liga `TANK_FULLSCREEN=1`, e a janela abre em tela cheia.
  Como no PC, o `Renderer::setScale` amplia e centraliza as telas lógicas (464x416) na TV.
- **`tools/emuelec/smoke.sh`** abre o binário no `qemu-aarch64`, com vídeo e áudio de mentira,
  e falha se faltar fonte, textura, som ou mapa no pacote, ou um arquivo que a entrada do
  gamelist cita (o atalho e as imagens), ou se o pacote trouxer um `ports_scripts/gamelist.xml`.
- **`make gamelist-test`** (`tools/emuelec/gamelist-test.sh`) confere a fusão sem o EmuELEC:
  a entrada entra sem apagar os outros ports, sem duplicar, guardando favorito e partidas, e
  pela API do EmulationStation (imitada com um servidor em Python). Roda também o `install.sh`
  com um `ssh` de mentira e o `card-install.sh` com um cartão de mentira (com um whiteout do
  overlay, quando o `mknod` é permitido): o pacote cai onde deve e a entrada é juntada sem mexer
  nos outros ports. A CI roda esse teste e publica o pacote. O teste não cobre a
  GPU e os controles de verdade: isso só se confere no stick.
