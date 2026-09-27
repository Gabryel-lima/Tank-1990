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
- ✅ **Sistema de detecção de controles**: Identifica controles conectados sem troca automática
- ✅ **Analógicos para Player 1**: Suporte completo aos analógicos do controle
- ✅ **Controles dedicados**: Cada jogador tem seu próprio controle físico
- ✅ **Sistema de pontuação** com bônus por eliminação de inimigos
- ✅ **8 tipos de power-ups** com efeitos únicos
- ✅ **Sistema de estrelas** (0-3 níveis) que melhora o tanque
- ✅ **4 tipos de tanques inimigos** com comportamentos diferentes
- ✅ **Efeitos sonoros** e visuais
- ✅ **Validação robusta** de controles conectados
- ✅ **Sistema de vidas** e respawn
- ✅ **Proteção da base** (águia) com paredes de pedra

## 🎯 Power-ups e Bônus

O jogo possui 8 tipos diferentes de power-ups que aparecem aleatoriamente quando você destrói tanques inimigos:

| Power-up | Efeito |
|----------|--------|
| **⭐ Estrela** | Aumenta o nível de estrela em 1 (máximo 3). Melhora velocidade e poder de fogo |
| **💣 Granada** | Destrói todos os inimigos no mapa instantaneamente |
| **🛡️ Capacete** | Concede escudo temporário que protege contra danos |
| **⏰ Relógio** | Congela todos os inimigos por um período |
| **⛏️ Pá** | Protege a base (águia) com paredes de pedra indestrutíveis |
| **🚗 Tanque** | Adiciona uma vida extra ao jogador |
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

### Player 1
- **Controle Físico 0**: Analógicos para movimento, botão X para atirar
- **Cor**: Amarelo Dourado (RGB: 255, 215, 0)
- **Posição Inicial**: Canto inferior esquerdo (128, 384)

### Player 2
- **Teclado Setas**: Setas direcionais para movimento, Space para atirar
- **Cor**: Verde (RGB: 0, 255, 0)
- **Posição Inicial**: Canto inferior direito (256, 384)

### Player 3
- **Controle Físico 1**: D-pad para movimento, botão X para atirar
- **Cor**: Azul (RGB: 0, 100, 255)
- **Posição Inicial**: Canto superior esquerdo (128, 320)

### Player 4
- **Controle Físico 2**: D-pad para movimento, botão X para atirar
- **Cor**: Vermelho (RGB: 255, 50, 50)
- **Posição Inicial**: Canto superior direito (256, 320)

## 🎮 Sistema de Controles

O jogo possui um sistema avançado de detecção e configuração de controles:

### Detecção de Controles
- Verifica quantos controles estão conectados usando `SDL_NumJoysticks()`
- Valida se cada dispositivo é um game controller com `SDL_IsGameController()`
- Atribui controles físicos específicos a cada jogador
- Sistema robusto que não falha se controles não estiverem conectados

### Configuração Fixa
- **Player 1**: Sempre usa controle físico 0 (analógicos)
- **Player 2**: Sempre usa teclado (setas direcionais)
- **Player 3**: Sempre usa controle físico 1 (D-pad)
- **Player 4**: Sempre usa controle físico 2 (D-pad)

### Mapeamento de Controles
- **Controle 0** → Player 1 (analógicos + X)
- **Controle 1** → Player 3 (D-pad + X)
- **Controle 2** → Player 4 (D-pad + X)
- **Teclado Setas** → Player 2 (dedicado)

### Tipos de Input Suportados
O sistema suporta três tipos de input através do enum `InputType`:

1. **Keyboard**: Apenas teclado
2. **Controller**: Apenas controle físico
3. **Hybrid**: Teclado + controle simultaneamente (teclado tem prioridade)

Exemplo de mapeamento fixo:

```cpp
// Player 1: Controle 0 (Analógicos + X) - Amarelo
// Player 2: Teclado Setas + Space - Verde
// Player 3: Controle 1 (D-pad + X) - Azul
// Player 4: Controle 2 (D-pad + X) - Vermelho
```

## 👥 Como Configurar Múltiplos Jogadores

