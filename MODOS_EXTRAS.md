# Regras dos modos extras

Regras que valem para **todo** modo em Extra Modes: os que existem (Duelo e Sobrevivência)
e os próximos. Cada regra saiu de algo que os dois modos já faziam do mesmo jeito; quando um
modo novo precisar quebrar uma delas, a exceção entra na tabela do fim, com o motivo.

Sempre que possível a regra está **no código**, num ponto comum (o `Game`, o `Player`, o
`Object`, o `Menu`, o `Powers`): um modo novo herda o comportamento em vez de copiá-lo. Código
copiado entre modos acaba divergindo; antes desta revisão, o duelo e a sobrevivência já
tratavam a pausa e o painel de poderes de jeitos diferentes.

## 1. Jogadores

| # | Regra | Onde |
|---|-------|------|
| J1 | Modo **não competitivo** (cooperativo, contra a IA): de **1 a 4 jogadores**, começando em 1. Dá para jogar sozinho. | `Menu` (`s_survival_players`) |
| J2 | Modo **competitivo**: de 2 a 4 jogadores, em equipes (1 contra 1, 2 contra 1, 2 contra 2, 3 contra 1...). Não precisa de opção para 1 jogador. | `DuelConfig` |
| J3 | A tela de configuração mostra o **dispositivo de cada jogador** (`P1 PAD 1`, `P2 WASD`); quem ficou sem nenhum aparece em vermelho como `NO PAD`, o título vira "Connect N pads" e o **Next fica bloqueado**. | `Controllers::inputName`, `Controllers::playersWithoutInput`, `Menu::playersOnScreen` |
| J4 | A distribuição de dispositivos é a do jogo inteiro: controles primeiro, depois `WASD` e `ARROWS`. | `Controllers::assign` |

## 2. Menu e controles

| # | Regra | Onde |
|---|-------|------|
| M1 | Fluxo: Extra Modes → configuração → **Next** → escolha de mapa (com miniatura e **Random**) → partida. | `Menu` |
| M2 | No fim da partida, volta para a **escolha de mapa com o último selecionado**: jogar de novo é um botão só. | `nextState()` do modo |
| M3 | Teclas: **Esc / Back** sai; **Enter / Start** pausa; na tela final, **tiro / Enter / A / Start** volta ao menu, liberado depois de 1,5 s (para não pular a tela sem querer). | `Game::extraModeInput` |
| M4 | A pausa é a **caixa padrão** ("PAUSE" e "ENTER / START"). O modo decide quando dá para pausar: sempre que os jogadores controlam os tanques. | `Game::drawPauseBox`, `drawPause()` |
| M5 | **Botão de poder:** LB no controle, Shift esquerdo no `WASD`, Shift direito no `ARROWS`. | `PlayerKeys::power`, `Player::takePowerPress` |

## 3. Mapas

| # | Regra | Onde |
|---|-------|------|
| P1 | Pasta própria `resources/<modo>_levels/`, com `maps.txt` (`arquivo;Nome`, um por linha) e mapas de 26×26 com os símbolos das fases. A pasta entra em `APP_RESOURCES` no `Makefile`. | `Makefile`, `AppConfig::<modo>_maps` |
| P2 | Uma classe `<Modo>Layout` valida cada mapa e diz o motivo (com linha e coluna). Mapa inválido é **recusado ao iniciar**, com o motivo no terminal, em vez de quebrar uma partida. | `DuelLayout`, `SurvivalLayout` |
| P3 | `make check-maps` verifica as pastas de todos os modos. | `tools/check_duel_maps.cpp` |
| P4 | Pontos de nascimento e os lugares que importam (base, bônus, surgimento de inimigos) ligados por caminho da **largura de um tanque** (2 tiles). | `<Modo>Layout` |

## 4. Cores

| # | Regra | Onde |
|---|-------|------|
| C1 | A cor do tanque diz **de que lado** o jogador está. Competitivo: a cor é da **equipe** (cores diferentes só entre adversários). Cooperativo: cada jogador tem a sua (P1 amarelo, P2 verde, P3 azul, P4 vermelho). | `Player::getPlayerColor`, `Duel::teamColor` |
| C2 | Bônus: no cooperativo, **todos cinza** (uma equipe só, qualquer um pega). Cor de equipe nos bônus é coisa de modo competitivo. | `Bonus::owner_team` |
| C3 | Estrutura **com dono** (competitivo) fica na **cor clareada do dono**. Cor numa estrutura quer dizer uma coisa só: o adversário não a derruba (o dono derruba). | `Duel::stoneTint` |

## 5. Poderes

| # | Regra | Onde |
|---|-------|------|
| W1 | Uma classificação só: o que depende de **onde e quando** é usado é **guardável** (mina, barricada, torreta, retorno, turbo); o resto é imediato. | `Powers::storable` |
| W2 | Cada modo tem a **sua tabela de sorteio**, sem poder que não faça nada nele (a trégua não entra no duelo, onde nada surge). | `Powers::duelTable`, `Powers::survivalTable` |
| W3 | Guardando um poder, o jogador **não pega outro bônus** (os bônus continuam no mapa). **Morrer perde** o poder guardado. | `Player::held_power`, `Player::destroy` |
| W4 | O painel mostra o **espaço do poder de cada jogador**: o ícone, ou uma moldura vazia. | `Game::drawPowerSlot` |
| W5 | Barricada e torreta só vão em área livre: dentro do mapa, sem cenário, **arbusto**, base, tanque ou torreta, e fora dos blocos que o modo reserva (pontos de nascimento). | `Game::areaFree`, `reservedTile()` do modo |
| W6 | Torreta e mina acabam sozinhas (tempo; a torreta também por munição). | `AppConfig::power_*` |
| W7 | Competitivo: as proteções do jogador (**escudo, barco**) valem contra os poderes do adversário. Contra os inimigos da IA, o poder pode ser total. | `Duel::hitTank`, `Game::killEnemy` |

