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
| M3 | Teclas: **Enter / Start** pausa; **Esc / Select** sai para o menu, **só com o jogo pausado** (nunca avança nem encerra nada no meio da partida); na tela final, **tiro / Enter / A / Start** volta ao menu, liberado depois de 1,5 s (para não pular a tela sem querer). | `Game::extraModeInput` |
| M4 | A pausa é a **caixa padrão** ("PAUSE", "ENTER / START: PLAY" e "ESC / SELECT: MENU"). O modo decide quando dá para pausar: sempre que os jogadores controlam os tanques. | `Game::drawPauseBox`, `drawPause()` |
| M5 | **Botão de poder:** LB no controle, Shift esquerdo no `WASD`, Shift direito no `ARROWS`. | `PlayerKeys::power`, `Player::takePowerPress` |
| M6 | **Demonstração:** todo modo entra no sorteio do fundo do menu com uma linha em `Demo::modes()`: a fábrica cria o modo com os jogadores no computador (o competitivo com a IA dele; o cooperativo com `Player::cpu`, que o `Game` guia com `hunt`) e marca `m_demo`, que esconde as caixas do modo. | `Demo::modes`, `Game::m_demo`, `Game::hunt` |

## 3. Mapas

O mapa é de quem o desenha: o validador **só recusa o que impede o modo de funcionar ou de
ser justo**. Conselho de desenho vira **aviso** (o mapa é aceito) e, onde o jogo consegue se
adaptar ao mapa, ele se adapta, em vez de proibir um desenho.

| # | Regra | Onde |
|---|-------|------|
| P1 | Pasta própria `resources/<modo>_levels/`, com `maps.txt` (`arquivo;Nome`, um por linha) e mapas de 26×26 com os símbolos das fases. A pasta entra em `APP_RESOURCES` no `Makefile`. | `Makefile`, `AppConfig::<modo>_maps` |
| P2 | Uma classe `<Modo>Layout` separa **erros** (`validate`: o mapa é **recusado ao iniciar**, com o motivo no terminal, em vez de quebrar uma partida) de **avisos** (`advise`: o mapa funciona, vale rever). Os dois dizem linha e coluna. | `DuelLayout`, `SurvivalLayout` |
| P3 | `make check-maps` verifica as pastas de todos os modos e mostra os erros e os avisos. Só erro reprova. | `tools/check_duel_maps.cpp` |
| P4 | **Erro** só para o indispensável: formato (26×26, símbolos conhecidos), ponto de nascimento livre, caminho da **largura de um tanque** (2 tiles) até o objetivo (o flanco da base inimiga; na sobrevivência, a muralha da águia) e, no competitivo, **justiça**: o terreno de uma equipe é o da outra pela mesma transformação que leva as bases e os nascimentos de uma às da outra (no duelo, girar 180°; espelho não é exigido). | `<Modo>Layout::validate` |
| P5 | **O jogo se adapta** em vez de proibir: monta a base e a muralha; o dono da pedra é decidido pela **parede inteira** (dentro da área com dono, é da equipe; cruzando a borda, é comum), então a parede nunca fica metade colorida; o bônus não surge num ponto coberto (e volta quando o bloco cai). | `DuelLayout::stoneOwners`, `Duel::openSpots` |
| P6 | **Aviso**, não erro: ponto de bônus coberto ou inalcançável a pé, defensor longe dos flancos da própria base. São escolhas de desenho que o jogo aguenta. | `<Modo>Layout::advise` |

## 4. Cores

| # | Regra | Onde |
|---|-------|------|
| C1 | A cor do tanque diz **de que lado** o jogador está. Competitivo: a cor é da **equipe** (cores diferentes só entre adversários). Cooperativo: cada jogador tem a sua (P1 amarelo, P2 verde, P3 azul, P4 vermelho). | `Player::getPlayerColor`, `Duel::teamColor` |
| C2 | Bônus: no cooperativo, **todos cinza** (uma equipe só, qualquer um pega). Cor de equipe nos bônus é coisa de modo competitivo. | `Bonus::owner_team` |
| C3 | Estrutura **com dono** (competitivo) fica na **cor clareada do dono**. Cor numa estrutura quer dizer uma coisa só: o adversário não a derruba (o dono derruba). O dono é da parede inteira (P5). | `Duel::stoneTint`, `Duel::stoneOwner` |

## 5. Poderes

