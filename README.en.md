# Tank 1990 - C++ Implementation

> 🇧🇷 [Versão em português](README.md)

A clone of the classic Tank 1990 (Battle City) written in C++ with SDL2. A
strategic action game where you drive a tank and must protect your base while
destroying every enemy.

## 🚀 Building and Running

### Windows

On Windows the game runs inside **WSL** (Windows Subsystem for Linux), in a
minimal Alpine Linux distribution. The game window opens on your desktop like
any other program.

> **Why WSL instead of an `.exe`?**
> Windows 11 **Smart App Control** blocks locally compiled executables because
> they have no recognized digital signature. There is no exception list, and
> turning it off is irreversible (it only comes back by reinstalling Windows).
> WSL works around this without touching the machine's security settings.

**1. Install the WSL platform** (first time only)

Open the Terminal **as administrator** and run:

```powershell
wsl --install --no-distribution
```

Restart the computer. `--no-distribution` installs only the platform, without
Ubuntu — the game uses Alpine, which is much smaller.

**2. Install the game**

Double-click **`install.cmd`** (or run it from a terminal, no admin needed):

```
install.cmd
```

It downloads Alpine Linux (~3.5 MB), installs SDL2 and the compiler, and
compiles the game. Every step shows a progress bar with elapsed time and an
estimate of the time remaining. The compiler stays installed so you can
rebuild; only `uninstall.cmd` removes it.

**3. Play**

```
play.cmd
```

**To uninstall:**

```
uninstall.cmd
```

The uninstaller asks what to remove:

| Option | What it deletes | Frees |
|---|---|---|
| 1 | The game, the compiler and the libraries (keeps Alpine in WSL) | ~350 MB |
| 2 | The game and the Alpine distribution | ~400 MB |
| 3 | Everything, including the WSL platform | ~1.8 GB |

**Disk space**

| Item | Size |
|---|---|
| WSL platform (once, shared by everything) | ~1.5 GB |
| Alpine + SDL2 + compiler + game | ~400 MB |
| Same, installed with `install.cmd --slim` (no compiler) | ~250 MB |

### Linux / macOS

```bash
# 1. Install dependencies
make install-deps

# 2. Build and run the game
make run
```

### Available commands (Linux, macOS and inside WSL)

```bash
make build        # Builds the whole project
make run          # Builds and runs the game
make clean        # Removes build files
make info         # Shows system information
make doc          # Generates documentation (Doxygen)
make install-deps # Installs dependencies (apk, apt, dnf or brew)
make help         # Lists every available command
```

### Rebuilding after changing the code (Windows/WSL)

The compiler stays installed after the installation. To apply your changes to
the game that `play.cmd` opens, run the installer again (it reuses the
compiler and only rebuilds):

```
install.cmd
```

To test without installing, build and run straight from the project folder:

```bat
wsl -d Tank1990 --cd "%CD%" -- make run
```

If you don't plan to change the code and want to save ~150 MB, install with
`install.cmd --slim`, which removes the compiler at the end.

### Dependencies

- **SDL2** - Main graphics library
- **SDL2_image** - Image loading
- **SDL2_mixer** - Audio
- **SDL2_ttf** - Font rendering
- **g++** - C++ compiler (C++17 support)

### Installing dependencies manually

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
```

**macOS (Homebrew):**
```bash
brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf
```

**Windows (WSL / Alpine):**

`install.cmd` handles this on its own. To do it by hand, inside the
distribution:

```sh
apk add --no-cache g++ make sdl2-dev sdl2_image-dev sdl2_mixer-dev sdl2_ttf-dev mesa-dri-gallium \
    libxext libxcursor libxi libxrandr libxfixes libxscrnsaver mesa-gl mesa-egl