## 6. Sinais visuais

| # | Regra | Onde |
|---|-------|------|
| V1 | **Algo acabando** (tempo ou munição): **névoa branca** pulsando sobre o sprite, a partir do último quarto, mais rápida e mais densa perto do fim. | `Object::drawHaze` |
| V2 | **Estado especial que ameaça o adversário** (o tiro demolidor): brilho branco curto (120 ms a cada 1 s) no tanque, ícone no painel, e **o que está ameaçado pisca no mesmo relógio**. | `Player::demolisherGlint`, `Duel::draw` |
| V3 | Mensagens no meio do mapa vão numa **caixa** (`drawMessageBox`), nunca em texto solto: texto direto sobre gelo ou pedra fica ilegível. Texto que precisa ficar sobre o mapa leva contorno. | `message_box.h`, `Renderer::drawTextOutlined` |
| V4 | Início da partida (ou da onda): caixa com o nome do modo ou da rodada, o **nome do mapa** e o objetivo. Fim: caixa com o resultado e os pontos de cada jogador na cor dele. | `drawOverlay()` do modo |
| V5 | Painel lateral: texto preto sobre bloco colorido (o texto colorido direto sobre o cinza do painel some). | `drawStatus()` do modo |

## 7. Regras de jogo

| # | Regra | Onde |
|---|-------|------|
| G1 | **Fogo amigo:** tiro não fere companheiro; o tiro de um **jogador** fere a **própria base** e derruba a **própria estrutura**, como no original. Tiro de bot aliado e de torreta não. | `Duel::bulletCanDamage`, `Duel::breaksBlock` |
| G2 | A IA **nunca atira** na direção da própria base ou da própria muralha. | `Duel::firesAtOwnBase` |
| G3 | Modo com IA não pode travar: se a rodada entrar num impasse, alguém sai do lugar (a simulação acusa partidas "travadas"). | `Duel::roleOf`, `tools/duel_sim.cpp` |

## 8. Código

| # | Regra |
|---|-------|
| K1 | **A campanha não muda.** O modo extra muda o jogo por **ganchos virtuais do `Game`** cujo padrão é o comportamento da campanha. As capturas de tela da campanha precisam continuar **idênticas pixel a pixel**. |
| K2 | Comportamento repetido em dois modos sobe para um ponto comum (`Game`, `Player`, `Object`, `Menu`), **não é copiado**. |
| K3 | Todo número ajustável fica no `AppConfig`, com o prefixo do modo (`duel_*`, `survival_*`) ou `power_*` quando é dos poderes, com um comentário explicando o efeito. |
| K4 | Comentários em português; o README (pt e en) descreve o modo: regras, mapas, controles e os números principais. |

## Checklist para um modo novo

- [ ] Não competitivo? Funciona com 1 jogador e começa em 1 (J1).
- [ ] Tela de configuração com o dispositivo de cada jogador e o Next bloqueado sem dispositivo (J3).
- [ ] Escolha de mapa com miniatura e Random; o fim volta para ela com o último mapa selecionado (M1, M2).
- [ ] `eventProcess` chama `extraModeInput`; a pausa usa `drawPauseBox` (M3, M4).
- [ ] Pasta de mapas, `maps.txt`, `<Modo>Layout` e o modo coberto pelo `make check-maps` (P1 a P4).
- [ ] Cores: de equipe no competitivo, de jogador no cooperativo; bônus cinza no cooperativo (C1, C2).
- [ ] Tabela de poderes própria, sem poder inútil no modo; painel com `drawPowerSlot` (W2, W4).
- [ ] `reservedTile` protege os pontos de nascimento (W5).
- [ ] Sinais visuais com `drawHaze` e caixas de mensagem (V1, V3, V4).
- [ ] A campanha continua idêntica pixel a pixel (K1).
- [ ] README em português e em inglês.

## Diferenças intencionais entre os modos

| O quê | Duelo | Sobrevivência | Por quê |
|-------|-------|---------------|---------|
| Jogadores | 2 a 4, em equipes | 1 a 4, juntos | J1, J2 |
| Cor do tanque | da equipe | do jogador | C1 |
| Bônus | da cor da equipe ou cinza | sempre cinza | C2 |
| Mina | respeita escudo e barco | destrói qualquer inimigo | W7: escudo é proteção entre jogadores |
| Trégua | fora do sorteio | existe | W2: no duelo nada surge |
| Tiro demolidor e pedra colorida | existem | não existem | estrutura com dono só existe com adversário humano (C3) |
| Quando dá para pausar | durante a rodada | durante a onda e o aviso dela (os jogadores já se movem) | M4 |
