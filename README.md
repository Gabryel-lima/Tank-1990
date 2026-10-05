# Tank 1990 - Implementação em C++

> 🇬🇧 [English version](README.en.md)

Este é um clone do clássico jogo Tank 1990 (Battle City) implementado em C++ usando SDL2. Um jogo de ação estratégica onde você controla um tanque e deve proteger sua base enquanto elimina todos os inimigos.

## 🚀 Compilação e Execução

### Windows

No Windows o jogo roda dentro do **WSL** (Windows Subsystem for Linux), numa
distribuição Alpine Linux mínima. A janela do jogo abre normalmente na sua
área de trabalho, como qualquer programa.

> **Por que WSL e não um `.exe`?**
> O **Smart App Control** do Windows 11 bloqueia executáveis compilados
> localmente, porque eles não têm assinatura digital reconhecida. Não existe
> lista de exceções, e desligá-lo é irreversível (só volta reinstalando o
> Windows). O WSL contorna isso sem mexer na segurança da máquina.

**1. Instale a base do WSL** (só na primeira vez)

Abra o Terminal **como administrador** e rode:

```powershell
wsl --install --no-distribution
```

Reinicie o computador. O `--no-distribution` instala apenas a base, sem o
Ubuntu — o jogo usa o Alpine, que é bem menor.

**2. Instale o jogo**

Dê um duplo clique em **`install.cmd`** (ou rode no terminal, sem admin):

```
install.cmd
```

Ele baixa o Alpine Linux (~3,5 MB), instala o SDL2 e o compilador e compila o
jogo. Cada etapa mostra uma barra de progresso com o tempo decorrido e uma
estimativa do que falta. O compilador fica instalado para você poder
recompilar; ele só é apagado pelo `uninstall.cmd`.

**3. Jogue**

```
play.cmd
```

**4. Controles (opcional)**

