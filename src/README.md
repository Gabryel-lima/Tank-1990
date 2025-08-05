# Tanks
Projekt Battle City / Tank 1990 na PJC

## Configuração de Input Híbrido

O jogo agora suporta input híbrido para o Player 1, permitindo usar tanto teclado quanto controle simultaneamente.

### Como funciona:

1. **Player 1**: Configurado com input híbrido (teclado WASD + Space + controle 0)
2. **Player 2**: Usa apenas controle 1
3. **Player 3**: Usa apenas controle 2  
4. **Player 4**: Usa apenas teclado

### Configuração atual no `appconfig.cpp`:

```cpp
// Jogador 1: Input híbrido (teclado WASD + Space + controle 0)
v.push_back(Player::PlayerKeys(SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_SPACE,
                               SDL_CONTROLLER_AXIS_LEFTY, -1, SDL_CONTROLLER_AXIS_LEFTX, -1, SDL_CONTROLLER_BUTTON_X));
```

### Tipos de Input Suportados:

- **Keyboard**: Apenas teclado
- **Controller**: Apenas controle
- **Hybrid**: Teclado + controle simultaneamente

### Como modificar a configuração:

Para alterar o input do Player 1 para apenas teclado:
```cpp
v.push_back(Player::PlayerKeys(SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_SPACE));
```

Para alterar para apenas controle:
```cpp
v.push_back(Player::PlayerKeys(SDL_CONTROLLER_AXIS_LEFTY, -1, SDL_CONTROLLER_AXIS_LEFTX, -1, SDL_CONTROLLER_BUTTON_X));
```

### Controles por Jogador:

- **Player 1**: WASD + Space (teclado) + Analógico esquerdo + X (controle)
- **Player 2**: Analógico esquerdo + X (controle)
- **Player 3**: Analógico esquerdo + X (controle)  
- **Player 4**: WASD + Space (teclado)