| # | Regra | Onde |
|---|-------|------|
| W1 | Uma classificação só: o que depende de **onde e quando** é usado é **guardável** (mina, barricada, torreta, retorno, turbo); o resto é imediato. | `Powers::storable` |
| W2 | Cada modo tem a **sua tabela de sorteio**, sem poder que não faça nada nele (a trégua não entra no duelo, onde nada surge). | `Powers::duelTable`, `Powers::survivalTable` |
| W3 | Com os espaços de poder cheios, o jogador **não pega outro bônus** (os bônus continuam no mapa). **Morrer perde** os poderes guardados. Um espaço guarda uma unidade; o modo pode dar mais espaços (a sobrevivência vende) e o botão usa as unidades na ordem em que entraram. | `Player::held_power`, `Player::power_stock`, `Player::destroy` |
| W4 | O painel mostra o **espaço do poder de cada jogador**: o ícone, ou uma moldura vazia. | `Game::drawPowerSlot` |
| W5 | Barricada e torreta só vão em área livre: dentro do mapa, sem cenário, **arbusto**, base, tanque ou torreta, e fora dos blocos que o modo reserva (pontos de nascimento). | `Game::areaFree`, `Game::rectFree`, `reservedTile()` do modo |
| W6 | Torreta e mina acabam sozinhas (tempo; a torreta também por munição), salvo no modo que as faz permanentes (a sobrevivência), onde só a destruição as tira, com um limite de torretas por jogador. O tiro fere o **corpo** da torreta, não o vão ao lado do cano. | `AppConfig::power_*`, `Turret::setPermanent`, `Turret::hitRect` |
| W7 | Competitivo: as proteções do jogador (**escudo, barco**) valem contra os poderes do adversário. Contra os inimigos da IA, o poder pode ser total. | `Duel::hitTank`, `Game::killEnemy` |
| W8 | **Poder com duração** (tempo ou munição) avisa que está acabando com a **névoa branca** do V1, a partir do último quarto, sobre o que o representa no mapa (o objeto, como a mina e a torreta; o tanque, como no turbo). O que fica no jogador também aparece no **espaço de poder do painel enquanto dura**, com a mesma névoa. **Vale para todo poder novo, em qualquer modo.** Hoje seguem: mina, torreta e turbo. Ainda sem a névoa (só mudam quando pedido): trégua (tem a contagem em texto), escudo de equipe e os originais da campanha (capacete, relógio, pá). | `Object::drawHaze`, `Object::hazeAlpha`, `Player::activePower`, `Game::drawPowerSlot` |
| W9 | **Poder colocado à frente** (barricada, torreta e os próximos) vai **logo à frente do tanque, na direção dele, nunca com um bloco vazio inteiro no meio**. O tanque para em qualquer pixel (anda `speed * dt`; só o lado se encaixa na grade ao virar). O que não precisa da grade (a torreta, que é um tanque parado) fica **encostado** no tanque e alinhado com ele; o que vira bloco do mapa (a barricada) vai na **primeira fileira de blocos que não toca o tanque**, com um vão menor que um bloco. Sem espaço ali, o poder **não procura outro lugar mais longe**: continua guardado. A mina é deixada **debaixo** do tanque (fica para trás quando ele anda), não à frente. Poder novo colocado à frente usa `frontArea`. | `Game::frontArea`, `Game::frontCell` |

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
| G1 | **Fogo amigo:** tiro não fere companheiro; o tiro de um **jogador** fere a **própria base** e derruba a **própria estrutura**, como no original. Tiro de bot aliado e de torreta não. | `Duel::bulletCanDamage`, `Duel::breaksBlock`, `Survival::onBaseHit` |
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
- [ ] Pasta de mapas, `maps.txt`, `<Modo>Layout` e o modo coberto pelo `make check-maps` (P1 a P3).
- [ ] O validador só recusa o indispensável; o resto é aviso ou o jogo se adapta (P4 a P6).
- [ ] Cores: de equipe no competitivo, de jogador no cooperativo; bônus cinza no cooperativo (C1, C2).
- [ ] Tabela de poderes própria, sem poder inútil no modo; painel com `drawPowerSlot` (W2, W4).
- [ ] `reservedTile` protege os pontos de nascimento (W5).
- [ ] Sinais visuais com `drawHaze` e caixas de mensagem (V1, V3, V4).
- [ ] Poder novo com duração: névoa branca no fim, no mapa e no painel enquanto dura (W8).
- [ ] Poder novo colocado à frente: `frontArea`, logo à frente do tanque, sem bloco vazio no meio (W9).
- [ ] Entra na demonstração do fundo do menu (`Demo::modes`), com todos os tanques no computador (M6).
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
| Poderes novos | caem como bônus, no sorteio | comprados na loja da equipe, ao lado da base, no intervalo entre as ondas, com as moedas da equipe (`survival_shop`) | com adversário humano o bônus no mapa é disputa; contra a IA, a loja vira decisão da equipe (onde e quando usar) |
| Tiro em tanque com estrela | destrói (`duel_star_armor`) | rebaixa um estágio, como na campanha | no duelo a armadura virava vidas extras para quem pega o canhão (desequilíbrio medido) |
| Mina e torreta | acabam (tempo; a torreta também por munição) | ficam até serem destruídas (até 3 torretas por jogador) | W6: no duelo, poder que fica para sempre vira fortaleza; na sobrevivência, a torreta é comprada e a defesa se constrói de onda em onda |
| Espaços de poder | 1 | 1, e a loja vende até 3 | W3: a loja é o lugar de planejar a defesa |
| Aliados do computador | bots de reforço da equipe, entre os inimigos, com a IA do duelo | tropa de reforço comprada na loja (`Game::m_allies`), atira só com o inimigo na mira | no duelo o bot é jogador de uma equipe; na sobrevivência, um ajudante contra a IA |
| Pedra | quebra só com o canhão | segura, mas raramente um inimigo a rompe (`survival_wall_breach_chance`) | evento raro de propósito, só contra a IA |
| Moedas | não há | da equipe ou de cada um (escolha no menu) | |
| Quando dá para pausar | durante a rodada | durante a onda e o aviso dela (os jogadores já se movem) | M4 |
