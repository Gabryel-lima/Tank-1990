# Tank 1990 - Implementação em C++

Este é um clone do clássico jogo Tank 1990 (Battle City) implementado em C++ usando SDL2.

## Funcionalidades

- Jogo completo com múltiplos níveis (36 níveis)
- Suporte a 1-4 jogadores simultâneos
- Controles exclusivos e sem conflitos para cada jogador
- **Sistema de detecção de controles**: Identifica controles conectados sem troca automática
- **Analógicos para Player 1**: Suporte completo aos analógicos do controle
- **Controles dedicados**: Cada jogador tem seu próprio controle físico
- Sistema de pontuação
- Power-ups e bônus
- Efeitos sonoros
- Validação robusta de controles conectados

## Controles

### Player 1
- **Controle Físico 0**: Analógicos para movimento, X para atirar
- **Cor**: Amarelo Dourado
- **Posição Inicial**: Canto inferior esquerdo (128, 384)

### Player 2
- **Teclado WASD**: W/A/S/D para movimento, Space para atirar
- **Cor**: Verde
- **Posição Inicial**: Canto inferior direito (256, 384)

### Player 3
- **Controle Físico 1**: D-pad para movimento, X para atirar
- **Cor**: Azul
- **Posição Inicial**: Canto superior esquerdo (128, 320)

### Player 4
- **Controle Físico 2**: D-pad para movimento, X para atirar
- **Cor**: Vermelho
- **Posição Inicial**: Canto superior direito (256, 320)

## Sistema de Controles

O jogo possui um sistema de detecção e configuração de controles:

### Detecção de Controles
- Verifica quantos controles estão conectados usando `SDL_NumJoysticks()`
- Valida se cada dispositivo é um game controller com `SDL_IsGameController()`
- Atribui controles físicos específicos a cada jogador

### Configuração Fixa
- **Player 1**: Sempre usa controle físico 0 (analógicos)
- **Player 2**: Sempre usa teclado WASD
- **Player 3**: Sempre usa controle físico 1 (D-pad)
- **Player 4**: Sempre usa controle físico 2 (D-pad)

### Mapeamento de Controles
- **Controle 0** → Player 1 (analógicos + X)
- **Controle 1** → Player 3 (D-pad + X)
- **Controle 2** → Player 4 (D-pad + X)
- **Teclado WASD** → Player 2 (dedicado)

## Configuração Atual

```cpp
// Player 1: Controle 0 (Analógicos + X) - Amarelo
// Player 2: Teclado WASD + Space - Verde
// Player 3: Controle 1 (D-pad + X) - Azul
// Player 4: Controle 2 (D-pad + X) - Vermelho
```

## Compilação

```bash
make clean
make
```

## Execução

```bash
./build/bin/Tanks
```

### Como Configurar Múltiplos Jogadores:

1. **Conecte os controles** ao computador:
   - Para 2 jogadores: 1 controle (Player 1) + teclado (Player 2)
   - Para 3 jogadores: 2 controles (Players 1 e 3) + teclado (Player 2)
   - Para 4 jogadores: 3 controles (Players 1, 3 e 4) + teclado (Player 2)
2. **Execute o jogo** e selecione quantos jogadores no menu
3. **Configuração fixa**:
   - Player 1: Controle físico 0 (analógicos) - Amarelo
   - Player 2: Teclado WASD - Verde
   - Player 3: Controle físico 1 (D-pad) - Azul
   - Player 4: Controle físico 2 (D-pad) - Vermelho
4. **Cada jogador mantém sua configuração** independentemente dos controles disponíveis

## Estrutura do Projeto

- `src/objects/` - Classes dos objetos do jogo (Player, Enemy, Tank, etc.)
- `src/app_state/` - Estados da aplicação (Menu, Game, Scores)
- `src/engine/` - Motor do jogo (Renderer, Engine)
- `resources/` - Recursos do jogo (imagens, sons, níveis)

## Funcionalidades Implementadas

- ✅ **Sistema de cores únicas para cada jogador**
- ✅ **Posições iniciais distintas e organizadas**
- ✅ **Detecção de controles conectados**
- ✅ **Validação robusta com SDL_NumJoysticks() e SDL_IsGameController()**
- ✅ **Mapeamento fixo de controles por jogador**
- ✅ **Suporte completo aos analógicos do Player 1**
- ✅ **D-pad digital para Players 3-4**
- ✅ **Teclado WASD dedicado para Player 2**
- ✅ **Construtor com índices separados (jogador vs controle físico)**
- ✅ **Sistema de power-ups e estrelas**
- ✅ **Múltiplos níveis com dificuldade progressiva**
- ✅ **Efeitos sonoros e visuais**

## Como Funciona o Sistema de Controles

1. **Detecção**: Verifica quantos joysticks estão conectados
2. **Validação**: Confirma se cada dispositivo é um game controller válido
3. **Mapeamento Fixo**: Cada jogador tem um controle físico específico atribuído
4. **Sem Troca Automática**: Configurações mantidas independentemente da disponibilidade
5. **Operação Silenciosa**: Sem logs desnecessários no console

## Suporte aos Analógicos (Player 1)

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

// Player 3: D-pad (controle físico 2)
SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 
SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_X

// Player 4: D-pad (controle físico 3)
SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_X
```

## Tecnologias Utilizadas

- **C++**: Linguagem principal
- **SDL2**: Biblioteca para gráficos, áudio e input
- **SDL2_image**: Suporte a imagens
- **SDL2_ttf**: Renderização de fontes
- **Make**: Sistema de build
