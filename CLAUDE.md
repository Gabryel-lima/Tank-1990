# Tank 1990

Clone de Battle City em C++17 com SDL2. Campanha de 1 a 4 jogadores e modos extras
(Duelo, Sobrevivência). Detalhes de uso no `README.md`.

## Antes de mexer em qualquer modo extra

Leia e siga **[MODOS_EXTRAS.md](MODOS_EXTRAS.md)**: as regras comuns a todos os modos extras
(jogadores, menu, mapas, cores, poderes, sinais visuais, fogo amigo, código) e o checklist de
um modo novo. Um modo novo segue todas as regras; uma exceção precisa de motivo e entra na
tabela de diferenças intencionais do fim do arquivo. Quando aparecer um padrão novo, repetido
em mais de um modo, ele vira regra lá e sobe para um ponto comum no código.

## Convenções

- Comentários e mensagens de commit em português; textos na tela do jogo em inglês.
- Números ajustáveis no `AppConfig` (`duel_*`, `survival_*`, `power_*`), comentados.
- A campanha não muda: os modos extras usam ganchos virtuais do `Game` cujo padrão é o
  comportamento da campanha.
- README em português (`README.md`) e em inglês (`README.en.md`), sempre os dois.
- Fins de linha no `.gitattributes`: LF nos `.sh` e no `Makefile`, CRLF nos `.cmd`. Script
  que o Windows manda rodar no WSL roda numa cópia sem `\r` (como o `install.cmd`), para
  funcionar também num clone antigo com CRLF.

## Verificar

```bash
make build        # compila (sem warnings)
make check-maps   # valida os mapas de todos os modos extras
make duel-sim     # simulação do duelo sem janela (equilíbrio e partidas travadas)
make pad-selftest # ponte de controles de ponta a ponta (controle de mentira)
make survival-test # regras da sobrevivência que já quebraram (bônus, contagem, compras)
```

Console de TV (EmuELEC, ver `EMUELEC.md`): `make emuelec` gera o pacote aarch64 (precisa de
Docker) e `sh tools/emuelec/smoke.sh` o abre no qemu. O pacote não traz `ports_scripts/gamelist.xml`
(apagaria o dos outros ports): a entrada vai em `ports/tank1990/gamelist-entry.xml` e o atalho a
junta com o `gamelist-merge.sh`, igual ao do CharyRick (`make gamelist-test`). Para o stick:
`make emuelec-card` (cartão no leitor do PC, Linux) ou `make emuelec-install HOST=<IP>` (SSH);
descompactar o `ports_scripts/` na partição das ROMs não serve no EmuELEC 4.3 (no boot ele monta um
overlay por cima dessa pasta, cuja camada de cima é `STORAGE/.config/emuelec/ports`, onde o
`card-install.sh` grava, e o atalho some). O pacote linka contra o SDL 2.0.9 do
EmuELEC 4.3: função do SDL mais nova que isso fica atrás de `SDL_VERSION_ATLEAST`, como no
`NetPad`, senão a build do pacote quebra. A arte e o vídeo do menu do EmuELEC saem de
`tools/emuelec/art/make_art.py` e `record_video.sh` (que grava as partidas de `Demo::modes()`
pelo `make attract`): refaça quando a cara do jogo mudar.

Bug corrigido num modo vira caso no teste dele (`tools/survival_test.cpp`), para não voltar.

A CI (`.github/workflows/build.yml`) roda isso no Linux, no macOS, no Windows (MSYS2) e o
`tools/wsl-setup.sh` num Alpine, como o `install.cmd`.

Poder novo com duração (tempo ou munição), em qualquer modo: avisa que está acabando com a
névoa branca (`Object::drawHaze`), no mapa e no espaço de poder do painel enquanto dura
(`Player::activePower`); regra W8 do `MODOS_EXTRAS.md`. Os poderes que já existem sem ela só
mudam quando pedido.

Poder colocado à frente (barricada, torreta...): logo à frente do tanque, na direção dele, nunca
com um bloco vazio inteiro no meio; use `Game::frontArea` (regra W9 do `MODOS_EXTRAS.md`).

Controles: o jogo só conhece o gamepad padronizado do SDL; o transporte (USB, Bluetooth, ponte)
fica abaixo (ver `CONTROLES.md`). Fonte nova de controles entra como joystick virtual do SDL,
como o `NetPad`, sem mudar o `Controllers` nem o jogo.

Modo novo entra na demonstração do fundo do menu: uma linha em `Demo::modes()`, com os
jogadores no computador (regra M6 do `MODOS_EXTRAS.md`).

Telas: desenhe sempre em coordenadas lógicas (`map_rect` + `status_rect`, 464x416); o
`Renderer::setScale` amplia e centraliza na janela. `AppConfig::windows_rect` é a janela real em
pixels (muda ao redimensionar) e não serve para posicionar nem para medir o que cabe na tela.
Confira uma tela nova também com a janela redimensionada (larga, alta, maximizada).

Mudança no `Game`, no `Player`, no `Tank`, no `Object` ou no `Renderer`: confirme que as
telas da campanha continuam idênticas pixel a pixel.
