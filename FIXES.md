# FIXES.md — Bugs encontrados e pendências

Levantamento feito a partir de uma leitura completa do código-fonte (`src/`).
Serve como lista de trabalho para revisar com calma.

**Status da verificação (última atualização):**

- ✅ Compila e linka no Windows (MinGW-w64 GCC 15 / w64devkit), gerando
  `build/bin/Tanks.exe` — PE x86-64, dependendo apenas de `SDL2.dll`,
  `SDL2_image.dll`, `SDL2_mixer.dll`, `SDL2_ttf.dll`, `KERNEL32`, `msvcrt` e
  `SHELL32` (libstdc++/libgcc entram estáticas, então a pasta `build/bin` é
  autocontida).
- ✅ `-Wall`: **zero avisos**.
- ⚠️ `-Wall -Wextra -Wshadow`: 9 avisos, todos cosméticos (itens **#29** e **#30**).
- ❌ O jogo **não foi executado**. O **Smart App Control** do Windows 11
  (policy `{0283ac0f-...}`, estado de imposição) bloqueia qualquer executável
  compilado localmente, por falta de assinatura digital reconhecida — e não
  tem lista de exceções. Por isso o caminho oficial no Windows passou a ser o
  WSL (`install.cmd`). Os itens de comportamento em tempo de execução abaixo
  vêm de leitura de código, não de teste.

Cada item tem: **onde**, **o que acontece**, **como reproduzir** (quando aplicável)
e **sugestão de correção**.

Legenda de severidade:

| | |
|---|---|
| 🔴 | Quebra o build ou trava/crasha o jogo |
| 🟠 | Bug funcional visível para o jogador, ou vazamento de memória |
| 🟡 | Comportamento estranho, cosmético ou código morto |
| 🔵 | Portabilidade Linux ↔ Windows / avisos do compilador |

---

## ✅ Parte 1 — Já corrigido

Estes já foram aplicados ao código. Estão listados para registro e revisão.

### 1. 🔴🔵 `main()` sem `SDL_main` quebrava a linkagem no Windows

**Onde:** `src/main.cpp`

`main.cpp` só incluía `app.h`, que traz apenas `<SDL2/SDL_events.h>`. Sem
`<SDL2/SDL.h>` (que puxa `SDL_main.h`), a macro `#define main SDL_main` não era
aplicada. No MinGW, o `SDL2main` define `main` **e** `WinMain`, então ao linkar com
`-mwindows -lSDL2main` dá `multiple definition of 'main'` ou
`undefined reference to 'SDL_main'`.

No Linux não acontece nada, porque lá o SDL não redefine `main`.

**Correção aplicada:** `#include <SDL2/SDL.h>` no topo de `main.cpp`.

### 2. 🟠 Vazamento de memória a cada frame no desenho de texto

**Onde:** `Renderer::drawText()` em `src/engine/renderer.cpp`

A `SDL_Surface` criada por `TTF_RenderText_Solid()` nunca era liberada. Como
`drawText` é chamado várias vezes por frame (HUD, vidas, número da fase), o
consumo de memória crescia continuamente durante a partida.

**Correção aplicada:** `SDL_FreeSurface(text_surface)` no fim da função e também
no caminho de erro (`m_text_texture == nullptr`).

### 3. 🔴 Ponteiros e membros não inicializados

| Onde | Membro | Consequência |
|---|---|---|
| `App::App()` — `src/app.cpp` | `m_app_state`, `is_running` | Se `SDL_Init`/som/`IMG_Init`/`TTF_Init` falhasse, o destrutor fazia `delete` num ponteiro indeterminado |
| `Renderer::Renderer()` — `src/engine/renderer.cpp` | `m_font3` | O destrutor lia `m_font3` mesmo sem `loadFont()` ter sido chamado |
| `Player` (3 construtores) — `src/objects/player.cpp` | `m_fire_time` | Cadência do primeiro tiro imprevisível (podia atirar na hora ou travar) |
| `Game(int)` e `Game(vector<Player*>, int)` — `src/app_state/game.cpp` | `m_enemy_redy_time` | Tempo até o primeiro inimigo aparecer era aleatório |
| `Game::update()` — alvo dos inimigos | `SDL_Point target` | Lido sem ter sido escrito quando nenhum alvo ficava abaixo da métrica 832 |
| `Enemy::draw()` — `src/objects/enemy.cpp` | `SDL_Color c` | Cor indeterminada se o tipo não fosse A/B/C/D |

**Correção aplicada:** todos inicializados no construtor / na declaração.

### 4. 🔵 Arquivos de fase em CRLF criavam uma coluna fantasma no Linux

**Onde:** `Game::loadLevel()` em `src/app_state/game.cpp`

Os arquivos em `resources/levels/` estão com terminação **CRLF** na cópia de
trabalho do Windows (o `core.autocrlf=true` converte no checkout).

- No **Windows**, o `std::fstream` em modo texto remove o `\r` → 26 colunas ✔
- No **Linux**, o `\r` fica na string → `line.size() == 27` → o mapa ganha uma
  27ª coluna fantasma fora da área visível.

E se alguém salvar um arquivo de fase com **linha em branco no final**, a linha
vazia virava uma `row` sem colunas e o `m_level.at(j).at(i)` de
"limpar o espaço ao redor da águia" lançava `std::out_of_range` → **crash no
carregamento da fase**.

**Correção aplicada:** remoção de `\r`/`\n` no fim da linha e `continue` em
linhas vazias.

> Vale considerar também um `.gitattributes` com
> `resources/levels/* text eol=lf` para fixar o formato no repositório.

### 5. 🔵 `min` / `abs` sem qualificação e `fabs()` sobre inteiros

**Onde:** `src/engine/renderer.cpp`, `src/objects/enemy.cpp`, `src/app_state/game.cpp`

`min(xs, ys)` e `abs(...)` só compilavam porque `appconfig.h` faz
`using namespace std;` e o `<algorithm>` chegava por include transitivo do
libstdc++. Em outra implementação da biblioteca padrão (ou outra versão do
MinGW) isso falha na compilação.

`fabs()` (double) era usado para calcular distância entre inteiros.

**Correção aplicada:** `std::min` / `std::abs`, com `<algorithm>` e `<cstdlib>`
incluídos explicitamente.

### 6. 🔵 Avisos de "variável possivelmente não inicializada"

**Onde:** `Game::checkCollisionTankWithLevel()` e `Game::checkCollisionBulletWithLevel()`

Os `switch(direction)` não têm `default`, então o g++ com `-Wall` avisa que
`row_start`, `row_end`, `column_start` e `column_end` podem ser usados sem
inicialização.

**Correção aplicada:** inicialização em zero antes do `switch`.

### 7. 🔵 Variável calculada e nunca usada

**Onde:** `Player::adjustInputType()` em `src/objects/player.cpp`

A função contava `available_controllers` e não fazia nada com o valor
(`-Wunused-but-set-variable`). Ver também o item **#12** abaixo.

**Correção aplicada:** corpo simplificado para `(void)player_index;`.

---

## ⚠️ Parte 2 — Bugs a revisar (não corrigidos)

Não mexi nestes porque envolvem decisões de design/jogabilidade.

### 8. 🟠 Jogador 1 e Jogador 2 atiram com a MESMA tecla

**Onde:** `src/appconfig.cpp:65`

```cpp
// Jogador 1: WASD + SPACE
v.push_back(Player::PlayerKeys(SDL_SCANCODE_W, ..., SDL_SCANCODE_SPACE, ...));
// Jogador 2: setas + SPACE   <-- mesma tecla de tiro
v.push_back(Player::PlayerKeys(SDL_SCANCODE_UP, SDL_SCANCODE_DOWN,
                               SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT,
                               SDL_SCANCODE_SPACE));
```

**Reproduzir:** modo 2 jogadores, aperte espaço → os dois tanques atiram juntos.

Repare que `appconfig.cpp:24-30` define as macros `P1_FIRE_KEY` / `P2_FIRE_KEY`
(com `RCTRL`, ou `RALT` no macOS) que **nunca são usadas** — sobra do projeto
original, onde o jogador 2 atirava com Ctrl direito.

**Sugestão:** usar as macros nos dois `push_back`, ou trocar a tecla do jogador 2
para `SDL_SCANCODE_RCTRL`. Lembrar de atualizar `README.md` e
`CORES_JOGADORES.md`.

### 9. 🟠 `Scores` nunca libera os jogadores → vazamento no game over

**Onde:** `src/app_state/scores.cpp:21`, `src/app_state/scores.h`

`Game::nextState()` transfere todos os `Player*` para `Scores`, mas `Scores`
**não tem destrutor**. No caminho de game over (`Scores::nextState()` → `Menu`),
todos os jogadores vazam — junto com o `Object` do escudo e o
`SDL_GameController` aberto de cada um.

No caminho de vitória não vaza, porque os ponteiros são repassados para o
próximo `Game`.

**Sugestão:** dar a `Scores` um destrutor que apague `m_players` **apenas quando
não repassou** os ponteiros ao próximo `Game` (ou usar `std::unique_ptr` /
`shared_ptr` e acabar com a dúvida de quem é o dono).

### 10. 🟠 `m_killed_players` vaza ao sair com ESC

**Onde:** `Game::clearLevel()` em `src/app_state/game.cpp:485`

`clearLevel()` apaga `m_enemies`, `m_players`, `m_bonuses`, `m_bushes`,
`m_level` e `m_eagle`, mas **não** `m_killed_players`.

**Reproduzir:** perca um jogador no modo 2P e aperte ESC → o jogador morto
nunca é liberado.

### 11. 🟡 Som de explosão do jogador toca mesmo com escudo

**Onde:** `Player::destroy()` — `src/objects/player.cpp:445`

```cpp
SoundManager::getInstance().playSound("player_exp");
if(testFlag(TSF_SHIELD)) return;   // <-- checagem vem DEPOIS do som
```

**Reproduzir:** leve um tiro logo após renascer (escudo ativo) → som de explosão
sem explosão.

**Sugestão:** mover o `playSound` para depois das checagens de escudo/barco.

### 12. 🟡 `adjustInputType()` não faz nada

**Onde:** `src/objects/player.cpp`

A função é chamada nos construtores e em todo frame do `Player::update()`
(só para o jogador de índice 1), mas o corpo é vazio — a troca automática de
teclado/controle foi desativada em algum momento. O `Player::update()` ainda faz
a chamada por frame.

**Sugestão:** implementar de verdade ou remover as chamadas.

### 13. 🟡 Som de "tiro em aço" nunca toca

**Onde:** `Brick::bulletHit()` — `src/objects/brick.cpp:31`

```cpp
if (type == ST_STONE_WALL) { ...playSound("steelhit"); }
```

`Brick` é sempre construído com `ST_BRICK_WALL`; as paredes de pedra são
`Object` puro (`game.cpp`, caso `'@'`) e nem chegam a chamar `bulletHit()`.
A condição é sempre falsa → `steelhit.ogg` é código morto.

Vale conferir também os outros sons carregados e nunca tocados em
`SoundManager::loadSounds()`: `tbonushit`, `pause`, `fexplosion` e — hoje —
`life` e `shieldhit`, cujos métodos (`Player::addLife()`, `Player::shieldHit()`)
não são chamados por ninguém.

### 14. 🟡 Tanque-ponteiro do menu "pula" 2 pixels

**Onde:** `src/app_state/menu.cpp:27` (construtor) vs. linhas 98/108/136/144

O construtor posiciona em `(index + 1) * 32 + 112` e todos os eventos usam
`+ 110`. Na primeira vez que você mexe na seleção, o tanque salta 2 px.

### 15. 🟡 Barco dos jogadores 3 e 4 sai com a cor do jogador 2

**Onde:** `Tank::setFlag()` — `src/objects/tank.cpp:372`

```cpp
m_boat = new Object(pos_x, pos_y, type == ST_PLAYER_1 ? ST_BOAT_P1 : ST_BOAT_P2);
```

Só existem dois sprites de barco. Jogadores 3 e 4 pegam o sprite do jogador 2.
Como o `Object` do barco não recebe o `color` do jogador (diferente do escudo,
em `Player::setFlag()`), o barco também não fica na cor certa.

**Sugestão:** aplicar `m_boat->color = color;` em `Player::setFlag(TSF_BOAT)`,
igual ao que já é feito com o escudo.

### 16. 🟡 Limite de tiros cresce sem teto

**Onde:** `Player::changeStarCountBy()` — `src/objects/player.cpp:491`

```cpp
if(star_count >= 2 && c > 0) m_bullet_max_size++;
else m_bullet_max_size = 2;
```

Cada estrela pega com `star_count >= 2` incrementa o limite, sem teto — pegando
várias estrelas seguidas dá para encher a tela de tiros. E o `else` fixa `2`,
ignorando `AppConfig::player_bullet_max_size` (que vale `1`), então esse
parâmetro de configuração não tem efeito nenhum.

### 17. 🟡 Tecla `B` na fase 1 leva para a fase "0"

**Onde:** `Game::eventProcess()` (`game.cpp:378`) e `Game::nextLevel()` (`game.cpp:913`)

`B` faz `m_current_level -= 2` e o `nextLevel()` soma 1. Estando na fase 1:
`1 - 2 + 1 = 0`. O teste é `if(m_current_level < 0) m_current_level = 35;`,
então o zero passa e o jogo carrega `resources/levels/0` mostrando "STAGE 0".

**Sugestão:** trocar para `if(m_current_level < 1) m_current_level = 35;`.

### 18. 🟡 `m_enemy_to_kill` pode ficar negativo

**Onde:** bônus granada em `Game::checkCollisionPlayerWithBonus()`

A granada faz `m_enemy_to_kill--` para cada inimigo em tela; as balas já em voo
que atingem inimigos também decrementam. O contador pode passar de zero para
baixo. Hoje não quebra nada (`m_enemy_to_kill <= 0` encerra a fase e o HUD não
desenha nada para valores negativos), mas é frágil.

### 19. 🟡 `Object::update()` não checa `m_sprite`

**Onde:** `src/objects/object.cpp:103`

`draw()` protege com `if(m_sprite == nullptr) return;`, mas `update()`
desreferencia direto. O construtor padrão `Object()` deixa `m_sprite = nullptr`,
então qualquer uso desse construtor causa segfault no primeiro `update()`.

### 20. 🟡 `SoundManager` opera sobre ponteiros nulos

**Onde:** `src/soundmanager.cpp:54` e `:59`

Se um `.ogg`/`.wav` não carregar (caminho errado, `SDL2_mixer` sem suporte a
Vorbis), o mapa guarda `nullptr`. `loadSounds()` avisa no `stderr`, mas
`setVolume()` e `cleanup()` passam os nulos adiante para `Mix_VolumeChunk` /
`Mix_FreeChunk`.

**Sugestão:** `if (!chunk) continue;` nos dois laços.

### 21. 🟠 Recursos são procurados a partir do diretório atual

**Onde:** `src/appconfig.cpp` (`texture.png`, `levels/`, `prstartk.ttf`) e
`src/soundmanager.cpp` (`resources/sound/...`)

Todos os caminhos são relativos ao *working directory*, não ao executável.
Funciona com `make run` e com duplo clique no `.exe` (o Explorer usa a pasta do
programa como cwd), mas falha se o jogo for chamado de outra pasta — por
exemplo `./build/bin/Tanks` a partir da raiz do projeto: janela abre preta, sem
sons e sem fases.

**Sugestão:** montar os caminhos a partir de `SDL_GetBasePath()` no início do
`App::run()`.

### 22. 🟡 Dois controles de FPS ao mesmo tempo

**Onde:** `App::run()` — `src/app.cpp`

O renderer é criado com `SDL_RENDERER_PRESENTVSYNC` e o loop ainda faz um
`SDL_Delay(delay)` com ajuste dinâmico mirando 60 FPS. Num monitor de 144 Hz o
comportamento fica dependente do driver.

**Sugestão:** escolher um dos dois (VSync é o mais simples).

### 23. 🟡 Saídas antecipadas em `App::run()` pulam a limpeza

**Onde:** `src/app.cpp`

`if (!SoundManager::getInstance().init()) return;`, o `return` do
`SDL_CreateWindow` nulo, do `IMG_Init` e do `TTF_Init` saem da função sem chamar
`SDL_Quit()` nem destruir a janela. Sem impacto prático (o processo morre logo
depois), mas atrapalha se um dia o `App` virar reutilizável — e o jogo fecha
**sem nenhuma mensagem** quando o `texture.png` ou o áudio não carregam.

**Sugestão:** ao menos um `SDL_ShowSimpleMessageBox` com o erro antes de sair.

### 24. 🟡 `drawObjectWithColor()` não restaura o alpha corretamente

**Onde:** `src/engine/renderer.cpp`

```cpp
SDL_SetTextureColorMod(m_texture, color.r, color.g, color.b);
if(color.a != 255) SDL_SetTextureAlphaMod(m_texture, color.a);
```

O alpha só é **aplicado** quando `!= 255`, mas é sempre **restaurado** no fim.
Se uma chamada com `a != 255` fosse seguida de outra com `a == 255`, a segunda
herdaria o alpha da primeira. Hoje nenhuma cor de jogador usa alpha diferente de
255, então está latente.

### 25. 🟡 `while(!level.eof())` é um padrão frágil

**Onde:** `Game::loadLevel()`

O idiomático é `while(std::getline(level, line))`. Com a correção do item **#4**
o sintoma sumiu, mas o laço continua lendo uma vez a mais no fim do arquivo.

Na mesma função: se `level.is_open()` for falso, o jogo segue com `m_level`
vazio, `m_level_rows_count == 0`, e cria a águia em
`(m_level_rows_count - 2) * 16 == -32`. Não há nenhuma mensagem de erro.

### 26. 🟡 Construtor `Player(double, double, SpriteType, int)` inconsistente

**Onde:** `src/objects/player.cpp`

Esse construtor lê `player_keys.type` antes de `player_keys` receber qualquer
configuração (fica no `PlayerKeys()` padrão, teclado com todas as teclas
`SDL_SCANCODE_UNKNOWN`) e não chama `adjustInputType()`. Aparentemente não é
usado em lugar nenhum — vale remover.

### 27. 🟡 Comentários desatualizados

- `src/appconfig.cpp`: as posições iniciais dizem "canto inferior esquerdo",
  "canto superior direito" etc., mas as quatro posições ficam na faixa de baixo
  do mapa (y = 384 e y = 320).
- `src/app_state/game.h`: `m_player_count` documentado como "1 ou 2" (hoje 1 a 4).
- `src/appconfig.h`: `player_starting_point` documentado como "as duas posições
  iniciais dos jogadores (Player 1, Player 2 e Player 3)".
- `src/engine/spriteconfig.cpp`: jogador 3 reaproveita o sprite do jogador 2 e o
  jogador 4 o do jogador 1 — só a cor os diferencia. Se dois jogadores da mesma
  "dupla de sprite" ficarem lado a lado, dá para confundir.

### 28. 🟡 `Doxyfile` e `make clean` discordam

`Doxyfile` tem `OUTPUT_DIRECTORY = build/doc`, mas o alvo `clean` do `Makefile`
remove `doc/` (na raiz). Como o `clean` também remove `build/`, na prática
funciona — mas o `.gitignore` e a mensagem "Documentação gerada em doc/" estão
desalinhados com o Doxyfile.

> Observação: o alvo `doc` chamava `doxywizard Doxyfile && doxygen` (abre a GUI).
> Isso foi trocado por `doxygen Doxyfile` na atualização do build.

### 29. 🔵 Parâmetros de construtor sombreiam membros (`-Wshadow`)

Cinco construtores recebem um parâmetro chamado `type`, que tem o mesmo nome do
membro `Object::type`:

```
src/objects/object.cpp:20   Object::Object(double x, double y, SpriteType type)
src/objects/tank.cpp:22     Tank::Tank(double x, double y, SpriteType type)
src/objects/player.cpp:75   Player::Player(double x, double y, SpriteType type, int idx)
src/objects/enemy.cpp:39    Enemy::Enemy(double x, double y, SpriteType type)
src/objects/bonus.cpp:13    Bonus::Bonus(double x, double y, SpriteType type)
```

Hoje funciona (`Object` usa `this->type = type;` e os derivados repassam ao
construtor da base), mas é fácil escrever `type = ...` achando que está mexendo
no membro. Em `Enemy::Enemy(double, double, SpriteType type)` o construtor já
compara `if(type == ST_TANK_B)` — que aqui lê o *parâmetro*, e por sorte tem o
mesmo valor do membro.

Tem também um `j` interno sombreando o `j` das linhas em
`Game::loadLevel()` (`src/app_state/game.cpp:451`, laço que limpa o espaço ao
redor da águia).

**Sugestão:** renomear para `sprite_type` / `row`.

### 30. 🔵 Parâmetros não utilizados (`-Wunused-parameter`)

- `main(int argc, char* args[])` — o SDL não passa nada adiante; marcar com
  `(void)argc; (void)args;` se quiser silenciar.
- `Brick::update(Uint32 dt)` — corpo vazio de propósito (o tijolo não anima).

---

## 🔍 Parte 3 — Como continuar a investigação

Sugestões de ferramentas:

```bash
# 1. Compilar com avisos agressivos (Linux)
make clean
make build CFLAGS="-c -Wall -Wextra -Wshadow -Wconversion -std=c++17"

# 2. Vazamentos e leituras inválidas
valgrind --leak-check=full --track-origins=yes ./build/bin/Tanks

# 3. Sanitizers (mais rápido que o valgrind)
make clean
make build CFLAGS="-c -Wall -std=c++17 -fsanitize=address,undefined -g" \
           LFLAGS="-fsanitize=address,undefined -g"

# 4. Análise estática
cppcheck --enable=all --inconclusive --std=c++17 src/
clang-tidy src/**/*.cpp -- -std=c++17 -Isrc
```

No Windows (MinGW), `-fsanitize=address` está disponível nas builds recentes do
MSYS2; o Valgrind não roda. A alternativa é
[Dr. Memory](https://drmemory.org/).
