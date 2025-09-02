# Tank 1990 - Implementação em C++

Este é um clone do clássico jogo Tank 1990 (Battle City) implementado em C++ usando SDL2.

## Funcionalidades

- Jogo completo com múltiplos níveis (36 níveis)
- Suporte a 1-4 jogadores simultâneos
- Controles exclusivos e sem conflitos para cada jogador
- **Sistema inteligente de controles**: Detecção automática e fallback para teclado
- **Analógicos para Player 1**: Suporte completo aos analógicos do controle
- **Controles dedicados**: Cada jogador tem seu próprio controle físico
- Sistema de pontuação
- Power-ups e bônus
- Efeitos sonoros
- Validação robusta de controles conectados

## Controles

### Player 1
- **Controle Físico 0**: Analógicos para movimento, X para atirar
- **Fallback**: WASD + Space (se controle não disponível)

### Player 2
- **Teclado Dedicado**: Setas direcionais (↑ ↓ ← →) para movimento, Space para atirar
- **Sempre usa teclado**: Configurado exclusivamente para teclado

### Player 3
- **Controle Físico 2**: D-pad para movimento, X para atirar
- **Fallback**: Setas direcionais + Right Shift (se controle não disponível)

### Player 4
- **Controle Físico 3**: D-pad para movimento, X para atirar
- **Fallback**: Teclado numérico (8456 + Enter) (se controle não disponível)

## Sistema Inteligente de Controles

O jogo possui um sistema robusto de detecção e atribuição de controles:

### Detecção Automática
- Verifica quantos controles estão conectados usando `SDL_NumJoysticks()`
- Valida se cada dispositivo é um game controller com `SDL_IsGameController()`
- Atribui controles físicos sequencialmente aos jogadores

### Fallback Inteligente
- **Controles suficientes**: Cada jogador usa seu controle dedicado
- **Controles insuficientes**: Jogadores sem controle usam teclas exclusivas do teclado
- **Player 2 sempre teclado**: Configurado para usar teclado independentemente

### Teclas de Fallback por Jogador
- **Player 1**: W A S D + Space
- **Player 2**: ↑ ↓ ← → + Space (dedicado)
- **Player 3**: ↑ ↓ ← → + Right Shift
- **Player 4**: Numpad 8 4 5 6 + Enter

## Configuração Atual

```cpp
// Player 1: Controle 0 (Analógicos + X)
// Player 2: Teclado (Setas + Space) - dedicado
// Player 3: Controle 2 (D-pad + X)
// Player 4: Controle 3 (D-pad + X)
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

1. **Conecte os controles** ao computador (até 4 controles para 4 jogadores, sendo que Player 2 usa teclado)
2. **Execute o jogo** e selecione quantos jogadores no menu
3. **Configuração automática**:
   - Player 1 usa o primeiro controle (analógicos)
   - Player 2 sempre usa teclado (Setas + Space)
   - Player 3 usa o terceiro controle (D-pad)
   - Player 4 usa o quarto controle (D-pad)
4. **Se não há controles suficientes**: Jogadores sem controle usam teclas exclusivas
5. **Mensagens de debug**: O console mostra quais controles foram detectados

## Estrutura do Projeto

- `src/objects/` - Classes dos objetos do jogo (Player, Enemy, Tank, etc.)
- `src/app_state/` - Estados da aplicação (Menu, Game, Scores)
- `src/engine/` - Motor do jogo (Renderer, Engine)
- `resources/` - Recursos do jogo (imagens, sons, níveis)

## Funcionalidades Implementadas

- ✅ **Detecção automática de controles conectados**
- ✅ **Validação robusta com SDL_NumJoysticks() e SDL_IsGameController()**
- ✅ **Atribuição exclusiva de controles físicos por jogador**
- ✅ **Sistema de fallback inteligente para teclado**
- ✅ **Suporte completo aos analógicos do Player 1**
- ✅ **D-pad digital para Players 3-4**
- ✅ **Teclado dedicado para Player 2**
- ✅ **Teclas exclusivas para cada jogador (sem conflitos)**
- ✅ **Mensagens de debug informativas**
- ✅ **Sistema de power-ups e estrelas**
- ✅ **Múltiplos níveis com dificuldade progressiva**
- ✅ **Efeitos sonoros e visuais**

## Como Funciona a Detecção de Controles

1. **Inicialização**: Verifica quantos joysticks estão conectados
2. **Validação**: Confirma se cada dispositivo é um game controller válido
3. **Atribuição**: Cada jogador recebe um controle físico específico
4. **Fallback**: Jogadores sem controle usam teclas exclusivas do teclado
5. **Debug**: Console mostra informações sobre controles detectados

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
