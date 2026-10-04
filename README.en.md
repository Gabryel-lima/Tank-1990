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
make sprites      # Paints the pixel art from tools/sprites/ into resources/png/texture.png
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
- ✅ **Extra modes**: team duel (10 maps) and wave survival (1 to 4 players)

## ⚔️ Duel Mode (Extra Modes)

From the main menu, **Extra Modes → Duel Mode** starts the multiplayer team mode, human players only: each team defends its own eagle and tries to destroy the other one. Team **A** (yellow) spawns at the bottom and team **B** (green) at the top, on dedicated maps, separate from the campaign (`resources/duel_levels/`), all mirrored both horizontally and vertically so both sides get the same terrain.

**Formats:** `1 vs 1`, `2 vs 2` or `Custom Teams`, where you pick 2 to 4 players and each one's team (2 vs 1, 3 vs 1...). No bots take player slots; since the game supports up to 4 players (see [Controls](#-controls)), there is no 3 vs 3 or 4 vs 4.

**Maps:** once the teams are set, **Next** opens the map selection, with a thumbnail of the highlighted map. If the list doesn't fit on the screen, it scrolls with the selection (arrows on the right show there are more items above or below):

| Map | Style |
|-----|-------|
| **Arena** | Balanced: bricks, side rivers (the boat helps) and ice in the middle |
| **Fortress** | Defensive: stone columns and a stone band across the middle |
| **River** | A river cuts through the middle, with three bridges |
| **Maze** | Brick maze: you can shoot your way through |
| **Open Field** | Open and fast: bushes for ambushes, and ice |
| **Crossroads** | Wide avenues crossing between brick and bush blocks |
| **Archipelago** | Water islands linked by ice bridges; the boat opens shortcuts |
| **Bunkers** | Scattered stone bunkers, with narrow corridors to the bases |
| **Frozen Lake** | An ice lake in the middle, ringed with bushes: hard to stop and aim |
| **Gauntlet** | A central stone gate: whoever goes through meets the opponent |
| **Random** | A random map each round |

**Adding a new map** (no code changes):

1. Save a **26×26** grid in `resources/duel_levels/` using the level symbols: `#` brick, `@` stone, `~` water, `%` bush, `-` ice, `.` empty.
2. Add an `file;Name` line to `resources/duel_levels/maps.txt`. The name is what the menu shows.
3. Run `make check-maps`. It uses the game's own rules (`DuelLayout`) and reports the row and column of each problem (it also checks the survival maps).

