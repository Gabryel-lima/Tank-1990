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
- O cartão de memória do aparelho, e um PC com leitor de cartão. Ou o stick na mesma rede do
  PC (veja a opção B abaixo).
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
ports/tank1990/               o jogo: binário aarch64, mapas, imagem, fonte e sons
```

## Levar para o cartão de memória

As duas pastas vão para a **raiz da pasta de ROMs** do EmuELEC, a que já tem `nes/`, `snes/`,
`psx/` etc. Se já existirem `ports/` e `ports_scripts/`, junte as pastas: não apague o que
já está lá.

### Opção A: cartão no PC

1. Desligue o stick pelo menu (*Start → Quit → Shutdown*) e tire o cartão.
2. Coloque o cartão no PC. Abra a partição **EEROMS**, a que tem as pastas dos consoles.
   (As outras partições, `COREELEC`/`EMUELEC` e `STORAGE`, são do sistema: não mexa.)
3. Descompacte o pacote ali, de modo que fiquem `EEROMS/ports_scripts/Tank1990.sh` e
   `EEROMS/ports/tank1990/Tanks`.
4. Ejete o cartão com segurança, devolva-o ao stick e ligue.

### Opção B: pela rede (stick no Wi-Fi)

1. No stick, conecte ao Wi-Fi (*Start → Network Settings*) e anote o IP.
2. No PC, abra a pasta compartilhada do stick:
   - no Windows, `\\EMUELEC\roms` (ou `\\<IP>\roms`) no Explorador de Arquivos;
   - no Linux ou macOS, `smb://<IP>/roms`.

   O compartilhamento costuma abrir sem senha. Se pedir uma, use a do sistema: usuário
   `root`, senha `emuelec` (o padrão, se você não a trocou).
3. Copie para dentro dela as pastas `ports_scripts` e `ports` do pacote.

## Jogar

1. No EmuELEC, atualize a lista de jogos: *Start → Game Settings → Update Gamelists*, ou
   reinicie o aparelho.
2. Abra o sistema **Ports** e escolha **Tank1990**.
3. Os controles do stick funcionam como no PC (ver *Controles* no [README](README.md#-controles)):
   direcional ou analógico esquerdo para andar, qualquer botão frontal (A, B, X, Y) para atirar,
   LB para o poder guardado, Start para pausar. No menu, A/Start confirma e B/Back volta. Os
   dois controles do Y6 são os jogadores 1 e 2; com mais controles (USB), até 4 na campanha.
4. Para sair, use **Exit** no menu principal do jogo: o EmuELEC volta sozinho.

## Problemas

- **"Ports" não aparece, ou o Tank1990 não está na lista.** Confira o caminho:
  `ports_scripts/Tank1990.sh`, na raiz das ROMs, e não dentro de outra pasta. Algumas versões
  de firmware dos sticks listam os ports em `ports/` em vez de `ports_scripts/`: nesse caso
  copie também o `Tank1990.sh` para `ports/`. Depois disso, *Update Gamelists*.
- **Abre e volta direto para o menu, ou fica tela preta.** O jogo grava o que aconteceu em
  `ports/tank1990/log.txt`. Abra esse arquivo no PC (ou pela rede) e veja a mensagem de erro.
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
  e falha se faltar fonte, textura, som ou mapa no pacote. A CI roda esse teste e publica o
  pacote. O teste não cobre a GPU e os controles de verdade: isso só se confere no stick.
