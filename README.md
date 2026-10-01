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

## ⚔️ Modo Duelo (Extra Modes)

No menu principal, **Extra Modes → Duel Mode** abre o modo multijogador por equipes, só entre jogadores humanos: cada equipe defende a sua águia e tenta destruir a do adversário. A equipe **A** (amarela) nasce embaixo e a **B** (verde) em cima, num mapa próprio (`resources/duel_levels/1`), espelhado na horizontal e na vertical para que os dois lados tenham o mesmo terreno.

**Formatos:** `1 vs 1`, `2 vs 2` ou `Custom Teams`, em que você escolhe de 2 a 4 jogadores e a equipe de cada um (2 contra 1, 3 contra 1...). Não há bots ocupando vagas; como o jogo aceita até 4 jogadores (P1 e P2 no teclado, P3 e P4 no controle), não existem 3 vs 3 nem 4 vs 4.

| Na configuração | Tecla / controle |
|-----------------|------------------|
| Mudar de opção | ↑ ↓ / D-pad / analógico |
| Mudar o valor | ← → / D-pad / analógico |
| Confirmar | Enter, Espaço / A, Start |
| Voltar | Esc / B, Back |

**Regras:**
- Vence a rodada quem destruir a base inimiga ou eliminar todos os jogadores inimigos; vence a partida quem ganhar **2 rodadas**.
- Cada jogador tem **3 vidas** e renasce com escudo por alguns segundos.
- **Reforço (bônus de tanque):** traz um **bot aliado** da cor da sua equipe, com uma vida. O primeiro reforço guarda a base, o segundo ataca; com 2 reforços da equipe em campo, o bônus vira vida extra. O reforço não segura a rodada: se todos os jogadores da equipe caírem, ela perde.
- **Sem fogo amigo:** tiros não ferem aliados, a própria base nem a muralha em volta dela.
- **Bônus:** com o mapa vazio de bônus por 10 s, surge um num ponto simétrico do meio do mapa (à mesma distância das duas bases). Se uma equipe estiver bem atrás em vidas, o bônus surge do lado dela. Só jogadores coletam (reforços não).
- Efeitos no duelo: **granada** destrói os inimigos em campo (escudo protege); **relógio** imobiliza a equipe inimiga por 4 s (humanos ainda giram e atiram); **pá** reforça a **sua** base com pedra; **canhão** quebra pedra (resposta à pá). Granada e canhão são os mais raros.
- **Equipes de tamanhos diferentes:** a menor recebe mais vidas por jogador (1 contra 3: 6 vidas contra 3) e, se a outra tiver o dobro de jogadores ou mais, base com muralha de pedra.
- Enter / Start pausa; Esc / Back abandona a partida. Ao sair ou no fim da partida (tiro / Enter / A), o jogo volta para a configuração do duelo, pronto para a revanche.

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

- **0 estrelas**: Velocidade e poder de fogo padrão, máximo de 2 projéteis simultâneos
- **1 estrela**: Velocidade aumentada em 30%, projéteis 30% mais rápidos
- **2 estrelas**: Velocidade aumentada, máximo de 3 projéteis simultâneos
- **3 estrelas**: Velocidade aumentada, máximo de 3 projéteis, projéteis causam dano extra

**Nota**: Se você tem 3 estrelas e é atingido, perde apenas 1 estrela. Com menos de 3 estrelas, perde todas as estrelas ao ser destruído.

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
- **Start**: pausa · **Back/Select**: volta ao menu
- **No menu**: D-pad ou analógico para escolher, A/Start para confirmar, B/Back para sair

**No teclado** há dois layouts: `WASD` (`W` `A` `S` `D` + `Espaço`) e
`ARROWS` (setas + `Ctrl direito`; `Alt direito` no Mac). Cada layout controla
um único jogador: apertar a tecla de um nunca move ou faz atirar outro.

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
| Qualquer controle por **Bluetooth** | Não (o usbipd só repassa USB) |

O `gamepads.cmd` só autoriza dispositivos que o Windows identifica como
gamepad/joystick (ou controle Xbox); teclado e mouse nunca são repassados.

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
│   │   ├── tank.h/cpp    # Classe base dos tanques
│   │   ├── bullet.h/cpp  # Projéteis
│   │   ├── bonus.h/cpp   # Power-ups
│   │   ├── brick.h/cpp   # Paredes de tijolo
│   │   └── eagle.h/cpp   # Base (águia)
│   ├── app_state/        # Estados da aplicação
│   │   ├── menu.h/cpp    # Menu principal
│   │   ├── game.h/cpp    # Lógica principal do jogo
│   │   ├── duel.h/cpp    # Modo duelo (equipes)
│   │   └── scores.h/cpp  # Tela de pontuação
│   ├── engine/           # Motor do jogo
│   │   ├── renderer.h/cpp    # Sistema de renderização
│   │   ├── engine.h/cpp      # Motor principal
│   │   └── spriteconfig.h/cpp # Configuração de sprites
│   ├── app.h/cpp         # Aplicação principal
│   ├── appconfig.h/cpp   # Configurações globais (inclui os layouts de teclado)
│   ├── controllers.h/cpp # Gamepads: hotplug e distribuição entre jogadores
│   ├── soundmanager.h/cpp # Gerenciador de áudio
│   └── type.h            # Definições de tipos
├── resources/            # Recursos do jogo
│   ├── img/              # Imagens e sprites
│   ├── sound/            # Efeitos sonoros
│   ├── font/             # Fontes do jogo
│   ├── levels/           # Arquivos dos 36 níveis
│   └── duel_levels/      # Mapas do modo duelo
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