make run
```

**Windows (native `.exe`, optional):**

The `Makefile` can still build an `.exe` with MinGW-w64 from MSYS2 or Git Bash.
SDL2 is searched for in `$SDL2_DIR`, `third_party/SDL2/<arch>-w64-mingw32`,
`$MINGW_HOME`, `resources/SDL/` and `pkg-config`, in that order:

```bash
make build                    # produces build/bin/Tanks.exe
make build ARCH=i686          # 32-bit
make build WIN_CONSOLE=1      # keeps the console open (debugging)
```

Keep in mind this `.exe` will most likely be **blocked by Smart App Control**
when run — see the Windows section above.

## 🎮 Features

- ✅ **36 levels** with increasing difficulty
- ✅ **1-4 simultaneous players**
- ✅ **Unique color** for each player
- ✅ **Dedicated, conflict-free controls** for each player
- ✅ **Controller detection**: finds connected controllers without automatic swapping
- ✅ **Analog sticks for Player 1**
- ✅ **Dedicated controllers**: each player has their own physical controller
- ✅ **Scoring** with bonuses for destroying enemies
- ✅ **8 power-up types** with unique effects
- ✅ **Star system** (levels 0-3) that upgrades the tank
- ✅ **4 enemy tank types** with different behavior
- ✅ **Sound and visual effects**
- ✅ **Robust validation** of connected controllers
- ✅ **Lives and respawn**
- ✅ **Base protection** (eagle) with stone walls

## 🎯 Power-ups

Eight power-ups appear at random when you destroy enemy tanks:

| Power-up | Effect |
|----------|--------|
| **⭐ Star** | Raises the star level by 1 (max 3). Improves speed and firepower |
| **💣 Grenade** | Instantly destroys every enemy on the map |
| **🛡️ Helmet** | Grants a temporary shield against damage |
| **⏰ Clock** | Freezes every enemy for a while |
| **⛏️ Shovel** | Surrounds the base (eagle) with indestructible stone walls |
| **🚗 Tank** | Gives the player an extra life |
| **🔫 Gun** | Raises the star level by 3 (maximum) |
| **🚤 Boat** | Lets the tank cross water |

### Star system

Stars (levels 0-3) progressively upgrade the tank:

- **0 stars**: default speed and firepower, at most 2 bullets at once
- **1 star**: 30% faster movement, 30% faster bullets
- **2 stars**: faster movement, at most 3 bullets at once
- **3 stars**: faster movement, at most 3 bullets, bullets deal extra damage

**Note**: with 3 stars, getting hit costs you only 1 star. With fewer than 3,
you lose all of them when destroyed.

## 🎮 Controls

| Player | Input | Color | Start position |
|---|---|---|---|
| **1** | Physical controller 0: analog sticks to move, X to fire | Golden yellow (255, 215, 0) | Bottom left (128, 384) |
| **2** | Keyboard: arrow keys to move, Space to fire | Green (0, 255, 0) | Bottom right (256, 384) |
| **3** | Physical controller 1: D-pad to move, X to fire | Blue (0, 100, 255) | Top left (128, 320) |
| **4** | Physical controller 2: D-pad to move, X to fire | Red (255, 50, 50) | Top right (256, 320) |

Details on how colors are applied are in [PLAYER_COLORS.md](PLAYER_COLORS.md).

### Controller detection

- Counts connected devices with `SDL_NumJoysticks()`
- Checks that each device is a game controller with `SDL_IsGameController()`
- Assigns a specific physical controller to each player
- Keeps working if controllers are not connected

### Supported input types

The `InputType` enum supports three modes:

1. **Keyboard**: keyboard only
2. **Controller**: physical controller only
3. **Hybrid**: keyboard and controller together (keyboard takes priority)

## 👥 Setting up multiple players

1. **Connect the controllers**:
   - 2 players: 1 controller (Player 1) + keyboard (Player 2)
   - 3 players: 2 controllers (Players 1 and 3) + keyboard (Player 2)
   - 4 players: 3 controllers (Players 1, 3 and 4) + keyboard (Player 2)
2. **Start the game** and pick the number of players in the menu
3. Each player keeps their fixed setup (see the table above) regardless of
   which controllers are available

**Note**: the game detects connected controllers automatically. If a
controller is missing, the matching player is not created.

## 🕹️ Analog sticks (Player 1)

- **Y axis**: up/down
- **X axis**: left/right
- **Deadzone**: 8192 for controller-only input, 6144 for hybrid input
- Values are read with `SDL_GameControllerGetAxis()`, filtered by the deadzone
  and turned into game directions
- In hybrid mode the keyboard takes priority over the sticks

```cpp
// Player 1: analog sticks (physical controller 0)
SDL_CONTROLLER_AXIS_LEFTY, -1, SDL_CONTROLLER_AXIS_LEFTX, -1, SDL_CONTROLLER_BUTTON_X

// Player 2: dedicated keyboard
SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE

// Player 3: D-pad (physical controller 1)
SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_X