Para jogar com controle USB, conecte-o e rode **`gamepads.cmd`** uma vez
(veja [Controles no Windows](#controles-no-windows-wsl)).

**Para desinstalar:**

```
uninstall.cmd
```

O desinstalador pergunta o que remover:

| Opção | O que apaga | Libera |
|---|---|---|
| 1 | O jogo, o compilador e as bibliotecas (mantém o Alpine no WSL) | ~350 MB |
| 2 | O jogo e a distribuição Alpine | ~400 MB |
| 3 | Tudo, incluindo a plataforma WSL | ~1,8 GB |

**Espaço em disco**

| Item | Tamanho |
|---|---|
| Plataforma WSL (uma vez, serve para tudo) | ~1,5 GB |
| Alpine + SDL2 + compilador + jogo | ~400 MB |
| Idem, instalado com `install.cmd --slim` (sem compilador) | ~250 MB |

### Linux / macOS

```bash
# 1. Instalar dependências
make install-deps

# 2. Compilar e executar o jogo
make run
```

### Comandos disponíveis (Linux, macOS e dentro do WSL)

```bash
make build       # Compila o projeto completo
make run         # Compila e executa o jogo
make clean       # Remove arquivos de build
make info        # Mostra informações do sistema
make doc         # Gera documentação (Doxygen)
make sprites     # Desenha a pixel art de tools/sprites/ em resources/png/texture.png
make pad-tools   # Diagnóstico (padprobe) e ponte (padbridge) de controles
make pad-selftest # Testa a ponte de controles de ponta a ponta
make install-deps # Instala as dependências (apk, apt, dnf ou brew)
make help        # Mostra todos os comandos disponíveis
```

### Recompilar depois de mexer no código (Windows/WSL)

O compilador continua instalado depois da instalação. Para aplicar suas
mudanças ao jogo que o `play.cmd` abre, rode o instalador de novo (ele
reaproveita o compilador e só recompila):

```
install.cmd
```

Para testar sem instalar, dá para compilar e rodar direto de dentro da pasta
do projeto:

```bat
wsl -d Tank1990 --cd "%CD%" -- make run
```

Se não pretende mexer no código e quer economizar ~150 MB, instale com
`install.cmd --slim`, que remove o compilador no fim.

### Dependências

- **SDL2** - Biblioteca gráfica principal
- **SDL2_image** - Carregamento de imagens
- **SDL2_mixer** - Sistema de áudio
- **SDL2_ttf** - Renderização de fontes
- **g++** - Compilador C++ (com suporte a C++17)

### Instalação Manual das Dependências

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
```

**macOS (com Homebrew):**
```bash
brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf
```

**Windows (WSL / Alpine):**

O `install.cmd` cuida disso sozinho. Se quiser fazer à mão, dentro da
distribuição:

```sh
apk add --no-cache g++ make sdl2-dev sdl2_image-dev sdl2_mixer-dev sdl2_ttf-dev mesa-dri-gallium \
    libxext libxcursor libxi libxrandr libxfixes libxscrnsaver mesa-gl mesa-egl
make run
```

**Windows (`.exe` nativo, opcional):**

O `Makefile` ainda compila um `.exe` com MinGW-w64, a partir do MSYS2 ou do
Git Bash. O SDL2 é procurado em `$SDL2_DIR`, `third_party/SDL2/<arch>-w64-mingw32`,
`$MINGW_HOME`, `resources/SDL/` e no `pkg-config`, nessa ordem:

```bash
make build                    # gera build/bin/Tanks.exe
make build ARCH=i686          # 32 bits
make build WIN_CONSOLE=1      # mantém o console aberto (depuração)
```

Lembrando que esse `.exe` provavelmente será **bloqueado pelo Smart App
Control** ao ser executado — veja a explicação na seção do Windows acima.

## 🎮 Funcionalidades

### Características Principais

- ✅ **36 níveis** com dificuldade progressiva
- ✅ **Suporte a 1-4 jogadores** simultâneos
- ✅ **Sistema de cores únicas** para cada jogador
- ✅ **Controles exclusivos** e sem conflitos para cada jogador
- ✅ **Gamepads para todos os jogadores**: D-pad ou analógico, com conexão a quente (hotplug)
- ✅ **Sistema de pontuação** com bônus por eliminação de inimigos
- ✅ **8 tipos de power-ups** com efeitos únicos
- ✅ **Sistema de estrelas** (0-3 níveis) que melhora o tanque
- ✅ **4 tipos de tanques inimigos** com comportamentos diferentes
- ✅ **Efeitos sonoros** e visuais
- ✅ **Sistema de vidas** e respawn
- ✅ **Proteção da base** (águia) com paredes de pedra
- ✅ **Modos extras**: duelo por equipes (10 mapas) e sobrevivência em ondas (1 a 4 jogadores)

## ⚔️ Modo Duelo (Extra Modes)

No menu principal, **Extra Modes → Duel Mode** abre o modo multijogador por equipes, só entre jogadores humanos: cada equipe defende a sua águia e tenta destruir a do adversário. A equipe **A** (amarela) nasce embaixo e a **B** (verde) em cima, em mapas próprios, separados da campanha (`resources/duel_levels/`), todos simétricos (girando o mapa 180°, um lado vira o outro) para que as duas equipes tenham o mesmo terreno.

**Formatos:** `1 vs 1`, `2 vs 2` ou `Custom Teams`, em que você escolhe de 2 a 4 jogadores e a equipe de cada um (2 contra 1, 3 contra 1...). Não há bots ocupando vagas; como o jogo aceita até 4 jogadores (ver [Controles](#-controles)), não existem 3 vs 3 nem 4 vs 4.

**Mapas:** depois de montar as equipes, **Next** abre a escolha de mapa, com uma miniatura do mapa selecionado. Se a lista não cabe na tela, ela rola junto com a seleção (setas à direita indicam que há mais itens acima ou abaixo):

| Mapa | Estilo |
|------|--------|
| **Arena** | Equilibrado: tijolos, rio nas laterais (o barco ajuda) e gelo no centro |
| **Fortress** | Defensivo: colunas de pedra e uma faixa de pedra no meio do campo |
| **River** | Um rio corta o meio do mapa, com três pontes |
| **Maze** | Labirinto de tijolos: dá para abrir caminho atirando |
| **Open Field** | Aberto e rápido: arbustos para emboscadas e gelo |
| **Crossroads** | Avenidas largas em cruz entre quarteirões de tijolo e arbusto |
| **Archipelago** | Ilhas de água ligadas por pontes de gelo; o barco abre atalhos |
| **Bunkers** | Casamatas de pedra espalhadas, com corredores estreitos até as bases |
| **Frozen Lake** | Lago de gelo no centro, cercado de arbustos: difícil parar e mirar |
| **Gauntlet** | Um portão central de pedra: quem atravessa encontra o adversário |
| **Random** | Um mapa sorteado a cada rodada |

**Criar um mapa novo** (sem mexer em código):

1. Salve uma grade de **26×26** em `resources/duel_levels/` com os símbolos das fases: `#` tijolo, `@` pedra, `~` água, `%` arbusto, `-` gelo, `.` vazio.
2. Acrescente uma linha `arquivo;Nome` em `resources/duel_levels/maps.txt`. O nome é o que aparece no menu.
3. Rode `make check-maps`. Ele usa as mesmas regras do jogo (`DuelLayout`) e aponta linha e coluna de cada erro (o mapa seria recusado) e de cada aviso (o mapa funciona, vale rever). Também verifica os mapas da sobrevivência.

**A base** é montada pelo jogo, igual em todo mapa (não precisa estar no arquivo):

```
............   pátio (2 linhas livres): por onde o defensor contorna a águia
............   e chega ao flanco de onde o inimigo vier
....#@@#....   frente blindada: 2 blocos de pedra
....#EE#....   laterais e cantos de tijolo (E = águia): a base cai pelos flancos
....#EE#....
```

Atirar de frente, de perto ou de longe pelo meio do mapa, não adianta (nem com o canhão; só o tiro demolidor, de perto). O ataque é pelos dois flancos, e o defensor sempre consegue ir de um lado para o outro pelo pátio, então não existe uma entrada única para alguém ficar esperando.

O que o jogo cuida sozinho, em qualquer mapa: as águias (colunas 12-13, nas duas primeiras e nas duas últimas linhas), a base acima; a **zona da base** (colunas 9-16, nas 7 linhas do lado de cada águia), onde o canhão não vale e a parede de pedra que fica inteira nela é da equipe; os pontos de nascimento sem mira; e os pontos de bônus (o coberto por um bloco fica de fora até o bloco cair).

O validador limita o mínimo possível quem desenha. **Erro** (o mapa é recusado ao iniciar o jogo, com o motivo no terminal, em vez de quebrar uma partida) só para o indispensável: ser **simétrico girando 180°** (as duas equipes com o mesmo terreno; espelho na horizontal e na vertical também vale, mas não é exigido: cata-ventos e diagonais servem); deixar livres os pontos de nascimento (colunas 4-5, 8-9, 16-17 e 20-21, nas linhas 0-1 e 24-25); e ter caminho com **2 tiles de largura** (a largura de um tanque) de cada nascimento até um **flanco da base inimiga**. **Aviso** (o mapa é aceito, o `check-maps` mostra): ponto de bônus coberto (o bônus não surge ali até o bloco cair) ou inalcançável a pé, e defensor a mais de 20 passos de um flanco da própria base. E o jogo se adapta ao resto: monta a base, e uma parede de pedra que cruza a borda da zona da base fica inteira como pedra comum.

**Equilíbrio** (o `check-maps` não verifica): num mapa espelhado, se ninguém defende, cada equipe ataca por um lado, as duas nunca se cruzam e a rodada vira uma corrida de quem chega primeiro. Confira com o `duel_sim`: no 2 contra 2 (um ataca e um defende, `--teams ABAB`), rodadas curtas com quase 100% de vitórias por base indicam um caminho fácil demais até os flancos inimigos.

| Na configuração | Tecla / controle |
|-----------------|------------------|
| Mudar de opção | ↑ ↓ / D-pad / analógico |
| Mudar o valor | ← → / D-pad / analógico |
| Confirmar | Enter, Espaço / A, Start |
| Voltar | Esc / B, Back |

**Regras:**
- Vence a rodada quem destruir a base inimiga ou eliminar todos os jogadores inimigos; vence a partida quem ganhar **2 rodadas**.
- Cada jogador tem **3 vidas** e renasce com escudo por alguns segundos.
- **Nascimento sem mira:** cada jogador nasce numa coluna só dele, alternando os lados da base (no 1 contra 1, um nasce à esquerda e o outro à direita). Como o tiro só anda em linha reta, ninguém nasce na linha de tiro de um adversário, em nenhum mapa ou formato; o escudo de nascimento fica como proteção extra.
- **Cores:** a cor é da **equipe**: companheiros têm a mesma cor (equipe A amarela, B verde) e cores diferentes só aparecem entre adversários. A paleta tem 4 cores (amarelo, verde, azul, vermelho), pronta para um futuro modo cada um por si, em que cada jogador seria a própria equipe. O painel lateral lista os jogadores de cada equipe e as vidas de cada um.
- **Bônus coloridos e cinza:** qualquer bônus pode surgir de duas formas:
  - **na cor de uma equipe** (com a letra **A** ou **B** em cima, ~65% das vezes): só jogadores dela conseguem pegar; os adversários passam por cima. Aparece **70% das vezes na metade do adversário** (é preciso invadir para buscar) e 30% na própria. A equipe é sorteada 50/50 (ou é a que estiver atrás em vidas), inclusive num 3 contra 1;
  - **cinza**, com o ícone original do jogo (~35%): **qualquer jogador** pega. Surge num ponto simétrico do meio do mapa (à mesma distância das duas bases) ou, se uma equipe estiver bem atrás em vidas, do lado dela.
- **Reforço:** o bônus de tanque traz um **bot aliado** na cor da equipe, com uma vida. O primeiro reforço guarda a base e o segundo ataca.
- **IA dos reforços:** o bot calcula um caminho pelo mapa (busca em grade, como num GPS), contornando pedra e água e abrindo caminho a tiro pelos tijolos, então funciona em qualquer mapa, inclusive nos personalizados. Ele atira quando um inimigo ou a base inimiga está na linha de tiro, vira para atirar em quem aparece ao lado e **segura a mira** enquanto o alvo continua na linha (antes virava e desvirava várias vezes por segundo); não dá meia-volta logo depois de virar; persegue um inimigo com histerese (começa a caçar a 10 blocos e só desiste a 14); os defensores guardam a base (um no pátio, à mesma distância dos dois flancos), cada um no seu posto, sem tapar a saída de quem nasce atrás; e, se ficar preso, desvia e recalcula. A IA nunca atira na direção da própria base.
- **Fogo amigo:** tiros não ferem companheiros. O tiro de um **jogador** destrói a **própria base** (a rodada vai para o adversário) e derruba os tijolos em volta dela, como no original, e também a pedra colorida da própria zona (ver abaixo): dá para abrir um ângulo de tiro, mas cuidado com a mira. O tiro do bot de reforço não fere a própria base.
- **Bônus:** com o mapa vazio de bônus por 10 s, surge o próximo. Só jogadores coletam (reforços não).
- Efeitos no duelo: **granada** explode os inimigos em campo, inclusive com escudo ou barco (é uma explosão, não um tiro; o companheiro não é atingido); **capacete** dá escudo por **6 s** (10 s na campanha); **relógio** imobiliza a equipe inimiga por 4 s (humanos ainda giram e atiram); **pá** reforça a **sua** base com pedra; **canhão** (3 estrelas) quebra pedra do cenário, mas não perto das bases (ver abaixo). Granada e canhão são os mais raros. No duelo, **estrela não segura tiro** (na campanha e na sobrevivência, o tiro só rebaixa o tanque um estágio): o canhão quebra pedra, mas não vale vidas extras.
- **Zona da base:** perto de cada águia (colunas 9 a 16, nas 7 linhas do lado da base), o canhão não vale: a bala age como uma comum, desgasta tijolo e para na pedra. A pedra da zona fica **na cor da equipe dona** (dourada embaixo, verde em cima): o adversário não a derruba (nem com o canhão, só com o tiro demolidor), mas **qualquer tiro de jogador da própria equipe a derruba**, como os tijolos da própria muralha. O defensor abre o caminho que quiser em casa, por exemplo para tirar um atacante escondido atrás de uma pedra; o tiro dos bots de reforço e da torreta não derruba. O dono é decidido pela **parede inteira** (blocos de pedra ligados lado com lado): a parede toda dentro da zona é da equipe; a que cruza a borda é pedra comum (cinza, o canhão quebra), então nenhuma parede fica metade de cada jeito. A muralha da águia é sempre da equipe.
- **Tiro demolidor (segundo estágio do tanque):** quem pega o **canhão já tendo estrela** (na mesma vida) ganha o tiro demolidor. Ele derruba a **pedra da base inimiga** (a frente, a pá e a base toda de pedra do 1 contra 3), mas **só disparado de dentro da zona dela**: abre o ataque por cima para quem chega perto, sem tiro de longe, de uma base para a outra. Quem tem o tiro demolidor dá um brilho branco rápido a cada segundo e aparece com o ícone do canhão no painel; **a pedra colorida da base ameaçada pisca junto com ele**, no mesmo ritmo, avisando o defensor. Morrer faz perdê-lo. Quantas estrelas são exigidas fica em `AppConfig::duel_demolisher_stars` (padrão 1).
- **Cadência:** no duelo, cada jogador dispara no máximo **3 tiros por segundo** (`AppConfig::duel_max_shots_per_second`), para que uma rajada não derrube a base inimiga sem chance de defesa.
- **Equipes de tamanhos diferentes:** a menor recebe mais vidas por jogador (1 contra 3: 6 vidas contra 3) e, se a outra tiver o dobro de jogadores ou mais, a base inteira de pedra (aí só se vence eliminando os jogadores).

**Simulação de equilíbrio:** `make duel-sim` compila `build/bin/duel_sim`, que joga partidas inteiras sem janela, com todos os jogadores controlados pela IA, e mostra quantas rodadas cada equipe vence em cada mapa, quanto cada bônus ajuda quem o pega (separado por quem estava atrás, parelho ou na frente em vidas) e quanto tempo os bots passam presos. Os ajustes de equilíbrio (`AppConfig::duel_*`) podem ser testados sem recompilar:

```bash
make duel-sim
cd build/bin
./duel_sim --matches 200 --teams ABAB --map 0      # 2 contra 2 na Arena
./duel_sim --teams AAB --helmet 8000 --heat        # 2 contra 1, capacete de 8 s, mapa de calor dos bots
```

A IA não joga como uma pessoa, então os números indicam tendências (um bônus que decide a rodada sozinho, um mapa que favorece um lado), não o resultado exato entre jogadores.
- Enter / Start pausa; com o jogo pausado, Esc / Select abandona a partida (a caixa da pausa mostra os dois). Ao sair ou no fim da partida (tiro / Enter / A), o jogo volta para a escolha de mapa, com o último já selecionado: revanche com um botão.

## 🛡️ Modo Sobrevivência (Extra Modes)

**Extra Modes → Survival**: de 1 a 4 jogadores, juntos, defendendo a águia contra **ondas de inimigos sem fim**. Escolha a quantidade de jogadores (← →; a tela mostra o dispositivo de cada um, como no duelo), **Next** e o mapa (com miniatura, ou **Random**). Cada jogador tem a sua cor (P1 amarelo, P2 verde, P3 azul, P4 vermelho), e o painel lateral mostra a onda, os inimigos que faltam e as vidas de cada um.

**Mapas** (`resources/survival_levels/`, separados da campanha e do duelo):

| Mapa | Estilo |
|------|--------|
| **Classic** | Um mapa no estilo da campanha: arbustos, tijolos e pedra |
| **Trenches** | Trincheiras de tijolo atravessando o mapa, com passagens |
| **Canyon** | Faixas de água com pontes; o barco abre atalhos |
| **Forest** | Mata fechada: os inimigos aparecem de perto |
| **Ice Rink** | Pista de gelo no meio: difícil parar e mirar |
| **Citadel** | Muralhas de pedra com portões em volta da base |
| **Labyrinth** | Labirinto de tijolos: dá para abrir caminho atirando |
| **Islands** | Ilhas de água com arbustos |
| **Crossfire** | Cruzes de tijolo e pilares de pedra, muitos ângulos de tiro |
| **Last Stand** | Camadas de tijolo protegendo a base, topo aberto |
| **Random** | Um mapa sorteado |

Criar um mapa novo segue o mesmo caminho do duelo: grade de **26×26** com os símbolos das fases em `resources/survival_levels/`, uma linha `arquivo;Nome` no `maps.txt` da pasta e `make check-maps`. O jogo monta a águia e a muralha de tijolos dela; o mapa precisa deixar livres os pontos onde os inimigos surgem (no topo) e onde os jogadores nascem, e ligar todos eles e a muralha da águia por caminhos da largura de um tanque. Mapas que não passam são recusados ao iniciar o jogo, com o motivo no terminal.

- Cada onda tem mais inimigos (6, 8, 10... até 40), mais deles no mapa ao mesmo tempo (4 na primeira onda, +1 por jogador extra e +1 a cada 3 ondas, até 10) e mais blindados (a onda N usa a dificuldade da fase 2N + 1 da campanha, até a 35).
- Entre as ondas há uma pausa com o aviso **WAVE N** (os jogadores já podem se posicionar), e sobreviver a uma onda é recompensado:
  - **o mapa inteiro regenera**: os tijolos, a pedra e os arbustos destruídos voltam, e a muralha da águia é refeita. Só volta o que era do mapa: as barricadas que os jogadores puseram ficam, nada nasce em cima de um tanque, torreta, mina ou bônus, e a pedra da pá fica até o tempo dela acabar;
  - **quem tinha caído volta ao jogo** com uma vida (o aviso mostra **P2 IS BACK**, na cor do jogador);
  - a cada **3 ondas**, todos ganham uma vida (**+1 LIFE**).
- Bônus, pontos e fogo amigo como na campanha (o tiro do jogador também derruba a própria águia). Os jogadores têm cores diferentes, mas são **uma equipe só**: todo bônus é **cinza**, com o ícone original, e qualquer jogador pega (as cores de equipe nos bônus são só do duelo).
- **Loja:** os 8 bônus originais continuam caindo dos tanques que piscam, como na campanha; os 9 poderes novos são **comprados**, com as **moedas da equipe**: cada 100 pontos de qualquer jogador valem uma moeda (um inimigo básico dá 1, o blindado 4), e o painel mostra o saldo no bloco dourado **$**. A loja de cada jogador fica no **seu ponto de nascimento**, ao lado da base, marcado no chão com cantoneiras na cor dele:
  - parado no ponto, com o espaço de poder vazio, o **botão de poder** abre a loja e passa para o próximo item; o **tiro compra** (o preço fica vermelho sem moedas suficientes); andar fecha, e ela fecha sozinha depois de 6 s sem apertar nada;
  - parado no ponto **sem abrir a loja, o tiro atira normalmente**: quem defende a base dali não compra sem querer. A dica **POWER: SHOP** aparece só nos primeiros 3 s, para não tapar o mapa;
  - com a loja aberta o tanque não atira: comprar é um momento de risco, e a hora boa é entre as ondas;
  - com um poder guardado, o botão de poder o usa, como em qualquer lugar (uma torreta comprada pode ir logo na frente da base);
  - preços: barricada e turbo 8, mina e retorno 10, reparo 12, reviver 15, torreta, trégua e escudo de equipe 20. A lista, a ordem e os preços ficam em `AppConfig::survival_shop_items`; `AppConfig::survival_shop = false` volta ao sorteio de todos os poderes no mapa.
- Acaba quando a águia cai ou todos perdem as vidas: a tela final mostra a onda alcançada, os tanques destruídos e os pontos de cada jogador. Tiro / Enter / A volta para a escolha de mapa, com o último selecionado: jogar de novo é um botão só.
- Os números ficam em `AppConfig::survival_*`.

## 🧨 Poderes novos (duelo e sobrevivência)

Além dos 8 bônus originais, os modos extras têm 9 poderes novos, com pixel art própria no estilo dos ícones do NES. Eles se dividem em dois tipos:

- **Guardáveis:** vão para o **espaço de poder** do jogador e são usados quando ele quiser, com o **botão de poder**. Enquanto guarda um poder, o jogador **não pega outro bônus**: os bônus continuam surgindo (e ficam no mapa), mas só dá para pegá-los depois de usar o que está guardado. **Morrer faz perder o poder guardado**, como as estrelas: guardar tem risco.
- **Imediatos:** fazem efeito na hora, como os bônus originais.

| Poder | Tipo | Efeito |
|-------|------|--------|
| **Mina** | guardável | Deixa uma mina onde o tanque está. Explode o primeiro tanque **adversário** que passar por cima (respeita escudo e barco, para não virar arma contra quem acabou de nascer). Qualquer tiro a detona antes. Dura 30 s; no último quarto, uma névoa branca pulsa sobre ela. Na sobrevivência, destrói até inimigo blindado. |
| **Barricada** | guardável | Levanta um bloco de tijolos 2×2 logo à frente. Não pode ser colocada em cima de tanque, base, cenário, arbusto ou ponto de nascimento; sem espaço, o poder continua guardado. |
| **Torreta** | guardável | Instala à frente um canhão fixo, na cor do dono, virado para onde o tanque olha. Ela gira e atira sozinha nos inimigos alinhados a até 12 tiles, **nunca na direção da própria base**. Um tiro a destrói. Acaba no que vier primeiro: **10 tiros ou 20 s**. Quando está acabando (últimos 3 tiros ou último quarto do tempo), uma névoa branca pulsa sobre ela, mais rápida perto do fim. |
| **Retorno** | guardável | Teleporta o tanque para o seu ponto de nascimento, ao lado da base (se estiver ocupado, para outro da equipe). Mantém o barco e as estrelas. É a resposta a uma invasão quando se está longe de casa. |
| **Turbo** | guardável | Velocidade ×1,5 por 8 s. |
| **Reviver** | imediato | Um companheiro que caiu volta com uma vida; se ninguém caiu, uma vida extra para quem da equipe tem menos. |
| **Reparo** | imediato | Refaz a muralha da própria base. |
| **Escudo de equipe** | imediato | Escudo para todos os jogadores da equipe ao mesmo tempo. |
| **Trégua** | imediato, **só na sobrevivência** | Por 10 s, nenhum inimigo novo entra no mapa (aviso **TRUCE** no topo). No duelo nada surge, então não teria efeito: fica fora do sorteio. |

**Por que esses são guardáveis e os outros não:** guardar só faz sentido quando o valor do poder depende de **onde** e **quando** ele é usado. Uma mina, uma barricada ou uma torreta valem muito na entrada da base e quase nada no lugar onde o bônus apareceu por acaso; o retorno só serve quando a base está sob ataque; o turbo, na hora de fugir ou de invadir. Já estrela, capacete, reviver, reparo e escudo valem o mesmo a qualquer hora (ou valem mais se usados logo), então guardá-los só atrasaria o efeito.

**Botão de poder:**

| Dispositivo | Botão |
|-------------|-------|
| Controle | **LB** (ombro esquerdo; L1 no PlayStation, L no Switch) |
| Teclado `WASD` | **Shift esquerdo** |
| Teclado `ARROWS` | **Shift direito** |

**No painel lateral**, abaixo das vidas de cada jogador, aparece o poder guardado (um quadrado vazio quando não há nenhum). No duelo, de 1 contra 1 até 4 jogadores, o painel de cada equipe cresce para mostrar uma linha por jogador; na sobrevivência, o ícone fica ao lado das vidas.

**Na sobrevivência** os poderes novos são comprados na loja (ver acima), e os mesmos 5 poderes também são guardáveis (`AppConfig::survival_store_powers = true`). Os bônus surgem em lugares aleatórios do mapa, então uma mina ou uma torreta usada na hora quase sempre cairia num canto sem inimigos; guardada, vira defesa da águia. Com `survival_store_powers = false`, todo poder é usado na hora em que é pego.

**Sorteio:** no duelo, os poderes novos dividem o sorteio com os originais; os mais fortes (torreta, reviver e escudo de equipe) são raros como a granada e o canhão (em 1 contra 1 o reviver vira só uma vida extra). As chances ficam em `Powers::duelTable()` e `Powers::survivalTable()` (`src/app_state/powers.cpp`); os tempos e alcances, em `AppConfig::power_*`.

**A arte** fica em texto, em `tools/sprites/powers.txt` (um caractere por pixel, com a paleta dos ícones originais), e `make sprites` a desenha em `resources/png/texture.png`. Para ajustar um ícone, edite o desenho e rode `make sprites` de novo.

## 🎯 Power-ups e Bônus

O jogo possui 8 tipos diferentes de power-ups que aparecem aleatoriamente quando você destrói tanques inimigos:

| Power-up | Efeito |
|----------|--------|
| **⭐ Estrela** | Aumenta o nível de estrela em 1 (máximo 3). Melhora velocidade e poder de fogo |
| **💣 Granada** | Destrói todos os inimigos no mapa instantaneamente |
| **🛡️ Capacete** | Concede escudo temporário que protege contra danos |
| **⏰ Relógio** | Congela todos os inimigos por um período |
| **⛏️ Pá** | Protege a base (águia) com paredes de pedra indestrutíveis |
| **🚗 Tanque** | Adiciona uma vida extra ao jogador (no modo duelo, traz um bot aliado de reforço) |
| **🔫 Canhão** | Aumenta o nível de estrela em 3 (máximo) |
| **🚤 Barco** | Permite atravessar água sem afundar |

### Sistema de Estrelas

O sistema de estrelas (0-3 níveis) melhora progressivamente o tanque:

- **0 estrelas**: tanque básico, 1 tiro por vez
- **1 estrela**: tanque e tiro 30% mais rápidos
- **2 estrelas**: 2 tiros por vez (rajada dupla)
- **3 estrelas**: 3 tiros por vez, e o tiro quebra pedra

O tanque muda de desenho a cada estágio, do leve ao pesado. **Ser atingido com estrela não mata: o tanque volta um estágio** (do pesado para o médio, para o leve, para o básico); só o tanque básico é destruído, e aí perde também o poder guardado. Vale na campanha e na sobrevivência. No Battle City original qualquer tiro mata; esta é uma regra deste jogo.

## 🎮 Controles

O jogo é pensado para **controle (gamepad)**; o teclado entra automaticamente
como reserva de quem não tiver controle. Não há nada para configurar.

| Jogador | Cor | Posição inicial (campanha) |
|---------|-----|----------------------------|
| **Player 1** | Amarelo | Inferior esquerda |
| **Player 2** | Verde | Inferior direita |
| **Player 3** | Azul | Superior esquerda |
| **Player 4** | Vermelho | Superior direita |

**No controle**, para qualquer jogador:

- **Mover**: D-pad ou analógico esquerdo
- **Atirar**: qualquer botão frontal (A, B, X ou Y)
- **Usar o poder guardado** (duelo e sobrevivência): LB
- **Start**: pausa · **Back/Select**: volta ao menu (nos modos extras, só com o jogo pausado)
- **No menu**: D-pad ou analógico para escolher, A/Start para confirmar, B/Back para sair

**No teclado** há dois layouts: `WASD` (`W` `A` `S` `D` + `Espaço`, poder no
`Shift esquerdo`) e `ARROWS` (setas + `Ctrl direito`, `Alt direito` no Mac; poder no
`Shift direito`). Cada layout controla um único jogador: apertar a tecla de um nunca
move ou faz atirar outro.

### Qual dispositivo fica com qual jogador

1. **Controle primeiro**: o 1º controle conectado vai para o Player 1, o 2º para o Player 2, e assim por diante.
2. **Teclado como reserva**: quem ficou sem controle recebe, na ordem, `WASD` e depois `ARROWS`.
3. Os layouts que ninguém precisou continuam com o Player 1 (`WASD`) e o Player 2 (`ARROWS`): quem joga sozinho pode usar o teclado mesmo com um controle conectado.

| Partida | 0 controles | 1 controle | 2 controles |
|---------|-------------|------------|-------------|
| 2 jogadores | P1 WASD, P2 ARROWS | P1 controle, P2 WASD | P1 e P2 controle |
| 3 jogadores | P3 sem dispositivo | P1 controle, P2 WASD, P3 ARROWS | P1, P2 controle, P3 WASD |
| 4 jogadores | P3 e P4 sem dispositivo | P4 sem dispositivo | P1, P2 controle, P3 WASD, P4 ARROWS |

Na configuração do **modo duelo**, cada jogador mostra o dispositivo que vai usar
(`PAD 1`, `WASD`, `ARROWS`). Quem ficar sem nenhum aparece em vermelho como
`NO PAD`, o título avisa quantos controles faltam e o **Start só libera quando
todos tiverem um dispositivo**. A tela se atualiza sozinha ao conectar um controle.

Controles podem ser conectados ou desconectados com o jogo aberto. Se um controle
desconectar no meio da partida, os outros **não trocam de jogador**: quem perdeu o
controle passa para o teclado (se houver layout livre) e recebe o controle de volta
ao reconectar. O jogo aceita qualquer controle que o SDL2 reconheça como
*game controller* (Xbox, PlayStation, Switch Pro e a maioria dos genéricos).

### Controles no Windows (WSL)

O WSL não enxerga controles USB sozinho. O jogo usa o
[usbipd-win](https://github.com/dorssel/usbipd-win) para emprestar o controle
ao WSL enquanto roda:

1. Conecte os controles **por cabo USB** e rode **`gamepads.cmd`** uma vez.
   Ele instala o usbipd-win (se precisar) e autoriza cada modelo de controle
   conectado. Pede permissão de administrador.
2. Jogue pelo `play.cmd` normalmente. Ele repassa os controles autorizados ao
   abrir o jogo (inclusive os conectados depois) e os **devolve ao Windows**
   quando o jogo fecha. Enquanto o jogo está aberto, o controle não funciona
   em outros programas do Windows.

Só é preciso rodar o `gamepads.cmd` de novo para um **modelo** de controle
novo. Outros comandos: `gamepads.cmd --list` (mostra os autorizados) e
`gamepads.cmd --remove` (remove as autorizações).

| Controle | Funciona no WSL? |
|---|---|
| PlayStation (DualShock 4, DualSense), Switch Pro, 8BitDo, genéricos USB | Sim |
| Xbox 360 / One / Series (cabo) | Sim, via o SDL2 com libusb que o `install.cmd` compila |
| Qualquer controle por **Bluetooth** | Sim, pela ponte de controles (abaixo) |

O `gamepads.cmd` só autoriza dispositivos que o Windows identifica como
gamepad/joystick (ou controle Xbox); teclado e mouse nunca são repassados.

### Controles por Bluetooth

Pareie o controle **no sistema** (Configurações → Bluetooth, no Windows e no macOS;
`bluetoothctl` ou as configurações da área de trabalho, no Linux) e abra o jogo. O jogo usa o
SDL2, que recebe o controle já tratado pelo sistema: por cabo ou por Bluetooth, para o jogo é o
mesmo controle. A investigação completa (protocolos, APIs de cada sistema, WSL2, fatos e
hipóteses) está em [CONTROLES.md](CONTROLES.md).

| Onde o jogo roda | Controle Bluetooth |
|---|---|
| Linux, macOS, Windows nativo (MSYS2) | Direto: pareie e jogue. No macOS, se aparecer o pedido de **Monitoramento de Entrada**, permita. |
| Windows com o `install.cmd` (WSL) | Pela **ponte de controles**: o `install.cmd` gera o `padbridge.exe`, e o `play.cmd` o abre em segundo plano. Ele lê no Windows os controles que o WSL não enxerga e os manda ao jogo, onde viram controles comuns. |

Sobre a ponte, no Windows:

- Ela leva **qualquer** controle que o Windows enxergue: Bluetooth, e também por cabo, mesmo sem
  o `gamepads.cmd`.
- Os controles repassados pelo `gamepads.cmd` saem do Windows, então não chegam em dobro.
- Se o **Smart App Control** do Windows 11 bloquear o `padbridge.exe` (é um executável compilado
  na sua máquina), o jogo abre do mesmo jeito, sem os controles Bluetooth. Nesse caso, use o
  controle por cabo com o `gamepads.cmd`.

**Diagnóstico** (em qualquer sistema):

```bash
make pad-tools
build/bin/padprobe          # lista os controles (nome, USB/Bluetooth, VID:PID) e mostra os botões
build/bin/padprobe --list   # só lista
make pad-selftest           # testa a ponte de ponta a ponta, sem controle de verdade
```

No Windows com WSL, o `padbridge.exe --list` (em `%LOCALAPPDATA%\Tank1990\bridge`) mostra o que a
ponte enxerga.

## 👾 Tipos de Inimigos

O jogo possui 4 tipos diferentes de tanques inimigos, cada um com características próprias:

- **Tanque Tipo A**: Inimigo básico
- **Tanque Tipo B**: Inimigo intermediário
- **Tanque Tipo C**: Inimigo avançado
- **Tanque Tipo D**: Inimigo elite

Cada tipo de inimigo tem diferentes padrões de movimento, velocidade e comportamento de combate.

## 🗺️ Elementos do Mapa

O jogo possui diversos elementos interativos no mapa:

- **🧱 Parede de Tijolos**: Destrutível por projéteis
- **🪨 Parede de Pedra**: Indestrutível, bloqueia projéteis e tanques
- **💧 Água**: Obstáculo intransponível (exceto com power-up Barco)
- **🌿 Arbusto**: Esconde tanques, mas não bloqueia projéteis
- **🧊 Gelo**: Faz o tanque deslizar, dificultando o controle
- **🦅 Águia**: Base do jogador que deve ser protegida

## 📁 Estrutura do Projeto

```
Tank-1990/
├── src/
│   ├── objects/          # Classes dos objetos do jogo
│   │   ├── player.h/cpp  # Jogador controlável
│   │   ├── enemy.h/cpp   # Tanques inimigos
│   │   ├── bot.h/cpp     # Bot aliado (bônus de reforço) no modo duelo
│   │   ├── mine.h/cpp    # Mina (poder dos modos extras)
│   │   ├── turret.h/cpp  # Torreta fixa (poder dos modos extras)
│   │   ├── tank.h/cpp    # Classe base dos tanques
│   │   ├── bullet.h/cpp  # Projéteis
│   │   ├── bonus.h/cpp   # Power-ups
│   │   ├── brick.h/cpp   # Paredes de tijolo
│   │   └── eagle.h/cpp   # Base (águia)
│   ├── app_state/        # Estados da aplicação
│   │   ├── menu.h/cpp    # Menu principal
│   │   ├── game.h/cpp    # Lógica principal do jogo
│   │   ├── duel.h/cpp    # Modo duelo (equipes)
│   │   ├── duel_layout.h/cpp # Geometria do duelo e validação dos mapas
│   │   ├── duel_ai.cpp   # IA do duelo (bots de reforço)
│   │   ├── survival.h/cpp # Modo sobrevivência (ondas)
│   │   ├── powers.h/cpp  # Poderes novos: guardáveis x imediatos e chances de sorteio
│   │   ├── survival_layout.h/cpp # Geometria e validação dos mapas da sobrevivência
│   │   ├── message_box.h/cpp # Caixa de mensagem dos modos extras
│   │   ├── navgrid.h/cpp # Busca de caminho em grade (Dijkstra) usada pela IA
│   │   └── scores.h/cpp  # Tela de pontuação
│   ├── engine/           # Motor do jogo
│   │   ├── renderer.h/cpp    # Sistema de renderização
│   │   ├── engine.h/cpp      # Motor principal
│   │   └── spriteconfig.h/cpp # Configuração de sprites
│   ├── app.h/cpp         # Aplicação principal
│   ├── appconfig.h/cpp   # Configurações globais (inclui os layouts de teclado)
│   ├── controllers.h/cpp # Gamepads: hotplug e distribuição entre jogadores
│   ├── input/            # Controles abaixo do jogo (ver CONTROLES.md)
│   │   ├── netpad.h/cpp  # Controles da ponte (rede) viram controles virtuais do SDL
│   │   ├── pad_protocol.h # Protocolo da ponte
│   │   ├── pad_socket.h/cpp # TCP portátil (Winsock / BSD)
│   │   └── pad_info.h/cpp # Barramento (USB/Bluetooth/virtual) e VID:PID, para diagnóstico
│   ├── soundmanager.h/cpp # Gerenciador de áudio
│   └── type.h            # Definições de tipos
├── resources/            # Recursos do jogo
│   ├── img/              # Imagens e sprites
│   ├── sound/            # Efeitos sonoros
│   ├── font/             # Fontes do jogo
│   ├── levels/           # Arquivos dos 36 níveis
│   ├── duel_levels/      # Mapas do modo duelo (lista em maps.txt)
│   └── survival_levels/  # Mapas do modo sobrevivência (lista em maps.txt)
├── tools/
│   ├── duel_sim.cpp      # Simulação do duelo sem janela (IA contra IA)
│   ├── paint_sprites.cpp # Desenha a pixel art em texto na textura (make sprites)
│   ├── padprobe.cpp      # Diagnóstico de controles (USB/Bluetooth/virtual, eventos)
│   ├── padbridge.cpp     # Ponte de controles: lê no Windows, manda ao jogo no WSL
│   ├── pad-selftest.sh   # Teste de ponta a ponta da ponte (make pad-selftest)
│   ├── sprites/powers.txt # Pixel art dos poderes novos
│   └── duel_sim_report.py # Soma os resultados de várias simulações
├── build/                # Arquivos de build (gerado)
├── Makefile              # Sistema de build
└── README.md             # Este arquivo
```

## 🎯 Objetivo do Jogo

O objetivo principal é proteger sua base (águia) enquanto elimina todos os tanques inimigos em cada nível. Você perde se:
- A águia for destruída
- Você perder todas as vidas

Você avança para o próximo nível quando:
- Todos os inimigos são eliminados
- Você ainda tem pelo menos uma vida
- A águia está intacta

## 🔧 Tecnologias Utilizadas

- **C++17**: Linguagem principal com recursos modernos
- **SDL2**: Biblioteca multiplataforma para gráficos, áudio e input
- **SDL2_image**: Suporte a múltiplos formatos de imagem (PNG, BMP, etc.)
- **SDL2_mixer**: Sistema de áudio para efeitos sonoros e música
- **SDL2_ttf**: Renderização de fontes TrueType
- **Make**: Sistema de build automatizado
- **Doxygen**: Geração de documentação (opcional)

## 📝 Notas de Desenvolvimento

### Arquitetura
- **Padrão State Machine**: Estados do jogo (Menu, Game, Scores) gerenciados por `AppState`
- **Herança**: Sistema de classes base (`Object`, `Tank`) com especializações (`Player`, `Enemy`)
- **Singleton**: `SoundManager` e `Renderer` usam padrão Singleton
- **Configuração Centralizada**: `AppConfig` contém todas as constantes do jogo

### Regras dos modos extras
Todo modo extra, existente ou novo, segue as regras de **[MODOS_EXTRAS.md](MODOS_EXTRAS.md)**: por exemplo, modo não competitivo funciona com 1 jogador; a configuração mostra o dispositivo de cada jogador; o fim volta para a escolha de mapa; o botão de poder é o mesmo; o que está acabando ganha a névoa branca. O arquivo também traz o checklist de um modo novo e as diferenças intencionais entre os modos.

### Sistema de Cores
Cada jogador tem uma cor única aplicada via `SDL_SetTextureColorMod()`, permitindo diferenciar visualmente os tanques durante o jogo multiplayer.

### Sistema de Colisões
O jogo utiliza detecção de colisões baseada em retângulos (`SDL_Rect`) para interações entre:
- Tanques e paredes
- Projéteis e objetos
- Jogadores e power-ups
- Tanques entre si

## 📚 Documentação Adicional

Para gerar documentação completa do código usando Doxygen:

```bash
make doc
```

A documentação será gerada no diretório `doc/` e pode ser visualizada abrindo `doc/html/index.html` em um navegador.

## 🐛 Solução de Problemas

### O `play.cmd` fica travado ou a janela fica cinza (Windows)
1. O suporte gráfico do WSL (WSLg) às vezes trava e passa a abrir janelas
   cinzas ou invisíveis. Reinicie o WSL e abra o jogo:
   ```
   play.cmd --reset
   ```
2. Se continuar, podem faltar ao SDL as bibliotecas X11 ou OpenGL: sem X11 ele
   roda sem janela nenhuma, e sem OpenGL a janela fica cinza e parada. Rode o
   `install.cmd` de novo: ele instala as bibliotecas X11 (`libxext`,
   `libxcursor`, `libxi`, `libxrandr`, `libxfixes`, `libxscrnsaver`) e o OpenGL
   (`mesa-gl`, `mesa-egl`).
3. Por fim, atualize o WSL com `wsl --update`.

### Controles não funcionam
- Pelo `play.cmd` (WSL): o controle precisa estar **no cabo** e autorizado pelo
  `gamepads.cmd` (confira com `gamepads.cmd --list`). O registro do repasse fica
  em `%LOCALAPPDATA%\Tank1990\gamepads.log`
- Controle por Bluetooth não funciona no WSL; use o cabo
- Confira a tabela de distribuição: o 1º controle conectado é sempre do Player 1; no modo duelo, a tela de configuração mostra o dispositivo de cada jogador
- No Linux, o usuário precisa de acesso a `/dev/input/event*` (grupo `input`)
- Certifique-se de que o SDL2 está instalado corretamente

### Erro de compilação
- Verifique se todas as dependências estão instaladas: `make info`
- Limpe o build anterior: `make clean`
- Tente compilar novamente: `make build`

### Recursos não encontrados
- Certifique-se de que o diretório `resources/` existe e contém todos os arquivos necessários
- Execute `make clean && make build` para recopiar os recursos

## 🙌 Créditos

Este projeto é um **fork** do jogo criado por **Krystian Kałużny**:

- **Repositório original:** https://github.com/krystiankaluzny/Tanks
- **Autor original:** Krystian Kałużny ([@krystiankaluzny](https://github.com/krystiankaluzny)) — 2015
- **Licença original:** MIT
- **Descrição original:** *"Implementation of Battle City / Tank 1990. Game was written in C++11 and SDL2 2D graphic library."*

Toda a base do motor do jogo (renderização SDL2, máquina de estados, IA dos inimigos,
sistema de bônus, colisões e os 35 níveis) vem desse trabalho. Os créditos são dele.

### O que foi feito neste fork

Mantido por **Gabryel Lima** ([@Gabryel-lima](https://github.com/Gabryel-lima)):

- Correção de diversos bugs do projeto base (ver [FIXES.md](FIXES.md))
- Suporte a **3 e 4 jogadores** (o original ia até 2)
- **Cores únicas por jogador**, com posições iniciais e controles próprios
- Suporte a **controles USB / gamepads**, para todos os jogadores, junto com o teclado, com hotplug
- Sistema de **sons** (`SoundManager`)
- Comentários e documentação traduzidos para português
- Build multiplataforma: `Makefile` (Linux/macOS/MSYS2) e instalador via WSL no Windows

## 📄 Licença

Projeto educacional baseado no clássico Tank 1990 (Battle City).
O código herdado do projeto original permanece sob a **licença MIT** de Krystian Kałużny.

## 🙏 Contribuições

Contribuições são bem-vindas! Sinta-se à vontade para:
- Reportar bugs
- Sugerir melhorias
- Enviar pull requests
- Melhorar a documentação
