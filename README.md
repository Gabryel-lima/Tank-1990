# Tank 1990 - Implementação em C++

Este é um clone do clássico jogo Tank 1990 (Battle City) implementado em C++ usando SDL2.

## Funcionalidades

- Jogo completo com múltiplos níveis (36 níveis)
- Suporte a 1-4 jogadores
- Controles configuráveis (teclado e gamepad)
- **Controle adaptativo**: Player 2 pode usar WASD + Space automaticamente quando Player 1 estiver usando controle
- **Analógicos para Player 1**: Suporte completo aos analógicos do controle para movimento suave
- Sistema de pontuação
- Power-ups e bônus
- Efeitos sonoros
- D-pad digital para controles (Players 2-4)

## Controles

### Player 1
- **Teclado**: WASD para movimento, Space para atirar
- **Controle**: Analógicos para movimento, X para atirar
- **Híbrido**: Ambos simultaneamente (teclado tem prioridade)
- **Deadzone**: 8192 para controle puro, 6144 para input híbrido

### Player 2
- **Teclado**: WASD para movimento, Space para atirar
- **Controle**: D-pad para movimento, X para atirar
- **Híbrido**: Ambos simultaneamente, com prioridade para teclado quando Player 1 usa controle

### Player 3
- **Controle**: D-pad para movimento, X para atirar

### Player 4
- **Controle**: D-pad para movimento, X para atirar

## Controle Adaptativo

O jogo detecta automaticamente quando o Player 1 está usando controle e ajusta o comportamento do Player 2:

- Se Player 1 estiver usando controle, Player 2 pode usar teclado (WASD + Space) sem conflitos
- O sistema prioriza o teclado para Player 2 quando há conflito de controles
- Funciona em tempo real, detectando mudanças durante o jogo
- Permite que dois jogadores usem o mesmo teclado quando necessário

## Configuração Atual

```cpp
// Player 1: Híbrido (WASD + Space + Analógicos + X)
// Player 2: Híbrido (WASD + Space + D-pad + X) - adaptativo
// Player 3: Controle (D-pad + X)
// Player 4: Controle (D-pad + X)
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

### Como Usar os Analógicos (Player 1):

1. **Conecte um controle** ao computador
2. **Execute o jogo** e selecione "1 Player" no menu
3. **Use o analógico esquerdo** para mover o tanque:
   - **Para cima**: Empurre o analógico para cima
   - **Para baixo**: Empurre o analógico para baixo
   - **Para esquerda**: Empurre o analógico para a esquerda
   - **Para direita**: Empurre o analógico para a direita
4. **Use o botão X** para atirar
5. **Alternativamente**: Use WASD + Space no teclado (tem prioridade)

## Estrutura do Projeto

- `src/objects/` - Classes dos objetos do jogo (Player, Enemy, Tank, etc.)
- `src/app_state/` - Estados da aplicação (Menu, Game, Scores)
- `src/engine/` - Motor do jogo (Renderer, Engine)
- `resources/` - Recursos do jogo (imagens, sons, níveis)

## Funcionalidades Implementadas

- ✅ Detecção automática de controles conectados
- ✅ Ajuste dinâmico de input baseado na disponibilidade de controles
- ✅ Priorização inteligente de teclado vs controle
- ✅ **Suporte completo aos analógicos do Player 1**
- ✅ Suporte completo ao D-pad digital (Players 2-4)
- ✅ Deadzone configurável para analógicos (8192/6144)
- ✅ Funciona em tempo real durante o jogo
- ✅ Sistema de power-ups e estrelas
- ✅ Múltiplos níveis com dificuldade progressiva
- ✅ Efeitos sonoros e visuais

## Como Funciona o Controle Adaptativo

1. **Detecção**: O sistema verifica se o Player 1 está usando controle
2. **Ajuste**: Se Player 1 usa controle, Player 2 pode usar teclado sem conflitos
3. **Priorização**: O teclado tem prioridade sobre o controle para Player 2
4. **Tempo Real**: Ajustes são feitos dinamicamente durante o jogo

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
// Player 1: Analógicos configurados
SDL_CONTROLLER_AXIS_LEFTY, -1, SDL_CONTROLLER_AXIS_LEFTX, -1, SDL_CONTROLLER_BUTTON_X
```

## Tecnologias Utilizadas

- **C++**: Linguagem principal
- **SDL2**: Biblioteca para gráficos, áudio e input
- **SDL2_image**: Suporte a imagens
- **SDL2_ttf**: Renderização de fontes
- **Make**: Sistema de build