// Player 4: D-pad (physical controller 2)
SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_X
```

## 👾 Enemies

- **Type A tank**: basic enemy
- **Type B tank**: intermediate enemy
- **Type C tank**: advanced enemy
- **Type D tank**: elite enemy

Each type has its own movement pattern, speed and combat behavior.

## 🗺️ Map elements

- **🧱 Brick wall**: destroyed by bullets
- **🪨 Stone wall**: indestructible, blocks bullets and tanks
- **💧 Water**: impassable (except with the Boat power-up)
- **🌿 Bush**: hides tanks but does not block bullets
- **🧊 Ice**: makes tanks slide
- **🦅 Eagle**: the player's base, which must be protected

## 📁 Project structure

```
Tank-1990/
├── src/
│   ├── objects/          # Game objects
│   │   ├── player.h/cpp  # Player-controlled tank
│   │   ├── enemy.h/cpp   # Enemy tanks
│   │   ├── tank.h/cpp    # Tank base class
│   │   ├── bullet.h/cpp  # Bullets
│   │   ├── bonus.h/cpp   # Power-ups
│   │   ├── brick.h/cpp   # Brick walls
│   │   └── eagle.h/cpp   # Base (eagle)
│   ├── app_state/        # Application states
│   │   ├── menu.h/cpp    # Main menu
│   │   ├── game.h/cpp    # Main game logic
│   │   └── scores.h/cpp  # Score screen
│   ├── engine/           # Game engine
│   │   ├── renderer.h/cpp     # Rendering
│   │   ├── engine.h/cpp       # Core engine
│   │   └── spriteconfig.h/cpp # Sprite configuration
│   ├── app.h/cpp         # Main application
│   ├── appconfig.h/cpp   # Global settings
│   ├── soundmanager.h/cpp # Audio manager
│   └── type.h            # Type definitions
├── resources/            # Game assets
│   ├── img/              # Images and sprites
│   ├── sound/            # Sound effects
│   ├── font/             # Fonts
│   └── levels/           # The 36 level files
├── tools/                # WSL install/uninstall scripts
├── install.cmd           # Windows installer (WSL)
├── play.cmd              # Starts the game on Windows
├── uninstall.cmd         # Windows uninstaller
├── Makefile              # Build system
└── README.md             # Portuguese README
```

## 🎯 Goal

Protect your base (eagle) while destroying every enemy tank in each level.
You lose if:
- the eagle is destroyed
- you lose all your lives

You advance to the next level when every enemy is destroyed, you still have at
least one life and the eagle is intact.

## 🔧 Technologies

- **C++17**
- **SDL2**, **SDL2_image**, **SDL2_mixer**, **SDL2_ttf**
- **Make**
- **Doxygen** (optional, for documentation)

## 📝 Development notes

### Architecture
- **State machine**: game states (Menu, Game, Scores) managed through `AppState`
- **Inheritance**: base classes (`Object`, `Tank`) with specializations (`Player`, `Enemy`)
- **Singleton**: `SoundManager` and `Renderer`
- **Central configuration**: `AppConfig` holds every game constant

### Colors
Each player gets a unique color applied with `SDL_SetTextureColorMod()`, so
tanks are easy to tell apart in multiplayer. See [PLAYER_COLORS.md](PLAYER_COLORS.md).

### Collisions
Collision detection uses rectangles (`SDL_Rect`) for tank/wall, bullet/object,
player/power-up and tank/tank interactions.

## 📚 Documentation

To generate the full code documentation with Doxygen:

```bash
make doc
```

Open `doc/html/index.html` in a browser.

## 🐛 Troubleshooting

### `play.cmd` hangs, or the window stays gray (Windows)
1. WSL's graphics support (WSLg) sometimes gets stuck and starts opening gray
   or invisible windows. Restart WSL and open the game:
   ```
   play.cmd --reset
   ```
2. If that doesn't help, SDL may be missing the X11 or OpenGL libraries:
   without X11 it runs with no window at all, and without OpenGL the window
   stays gray and frozen. Run `install.cmd` again: it installs the X11
   libraries (`libxext`, `libxcursor`, `libxi`, `libxrandr`, `libxfixes`,
   `libxscrnsaver`) and OpenGL (`mesa-gl`, `mesa-egl`).
3. Finally, update WSL with `wsl --update`.

### Controllers don't work
- Connect the controllers before starting the game
- On Linux you may need permissions for `/dev/input/js*`
- Make sure SDL2 is installed correctly

### Build errors
- Check the dependencies: `make info`
- Clean the previous build: `make clean`
- Build again: `make build`

### Missing resources
- Make sure `resources/` exists and has every file
- Run `make clean && make build` to copy the resources again

## 🙌 Credits

This project is a **fork** of the game by **Krystian Kałużny**:

- **Original repository:** https://github.com/krystiankaluzny/Tanks
- **Original author:** Krystian Kałużny ([@krystiankaluzny](https://github.com/krystiankaluzny)) — 2015
- **Original license:** MIT
- **Original description:** *"Implementation of Battle City / Tank 1990. Game was written in C++11 and SDL2 2D graphic library."*

The whole engine (SDL2 rendering, state machine, enemy AI, bonuses, collisions
and the 35 levels) comes from that work. The credit is his.

### What this fork adds

Maintained by **Gabryel Lima** ([@Gabryel-lima](https://github.com/Gabryel-lima)):

- Fixes for many bugs in the base project (see [FIXES.md](FIXES.md), in Portuguese)
- Support for **3 and 4 players** (the original supported 2)
- **Unique player colors**, with their own start positions and controls
- **USB controller / gamepad** support, including hybrid keyboard + controller mode
- **Sound** system (`SoundManager`)
- Comments and documentation translated to Portuguese
- Cross-platform build: `Makefile` (Linux/macOS/MSYS2) and a WSL installer for Windows

## 📄 License

Educational project based on the classic Tank 1990 (Battle City).
Code inherited from the original project remains under Krystian Kałużny's **MIT license**.

## 🙏 Contributing

Contributions are welcome: bug reports, suggestions, pull requests and
documentation improvements.
