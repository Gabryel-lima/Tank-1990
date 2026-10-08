#!/bin/bash
# Tank 1990 no EmuELEC: o atalho que aparece em "Ports". Vai em roms/ports_scripts/ e o jogo em
# roms/ports/tank1990/ (ver EMUELEC.md no repositório).

# Variáveis e funções do EmuELEC (EE_DEVICE, fbfix...)
. /etc/profile

# O jogo fica em ../ports/tank1990 a partir deste script (cartão ou pendrive)
SCRIPTDIR="$(cd "$(dirname "$0")" && pwd)"
GAMEDIR="$(cd "$(dirname "$0")/.." && pwd)/ports/tank1990"
[ -d "$GAMEDIR" ] || GAMEDIR=/storage/roms/ports/tank1990
cd "$GAMEDIR" || exit 1

# Nome, descrição e arte do menu: junta a entrada do jogo ao gamelist.xml dos Ports, sem apagar
# a dos outros ports (ver gamelist-merge.sh). Na primeira vez, aparecem quando o jogo fecha
sh ./gamelist-merge.sh ./gamelist-entry.xml "$SCRIPTDIR" ./.gamelist-stamp > ./gamelist.log 2>&1

# Nos Amlogic-ng (S905X2, S905X3, S922X) o framebuffer precisa disso antes de um port SDL
if [ "$EE_DEVICE" = "Amlogic-ng" ] && command -v fbfix >/dev/null 2>&1; then
    fbfix
fi

# Tela cheia, e os mapeamentos de controles que o EmuELEC já conhece
export TANK_FULLSCREEN=1
DB=/storage/.config/SDL-GameControllerDB/gamecontrollerdb.txt
[ -f "$DB" ] && export SDL_GAMECONTROLLERCONFIG="$(cat "$DB")"

# A partição das ROMs é FAT: conforme a montagem do stick, nada nela tem permissão de executar
# ("Permission denied", saída 126) e o chmod não muda isso. Por isso o jogo roda de uma cópia do
# binário em /tmp (RAM), com a pasta de trabalho no jogo: os mapas, os sons e a fonte ficam no
# cartão (o jogo os abre pelo caminho relativo à pasta de trabalho)
RUN=/tmp/tank1990-run
rm -rf "$RUN"
if mkdir -p "$RUN" && cp ./Tanks "$RUN/Tanks" && chmod +x "$RUN/Tanks"; then
    BIN="$RUN/Tanks"
else
    BIN=./Tanks
fi
echo "launcher: $BIN" > "$GAMEDIR/log.txt"
"$BIN" >> "$GAMEDIR/log.txt" 2>&1
rm -rf "$RUN"