1. **Conecte os controles** ao computador:
   - Para 2 jogadores: 1 controle (Player 1) + teclado (Player 2)
   - Para 3 jogadores: 2 controles (Players 1 e 3) + teclado (Player 2)
   - Para 4 jogadores: 3 controles (Players 1, 3 e 4) + teclado (Player 2)
2. **Execute o jogo** e selecione quantos jogadores no menu
3. **Configuração fixa**:
   - Player 1: Controle físico 0 (analógicos) - Amarelo
   - Player 2: Teclado (setas direcionais) - Verde
   - Player 3: Controle físico 1 (D-pad) - Azul
   - Player 4: Controle físico 2 (D-pad) - Vermelho
4. **Cada jogador mantém sua configuração** independentemente dos controles disponíveis

**Nota**: O jogo detecta automaticamente os controles conectados. Se um controle não estiver disponível, o jogador correspondente não será criado.

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
│   │   ├── tank.h/cpp    # Classe base dos tanques
│   │   ├── bullet.h/cpp  # Projéteis
│   │   ├── bonus.h/cpp   # Power-ups
│   │   ├── brick.h/cpp   # Paredes de tijolo
│   │   └── eagle.h/cpp   # Base (águia)
│   ├── app_state/        # Estados da aplicação
│   │   ├── menu.h/cpp    # Menu principal
│   │   ├── game.h/cpp    # Lógica principal do jogo
│   │   └── scores.h/cpp  # Tela de pontuação
│   ├── engine/           # Motor do jogo
│   │   ├── renderer.h/cpp    # Sistema de renderização
│   │   ├── engine.h/cpp      # Motor principal
│   │   └── spriteconfig.h/cpp # Configuração de sprites
│   ├── app.h/cpp         # Aplicação principal
│   ├── appconfig.h/cpp   # Configurações globais
│   ├── soundmanager.h/cpp # Gerenciador de áudio
│   └── type.h            # Definições de tipos
├── resources/            # Recursos do jogo
│   ├── img/              # Imagens e sprites
│   ├── sound/            # Efeitos sonoros
│   ├── font/             # Fontes do jogo
│   └── levels/           # Arquivos dos 36 níveis
├── build/                # Arquivos de build (gerado)
├── Makefile              # Sistema de build
└── README.md             # Este arquivo
```

## 🔍 Como Funciona o Sistema de Controles

1. **Detecção**: Verifica quantos joysticks estão conectados usando `SDL_NumJoysticks()`
2. **Validação**: Confirma se cada dispositivo é um game controller válido com `SDL_IsGameController()`
3. **Mapeamento Fixo**: Cada jogador tem um controle físico específico atribuído
4. **Sem Troca Automática**: Configurações mantidas independentemente da disponibilidade
5. **Operação Robusta**: O jogo funciona mesmo se controles não estiverem conectados

## 🕹️ Suporte aos Analógicos (Player 1)

O Player 1 possui suporte completo aos analógicos do controle:

### Características:
- **Eixo Y (Vertical)**: Movimento para cima/baixo
- **Eixo X (Horizontal)**: Movimento para esquerda/direita
- **Deadzone**: 8192 para controle puro, 6144 para input híbrido
- **Detecção Automática**: O sistema detecta automaticamente se são analógicos ou botões
- **Compatibilidade**: Funciona com qualquer controle compatível com SDL2

### Como Funciona:
1. **Detecção**: Valores negativos nos eixos indicam analógicos
2. **Leitura**: `SDL_GameControllerGetAxis()` lê os valores dos analógicos
3. **Processamento**: Aplica deadzone e converte para direções do jogo
4. **Prioridade**: Teclado tem prioridade sobre analógicos no modo híbrido

### Configuração Técnica:
```cpp
// Player 1: Analógicos configurados (controle físico 0)
SDL_CONTROLLER_AXIS_LEFTY, -1, SDL_CONTROLLER_AXIS_LEFTX, -1, SDL_CONTROLLER_BUTTON_X

// Player 2: Teclado dedicado
SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE

// Player 3: D-pad (controle físico 1)
SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 
SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_X

// Player 4: D-pad (controle físico 2)
SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_X
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
- Verifique se os controles estão conectados antes de iniciar o jogo
- No Linux, pode ser necessário configurar permissões para `/dev/input/js*`
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
- Suporte a **controles USB / gamepads**, incluindo modo híbrido teclado + controle
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