**The base** is built by the game, the same on every map (it doesn't need to be in the file):

```
............   yard (2 free rows): where the defender goes around the eagle
............   to reach whichever flank the enemy comes from
....#@@#....   armored front: 2 stone blocks
....#EE#....   brick sides and corners (E = eagle): the base falls from the flanks
....#EE#....
```

Shooting at the front, up close or from across the map, does nothing (not even with the gun). Attacks come from either flank, and the defender can always cross from one side to the other through the yard, so there's no single entrance for someone to camp.

What the game handles on any map: the eagles (columns 12-13, on the first two and last two rows), the base above; the **base zone** (columns 9-16, on the 7 rows on each eagle's side), where the gun has no effect and stone can't be destroyed; spawn points out of anyone's line of fire; and the power-up spots.

What the map must follow, checked by `check-maps`: be mirrored horizontally and vertically (both teams get the same terrain); keep the spawn points (columns 4-5, 8-9, 16-17 and 20-21, on rows 0-1 and 24-25) and power-up spots clear; have a path **2 tiles wide** (a tank's width) from every spawn to a **flank of the enemy base** and to every power-up spot; and from every spawn to **both flanks of its own base** in at most 20 steps, so the defender can go around the eagle to the side under attack. A map that fails is rejected when the game starts, with the reason printed in the terminal, instead of breaking a match.

**Balance** (not checked by `check-maps`): since maps are mirrored, when nobody defends each team attacks down its own side, they never cross paths, and the round turns into a race. Check with `duel_sim`: in a 2 vs 2 (one attacks, one defends, `--teams ABAB`), short rounds with nearly 100% base wins point to a path that's too easy to the enemy flanks.

| In the setup screen | Key / controller |
|---------------------|------------------|
| Change option | ↑ ↓ / D-pad / stick |
| Change value | ← → / D-pad / stick |
| Confirm | Enter, Space / A, Start |
| Back | Esc / B, Back |

**Rules:**
- A round is won by destroying the enemy base or eliminating every enemy player; the match goes to the first team to win **2 rounds**.
- Each player has **3 lives** and respawns with a short shield.
- **No spawn camping by design:** each player spawns in a column of their own, alternating sides of the base (in a 1 vs 1, one spawns on the left and the other on the right). Since bullets only travel in straight lines, nobody spawns in an opponent's line of fire, on any map or format; the spawn shield is an extra safety net.
- **Colors:** color belongs to the **team**: teammates share a color (team A yellow, B green) and different colors only show up between opponents. The palette has 4 colors (yellow, green, blue, red), ready for a future free-for-all mode where each player would be their own team. The side panel lists each team's players and their lives.
- **Colored and gray power-ups:** any power-up can show up in one of two ways:
  - **in one team's color** (with an **A** or **B** above it, ~65% of the time): only that team's players can pick it up; opponents drive over it. It shows up **70% of the time in the opponent's half** (you have to invade to get it) and 30% in your own. The team is drawn 50/50 (or is the one behind on lives), even in a 3 vs 1;
  - **gray**, with the game's original icon (~35%): **any player** can pick it up. It appears at a symmetric spot in the middle of the map (same distance from both bases) or, if a team is far behind on lives, on that team's side.
- **Reinforcement:** the tank power-up brings an **allied bot** in the team's color, with one life. The first reinforcement guards the base and the second one attacks.
- **Reinforcement AI:** the bot plans a route across the map (grid pathfinding, like a GPS), going around stone and water and shooting its way through bricks, so it works on any map, including custom ones. It fires when an enemy or the enemy base is in its line of fire, turns to shoot anyone who shows up beside it and **holds its aim** while the target stays in line (it used to turn back and forth several times a second); it doesn't do a U-turn right after turning; it chases enemies with hysteresis (starts at 10 tiles, gives up at 14); defenders guard the base (one in the yard, the same distance from both flanks), each at its own post, without blocking whoever spawns behind them; and, if it gets stuck, it sidesteps and plans again. The AI never fires toward its own base.
- **Friendly fire:** bullets don't hurt teammates. A **player's** shot destroys **your own base** (the round goes to the opponent) and breaks the bricks around it, as in the original: you can open a firing angle, but mind your aim. Reinforcement bots' shots don't hurt their own base.
- **Power-ups:** after 10 s with no power-up on the map, the next one appears. Only players can pick them up (reinforcements can't).
- Duel effects: **grenade** blows up the enemy tanks on the field, even with a shield or a boat (it's an explosion, not a shot; teammates are safe); **helmet** gives a shield for **6 s** (10 s in the campaign); **clock** pins the enemy team in place for 4 s (humans can still turn and shoot); **shovel** fortifies **your** base with stone; **gun** (3 stars) breaks the map's stone, but not near the bases (see below). Grenade and gun are the rarest. In the duel, **3 stars don't absorb a hit** (in the campaign a hit only removes one star): the gun breaks stone but isn't worth an extra life.
- **Base zone:** near each eagle (columns 9 to 16, on the 7 rows on the base's side) the gun has no effect: bullets act like regular ones, wearing down bricks and stopping at stone. The base's stone (the front, and the sides when the shovel fortifies them) stays up, except against the demolisher shot.
- **Demolisher shot (the tank's second stage):** picking up the **gun while already holding a star** (in the same life) gives the demolisher shot. It breaks the **enemy base's stone** (the front, the shovel's stone and the all-stone base of 1 vs 3), but **only when fired from inside that base's zone**: it opens an attack over the top for whoever gets close, with no long-range shots from one base to the other. It doesn't break your own stone. A demolisher tank gives a quick white glint every second and shows the gun icon in the side panel; dying loses it. The stars required are in `AppConfig::duel_demolisher_stars` (default 1).
- **Fire rate:** in duel mode each player fires at most **3 shots per second** (`AppConfig::duel_max_shots_per_second`), so a barrage can't take down the enemy base with no chance to defend.
- **Uneven teams:** the smaller team gets more lives per player (1 vs 3: 6 lives vs 3) and, if the other team has twice as many players or more, an all-stone base (then the only way to win is eliminating its players).

**Balance simulation:** `make duel-sim` builds `build/bin/duel_sim`, which plays whole matches with no window, every player driven by the AI, and reports how many rounds each team wins on each map, how much each power-up helps whoever picks it up (split by whether they were behind, even or ahead on lives) and how long bots spend stuck. The balance settings (`AppConfig::duel_*`) can be tried without recompiling:

```bash
make duel-sim
cd build/bin
./duel_sim --matches 200 --teams ABAB --map 0      # 2 vs 2 on Arena
./duel_sim --teams AAB --helmet 8000 --heat        # 2 vs 1, 8 s helmet, bot heat map
```

The AI doesn't play like a person, so the numbers show trends (a power-up that decides the round on its own, a map that favors one side), not the exact result between players.
- Enter / Start pauses; Esc / Back leaves the match. Leaving, or pressing fire / Enter / A when the match ends, takes you back to the map selection with the last map highlighted: a rematch is one button away.

## 🛡️ Survival Mode (Extra Modes)

**Extra Modes → Survival**: 1 to 4 players, together, defending the eagle against **endless waves of enemies**. Pick the number of players (← →), **Next** and the map (with a thumbnail, or **Random**). Each player has their own color (P1 yellow, P2 green, P3 blue, P4 red), and the side panel shows the wave, the enemies left and everyone's lives.

**Maps** (`resources/survival_levels/`, separate from the campaign and the duel):

| Map | Style |
|-----|-------|
| **Classic** | A campaign-style map: bushes, bricks and stone |
| **Trenches** | Brick trenches across the map, with gaps |
| **Canyon** | Water bands with bridges; the boat opens shortcuts |
| **Forest** | Thick woods: enemies show up close |
| **Ice Rink** | An ice rink in the middle: hard to stop and aim |
| **Citadel** | Stone walls with gates around the base |
| **Labyrinth** | Brick maze: you can shoot your way through |
| **Islands** | Water islands with bushes |
| **Crossfire** | Brick crosses and stone pillars, lots of firing angles |
| **Last Stand** | Layers of brick shielding the base, open top |
| **Random** | A random map |

Adding a map works like in the duel: a **26×26** grid using the level symbols in `resources/survival_levels/`, a `file;Name` line in that folder's `maps.txt`, and `make check-maps`. The game builds the eagle and its brick wall; the map must keep clear the spots where enemies appear (at the top) and where players spawn, and connect all of them and the eagle's wall with tank-wide paths. Maps that fail are rejected when the game starts, with the reason printed in the terminal.

- The map wears down from wave to wave.
- Each wave has more enemies (6, 8, 10... up to 40), more of them on the map at once (4 on the first wave, +1 per extra player and +1 every 3 waves, up to 10) and tougher ones (wave N uses the difficulty of campaign stage 2N + 1, up to 35).
- Between waves there's a pause with a **WAVE N** banner (players can already get into position) and the eagle's brick wall is rebuilt.
- Every **5 waves**, everyone gets an extra life and anyone who had fallen **comes back**.
- Power-ups, scoring and friendly fire work as in the campaign (a player's shot can also take down the eagle). Players have different colors but are **one team**: every power-up is **gray**, with the original icon, and any player can pick it up (team-colored power-ups are duel-only).
- It ends when the eagle falls or everyone runs out of lives: the final screen shows the wave reached, the tanks destroyed and each player's score. Fire / Enter / A goes back to the map selection with the last map highlighted: playing again is one button away.
- The numbers live in `AppConfig::survival_*`.

## 🧨 New powers (duel and survival)

On top of the 8 original bonuses, the extra modes have 9 new powers, with their own pixel art in the style of the NES icons. They come in two kinds:

- **Storable:** they go into the player's **power slot** and are used whenever the player wants, with the **power button**. While holding a power, the player **can't pick up any other bonus**: bonuses keep appearing (and stay on the map), but can only be picked up after using the held one. **Dying loses the held power**, just like stars: holding it is a risk.
- **Immediate:** they take effect at once, like the original bonuses.

| Power | Kind | Effect |
|-------|------|--------|
| **Mine** | storable | Drops a mine where the tank stands. It blows up the first **enemy** tank that drives over it (it respects shield and boat, so it can't be used to kill a tank that just spawned). Any shot sets it off early. Lasts 30 s; in the last quarter, a white haze pulses over it. In survival it destroys even armored enemies. |
| **Barricade** | storable | Raises a 2×2 brick block right ahead. It can't be placed on a tank, base, scenery, bush or spawn point; with no room, the power stays held. |
| **Turret** | storable | Sets up a fixed cannon ahead, in the owner's color, facing where the tank faces. It turns and fires on its own at enemies lined up within 12 tiles, **never towards its own base**. One shot destroys it. It ends with whichever comes first: **10 shots or 20 s**. When it is running out (last 3 shots or last quarter of its time), a white haze pulses over it, faster near the end. |
| **Recall** | storable | Teleports the tank to its spawn point, next to its base (if taken, to another one of the team). Keeps the boat and stars. The answer to an invasion when you are far from home. |
| **Turbo** | storable | Speed ×1.5 for 8 s. |
| **Revive** | immediate | A fallen teammate comes back with one life; if nobody fell, an extra life for the teammate with the fewest. |
| **Repair** | immediate | Rebuilds your own base wall. |
| **Team shield** | immediate | Shield for every player of the team at once. |
| **Truce** | immediate, **survival only** | For 10 s, no new enemy enters the map (**TRUCE** shows at the top). Nothing spawns in the duel, so it would do nothing there: it is left out of the draw. |

**Why these are storable and the others aren't:** storing only makes sense when a power's value depends on **where** and **when** it is used. A mine, a barricade or a turret is worth a lot at the entrance of your base and almost nothing where the bonus happened to appear; recall only helps when the base is under attack; turbo, when fleeing or invading. Star, helmet, revive, repair and shield are worth the same at any time (or more if used right away), so storing them would only delay the effect.

**Power button:**

| Device | Button |
|--------|--------|
| Controller | **LB** (left shoulder; L1 on PlayStation, L on Switch) |
| `WASD` keyboard | **Left Shift** |
| `ARROWS` keyboard | **Right Shift** |

**In the side panel**, below each player's lives, the held power is shown (an empty square when there is none). In the duel, from 1 vs 1 up to 4 players, each team's panel grows to show one line per player; in survival, the icon sits next to the lives.

**In survival** the same 5 powers are storable too (`AppConfig::survival_store_powers = true`). Bonuses appear at random spots on the map, so a mine or turret used on pickup would almost always land in a corner with no enemies; stored, it becomes a defense for the eagle. With `survival_store_powers = false`, every power is used the moment it is picked up.

**Draw:** in the duel, the new powers share the draw with the originals; the strongest ones (turret, revive and team shield) are as rare as the grenade and the gun (in 1 vs 1, revive is just an extra life). The odds are in `Powers::duelTable()` and `Powers::survivalTable()` (`src/app_state/powers.cpp`); durations and ranges in `AppConfig::power_*`.

**The art** is kept as text in `tools/sprites/powers.txt` (one character per pixel, using the original icons' palette), and `make sprites` paints it into `resources/png/texture.png`. To tweak an icon, edit the drawing and run `make sprites` again.

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

The game is meant to be played with **controllers (gamepads)**; the keyboard
automatically backs up anyone without one. There is nothing to configure.

| Player | Color | Start position (campaign) |
|--------|-------|---------------------------|
| **Player 1** | Yellow | Bottom left |
| **Player 2** | Green | Bottom right |
| **Player 3** | Blue | Top left |
| **Player 4** | Red | Top right |

**On a controller**, for any player:

- **Move**: D-pad or left stick
- **Fire**: any face button (A, B, X or Y)
- **Use the held power** (duel and survival): LB
- **Start**: pause · **Back/Select**: back to the menu
- **In the menu**: D-pad or stick to choose, A/Start to confirm, B/Back to quit

**On the keyboard** there are two layouts: `WASD` (`W` `A` `S` `D` + `Space`,
power on `Left Shift`) and `ARROWS` (arrow keys + `Right Ctrl`, `Right Alt` on Mac;
power on `Right Shift`). Each layout drives a single player: one player's keys
never move or fire another tank.

### Which device goes to which player

1. **Controllers first**: the 1st connected controller goes to Player 1, the 2nd to Player 2, and so on.
2. **Keyboard as backup**: players left without a controller get `WASD`, then `ARROWS`, in order.
3. Layouts nobody needed stay with Player 1 (`WASD`) and Player 2 (`ARROWS`), so a solo player can still use the keyboard with a controller plugged in.

| Game | 0 controllers | 1 controller | 2 controllers |
|------|---------------|--------------|---------------|
| 2 players | P1 WASD, P2 ARROWS | P1 controller, P2 WASD | P1 and P2 controllers |
| 3 players | P3 has no device | P1 controller, P2 WASD, P3 ARROWS | P1, P2 controllers, P3 WASD |
| 4 players | P3 and P4 have no device | P4 has no device | P1, P2 controllers, P3 WASD, P4 ARROWS |

In the **duel mode** setup, each player shows the device they will use
(`PAD 1`, `WASD`, `ARROWS`). Anyone without a device shows up in red as
`NO PAD`, the title says how many controllers are missing, and **Start only
works once everyone has a device**. The screen updates by itself when a
controller is plugged in.

Controllers can be plugged in or removed while the game is running. If one
disconnects mid-match, the others **don't switch players**: whoever lost it falls
back to the keyboard (if a layout is free) and gets the controller back on
reconnect. Any controller SDL2 recognizes as a *game controller* works (Xbox,
PlayStation, Switch Pro and most generic ones).

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
│   │   ├── mine.h/cpp    # Mine (extra-mode power)
│   │   ├── turret.h/cpp  # Fixed turret (extra-mode power)
│   │   ├── tank.h/cpp    # Tank base class
│   │   ├── bullet.h/cpp  # Bullets
│   │   ├── bonus.h/cpp   # Power-ups
│   │   ├── brick.h/cpp   # Brick walls
│   │   └── eagle.h/cpp   # Base (eagle)
│   ├── app_state/        # Application states
│   │   ├── menu.h/cpp    # Main menu
│   │   ├── game.h/cpp    # Main game logic
│   │   ├── duel.h/cpp    # Duel (team) mode
│   │   ├── duel_layout.h/cpp # Duel geometry and map validation
│   │   ├── duel_ai.cpp   # Duel AI (reinforcement bots)
│   │   ├── survival.h/cpp # Survival mode (waves)
│   │   ├── powers.h/cpp  # New powers: storable vs immediate, and draw odds
│   │   ├── survival_layout.h/cpp # Survival map geometry and validation
│   │   ├── message_box.h/cpp # Message box for the extra modes
│   │   ├── navgrid.h/cpp # Grid pathfinding (Dijkstra) used by the AI
│   │   └── scores.h/cpp  # Score screen
│   ├── engine/           # Game engine
│   │   ├── renderer.h/cpp     # Rendering
│   │   ├── engine.h/cpp       # Core engine
│   │   └── spriteconfig.h/cpp # Sprite configuration
│   ├── app.h/cpp         # Main application
│   ├── appconfig.h/cpp   # Global settings (including keyboard layouts)
│   ├── controllers.h/cpp # Gamepads: hotplug and player assignment
│   ├── soundmanager.h/cpp # Audio manager
│   └── type.h            # Type definitions
├── resources/            # Game assets
│   ├── img/              # Images and sprites
│   ├── sound/            # Sound effects
│   ├── font/             # Fonts
│   ├── levels/           # The 36 level files
│   ├── duel_levels/      # Duel mode maps (listed in maps.txt)
│   └── survival_levels/  # Survival mode maps (listed in maps.txt)
├── tools/                # WSL install/uninstall scripts and the duel simulation
│   ├── duel_sim.cpp      # Headless duel simulation (AI vs AI)
│   ├── paint_sprites.cpp # Paints text pixel art into the texture (make sprites)
│   ├── sprites/powers.txt # Pixel art of the new powers
│   └── duel_sim_report.py # Adds up the results of several simulation runs
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

### Extra modes: always playable solo
Every extra mode that is **not** a contest between players (like survival) must work with **a single player**, as well as 2 to 4. Competitive modes (the duel: 1 vs 1, 2 vs 1, 2 vs 2, up to 4 players) are the exception, since they need an opponent.

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
- Check the assignment table: the 1st connected controller always belongs to Player 1; in duel mode, the setup screen shows each player's device
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
