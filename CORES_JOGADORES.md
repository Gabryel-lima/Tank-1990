# Sistema de Cores e Controles - Tank 1990

> 🇬🇧 [English version](PLAYER_COLORS.md)

## Implementação Concluída

Foi implementado um sistema completo de cores e controles para diferenciar os jogadores no jogo Tank 1990. Cada jogador tem uma cor única, posição inicial distinta e configuração de controle específica.

## Configuração dos Jogadores

### Jogador 1 - Amarelo Dourado
- **Cor**: RGB (255, 215, 0)
- **Controle**: Teclado `W` `A` `S` `D` + `Espaço`, e/ou um gamepad
- **Posição**: Canto inferior esquerdo (128, 384)

### Jogador 2 - Verde  
- **Cor**: RGB (0, 255, 0)
- **Controle**: Teclado setas + `Ctrl direito` (`Alt direito` no Mac), e/ou um gamepad
- **Posição**: Canto inferior direito (256, 384)

### Jogador 3 - Azul
- **Cor**: RGB (0, 100, 255)
- **Controle**: Gamepad (sem teclado)
- **Posição**: Canto superior esquerdo (128, 320)

### Jogador 4 - Vermelho
- **Cor**: RGB (255, 50, 50)
- **Controle**: Gamepad (sem teclado)
- **Posição**: Canto superior direito (256, 320)

## Funcionalidades Implementadas

### 1. Sistema de Renderização com Cores
- Adicionado método `drawObjectWithColor()` no renderer
- Suporte a colorização de sprites via SDL_SetTextureColorMod
- Preservação da cor original da textura após renderização

### 2. Suporte a Cores na Classe Object
- Campo `color` adicionado à classe base Object
- Método `draw()` atualizado para usar cores quando necessário
- Cor padrão branca (255, 255, 255, 255) não altera sprites originais

### 3. Cores Específicas para Jogadores
- Método `getPlayerColor(int player_index)` para obter cores predefinidas
- Método `setPlayerColor(SDL_Color color)` para aplicar cores
- Cores aplicadas automaticamente no construtor baseado no índice do jogador

### 4. Sincronização de Cores com Escudo
- Escudo herda automaticamente a cor do jogador
- Método `setFlag()` sobrescrito para manter sincronização
- Cores aplicadas tanto na criação quanto na reativação do escudo

## Como Usar

As cores são aplicadas automaticamente quando os jogadores são criados. Não é necessária nenhuma configuração adicional - o sistema identifica automaticamente qual jogador está sendo criado (através do índice) e aplica a cor correspondente.

## Benefícios

1. **Diferenciação Visual**: Cada jogador agora tem uma identidade visual única
2. **Melhor Jogabilidade**: Mais fácil identificar qual tanque pertence a cada jogador
3. **Compatibilidade**: Sistema mantém compatibilidade com sprites originais
4. **Flexibilidade**: Cores podem ser facilmente alteradas modificando o método `getPlayerColor()`

## Mudanças Recentes (2024)

### Controles
- **Jogador 2**: atira com `Ctrl direito`; antes usava `Espaço`, a mesma tecla
  do Jogador 1, e os dois tanques atiravam juntos
- **Gamepads**: distribuídos pela classe `Controllers` (`src/controllers.cpp`),
  com hotplug; a tabela de distribuição está no [README](README.md#-controles)

### Posições Reorganizadas
- Posições dos jogadores reorganizadas para evitar sobreposição
- Distribuição em cantos para melhor visibilidade

### Construtor
- `Player(keys, idx)`: `idx` define cor, posição e qual gamepad o jogador recebe

## Arquivos Modificados

- `src/appconfig.cpp` - Configurações de controles e posições
- `src/app_state/game.cpp` - Criação de jogadores com novos parâmetros
- `src/objects/player.h` e `src/objects/player.cpp` - Construtor e leitura de teclado/gamepad
- `src/controllers.h` e `src/controllers.cpp` - Distribuição dos gamepads
- `src/engine/renderer.h` e `src/engine/renderer.cpp` - Sistema de cores
- `src/objects/object.h` e `src/objects/object.cpp` - Suporte a cores

O sistema está otimizado e funcionando corretamente!
