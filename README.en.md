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

**4. Controllers (optional)**

To play with a USB controller, plug it in and run **`gamepads.cmd`** once
(see [Controllers on Windows](#controllers-on-windows-wsl)).

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
- ✅ **Gamepads for every player**: D-pad or analog stick, with hotplug
- ✅ **Scoring** with bonuses for destroying enemies
- ✅ **8 power-up types** with unique effects
- ✅ **Star system** (levels 0-3) that upgrades the tank
- ✅ **4 enemy tank types** with different behavior
- ✅ **Sound and visual effects**
- ✅ **Lives and respawn**
- ✅ **Base protection** (eagle) with stone walls

## ⚔️ Duel Mode (Extra Modes)

From the main menu, **Extra Modes → Duel Mode** starts the multiplayer team mode, human players only: each team defends its own eagle and tries to destroy the other one. Team **A** (yellow) spawns at the bottom and team **B** (green) at the top, on a dedicated map (`resources/duel_levels/1`) mirrored both horizontally and vertically so both sides get the same terrain.

**Formats:** `1 vs 1`, `2 vs 2` or `Custom Teams`, where you pick 2 to 4 players and each one's team (2 vs 1, 3 vs 1...). No bots take player slots; since the game supports up to 4 players (P1 and P2 on the keyboard, P3 and P4 on controllers), there is no 3 vs 3 or 4 vs 4.

| In the setup screen | Key / controller |
|---------------------|------------------|
| Change option | ↑ ↓ / D-pad / stick |
| Change value | ← → / D-pad / stick |
| Confirm | Enter, Space / A, Start |
| Back | Esc / B, Back |

**Rules:**
- A round is won by destroying the enemy base or eliminating every enemy player; the match goes to the first team to win **2 rounds**.
- Each player has **3 lives** and respawns with a short shield.
- **Reinforcement (tank power-up):** brings an **allied bot** in your team's color, with one life. The first one guards the base, the second attacks; with 2 of your team's reinforcements on the field, the power-up becomes an extra life. Reinforcements don't keep a round alive: if every player on a team is out, that team loses.
- **No friendly fire:** bullets don't hurt teammates, your own base or the wall around it.
- **Power-ups:** after 10 s with no power-up on the map, one appears at a symmetric spot in the middle (same distance from both bases). If a team is far behind on lives, it appears on that team's side instead. Only players can pick them up (reinforcements can't).
- Duel effects: **grenade** destroys the enemy tanks on the field (shields protect); **clock** pins the enemy team in place for 4 s (humans can still turn and shoot); **shovel** fortifies **your** base with stone; **gun** breaks stone (the answer to the shovel). Grenade and gun are the rarest.
- **Uneven teams:** the smaller team gets more lives per player (1 vs 3: 6 lives vs 3) and, if the other team has twice as many players or more, a stone wall around its base.
- Enter / Start pauses; Esc / Back leaves the match. Leaving, or pressing fire / Enter / A when the match ends, takes you back to the duel setup, ready for a rematch.

## 🎯 Power-ups

Eight power-ups appear at random when you destroy enemy tanks:

| Power-up | Effect |
|----------|--------|
| **⭐ Star** | Raises the star level by 1 (max 3). Improves speed and firepower |
| **💣 Grenade** | Instantly destroys every enemy on the map |
| **🛡️ Helmet** | Grants a temporary shield against damage |
| **⏰ Clock** | Freezes every enemy for a while |
| **⛏️ Shovel** | Surrounds the base (eagle) with indestructible stone walls |
| **🚗 Tank** | Gives the player an extra life (in duel mode, brings an allied reinforcement bot) |
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

| Player | Color | Keyboard | Start position |
|--------|-------|----------|----------------|
| **Player 1** | Yellow | `W` `A` `S` `D` + `Space` | Bottom left |
| **Player 2** | Green | Arrow keys + `Right Ctrl` (`Right Alt` on Mac) | Bottom right |
| **Player 3** | Blue | — (controller only) | Top left |
| **Player 4** | Red | — (controller only) | Top right |

Each player has their own keys: pressing one player's key never moves or
fires another player's tank.

**On a controller (gamepad)**, for any player:

- **Move**: D-pad or left stick
- **Fire**: any face button (A, B, X or Y)
- **Start**: pause · **Back/Select**: back to the menu
- **In the menu**: D-pad or stick to choose, A/Start to confirm, B/Back to quit

Players 1 and 2 can use keyboard and controller at the same time.

### Which controller goes to which player

Controllers are handed out in the order they were connected: first to the
players without a keyboard (3 and 4), then to players 1 and 2.

| Game | 1st controller | 2nd controller | 3rd controller | 4th controller |
|------|----------------|----------------|----------------|----------------|
| 1 or 2 players | Player 1 | Player 2 | — | — |
| 3 players | Player 3 | Player 1 | Player 2 | — |
| 4 players | Player 3 | Player 4 | Player 1 | Player 2 |

So a 3-player game needs just **1 controller** (for Player 3) plus the
keyboard for the other two; a 4-player game needs **2 controllers**.

Controllers can be plugged in or removed while the game is running. Any
controller SDL2 recognizes as a *game controller* works (Xbox, PlayStation,
Switch Pro and most generic ones).

### Controllers on Windows (WSL)

WSL can't see USB controllers on its own. The game uses
[usbipd-win](https://github.com/dorssel/usbipd-win) to lend the controller to
WSL while it runs:

1. Plug the controllers in **with a USB cable** and run **`gamepads.cmd`**
   once. It installs usbipd-win (if needed) and authorizes each connected
   controller model. It asks for administrator permission.
2. Play through `play.cmd` as usual. It forwards the authorized controllers
   when the game opens (including ones plugged in later) and **hands them
   back to Windows** when the game closes. While the game is open, the
   controller doesn't work in other Windows programs.

You only need to run `gamepads.cmd` again for a new controller **model**.
Other commands: `gamepads.cmd --list` (show authorized controllers) and
`gamepads.cmd --remove` (remove the authorizations).

| Controller | Works in WSL? |
|---|---|
| PlayStation (DualShock 4, DualSense), Switch Pro, 8BitDo, generic USB | Yes |
| Xbox 360 / One / Series (cable) | Yes, through the libusb-enabled SDL2 that `install.cmd` builds |
| Any **Bluetooth** controller | No (usbipd only forwards USB) |

`gamepads.cmd` only authorizes devices Windows identifies as a
gamepad/joystick (or an Xbox controller); keyboards and mice are never
forwarded.

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
│   │   ├── bot.h/cpp     # Allied bot (reinforcement power-up) in duel mode
│   │   ├── tank.h/cpp    # Tank base class
│   │   ├── bullet.h/cpp  # Bullets
│   │   ├── bonus.h/cpp   # Power-ups
│   │   ├── brick.h/cpp   # Brick walls
│   │   └── eagle.h/cpp   # Base (eagle)
│   ├── app_state/        # Application states
│   │   ├── menu.h/cpp    # Main menu
│   │   ├── game.h/cpp    # Main game logic
│   │   ├── duel.h/cpp    # Duel (team) mode
│   │   └── scores.h/cpp  # Score screen
│   ├── engine/           # Game engine
│   │   ├── renderer.h/cpp     # Rendering
│   │   ├── engine.h/cpp       # Core engine
│   │   └── spriteconfig.h/cpp # Sprite configuration
│   ├── app.h/cpp         # Main application
│   ├── appconfig.h/cpp   # Global settings (including player keys)
│   ├── controllers.h/cpp # Gamepads: hotplug and player assignment
│   ├── soundmanager.h/cpp # Audio manager
│   └── type.h            # Type definitions
├── resources/            # Game assets
│   ├── img/              # Images and sprites
│   ├── sound/            # Sound effects
│   ├── font/             # Fonts
│   ├── levels/           # The 36 level files
│   └── duel_levels/      # Duel mode maps
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
- Through `play.cmd` (WSL): the controller must be **on a cable** and
  authorized by `gamepads.cmd` (check with `gamepads.cmd --list`). The
  forwarding log is at `%LOCALAPPDATA%\Tank1990\gamepads.log`
- Bluetooth controllers don't work in WSL; use the cable
- Check the assignment table: in a 3-player game the 1st controller belongs to Player 3
- On Linux your user needs access to `/dev/input/event*` (the `input` group)
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
- **USB controller / gamepad** support, usable by every player alongside the keyboard, with hotplug
- **Sound** system (`SoundManager`)
- Comments and documentation translated to Portuguese
- Cross-platform build: `Makefile` (Linux/macOS/MSYS2) and a WSL installer for Windows

## 📄 License

Educational project based on the classic Tank 1990 (Battle City).
Code inherited from the original project remains under Krystian Kałużny's **MIT license**.

## 🙏 Contributing

Contributions are welcome: bug reports, suggestions, pull requests and
documentation improvements.
