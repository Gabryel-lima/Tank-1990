#!/bin/bash
# Tank 1990 no EmuELEC: o atalho que aparece em "Ports". Vai em roms/ports_scripts/ e o jogo em
# roms/ports/tank1990/ (ver EMUELEC.md no repositório).

# Variáveis e funções do EmuELEC (EE_DEVICE, fbfix...)
. /etc/profile

# O jogo fica em ../ports/tank1990 a partir deste script (cartão ou pendrive)
GAMEDIR="$(cd "$(dirname "$0")/.." && pwd)/ports/tank1990"
[ -d "$GAMEDIR" ] || GAMEDIR=/storage/roms/ports/tank1990
cd "$GAMEDIR" || exit 1

# Nos Amlogic-ng (S905X2, S905X3, S922X) o framebuffer precisa disso antes de um port SDL
if [ "$EE_DEVICE" = "Amlogic-ng" ] && command -v fbfix >/dev/null 2>&1; then
    fbfix
fi

# Tela cheia, e os mapeamentos de controles que o EmuELEC já conhece
export TANK_FULLSCREEN=1
DB=/storage/.config/SDL-GameControllerDB/gamecontrollerdb.txt
[ -f "$DB" ] && export SDL_GAMECONTROLLERCONFIG="$(cat "$DB")"

chmod +x ./Tanks 2>/dev/null
./Tanks > "$GAMEDIR/log.txt" 2>&1
