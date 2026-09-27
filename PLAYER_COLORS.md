# Player Colors and Controls - Tank 1990

> 🇧🇷 [Versão em português](CORES_JOGADORES.md)

Each player has a unique color, a distinct start position and a dedicated
control setup, so tanks are easy to tell apart in multiplayer games.

## Player setup

### Player 1 - Golden yellow
- **Color**: RGB (255, 215, 0)
- **Input**: physical controller 0 (analog sticks + X button)
- **Position**: bottom left (128, 384)

### Player 2 - Green
- **Color**: RGB (0, 255, 0)
- **Input**: keyboard arrow keys + Space
- **Position**: bottom right (256, 384)

### Player 3 - Blue
- **Color**: RGB (0, 100, 255)
- **Input**: physical controller 1 (D-pad + X button)
- **Position**: top left (128, 320)

### Player 4 - Red
- **Color**: RGB (255, 50, 50)
- **Input**: physical controller 2 (D-pad + X button)
- **Position**: top right (256, 320)

## How it works

### 1. Colored rendering
- The renderer has a `drawObjectWithColor()` method
- Sprites are tinted with `SDL_SetTextureColorMod`
- The texture's original color is restored after drawing, so other objects are
  not affected

### 2. Color support in `Object`
- The base `Object` class has a `color` field
- `draw()` uses the color when one is set
- The default color, white (255, 255, 255, 255), leaves the original sprites
  unchanged

### 3. Per-player colors
- `getPlayerColor(int player_index)` returns the predefined color for a player
- `setPlayerColor(SDL_Color color)` applies a color
- The constructor applies the color automatically from the player index

### 4. Shield color
- The shield automatically inherits the player's color
- `setFlag()` is overridden to keep them in sync
- The color is applied both when the shield is created and when it is
  reactivated

## Usage

Colors are applied automatically when players are created. No configuration is
needed: the game knows which player is being created (from its index) and
applies the matching color.

To change a color, edit `getPlayerColor()`.

## Benefits

1. **Visual identity**: each player has a unique look
2. **Better gameplay**: it is easy to see which tank belongs to whom
3. **Compatibility**: the original sprites still work unchanged
4. **Flexibility**: colors are defined in a single method

## Recent changes (2024)

### Controls
- **Player 2**: uses the keyboard arrow keys (↑↓←→ + Space)
- **Player 3**: fixed the physical controller mapping (now uses controller 1)
- **No logs**: the input system runs without debug messages
- **Fixed setup**: players are no longer switched to the keyboard automatically

### Positions
- Start positions were rearranged so players don't overlap
- Players start in the corners for better visibility

### New constructor
`Player(keys, player_idx, controller_idx)` separates:
- `player_idx`: sets the player's color and position
- `controller_idx`: sets which physical controller to use

## Files involved

- `src/appconfig.cpp` - control and position settings
- `src/app_state/game.cpp` - player creation
- `src/objects/player.h` and `src/objects/player.cpp` - constructor and input logic
- `src/engine/renderer.h` and `src/engine/renderer.cpp` - colored rendering
- `src/objects/object.h` and `src/objects/object.cpp` - color support
