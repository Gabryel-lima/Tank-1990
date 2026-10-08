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
# binário numa pasta que executa (a STORAGE é ext4; depois /tmp e /dev/shm, que são RAM), com a
# pasta de trabalho no jogo: os mapas, os sons e a fonte ficam no cartão (o jogo os abre pelo
# caminho relativo à pasta de trabalho). Cada erro vai para o log.txt; se o binário não executar
# (126), tenta o próximo lugar e, por fim, o do cartão
LOG="$GAMEDIR/log.txt"
: > "$LOG"
STATUS=126
# (o gamelist-test.sh troca a lista)
for DIR in ${EMUELEC_RUN_DIRS:-/storage/.tmp /tmp /dev/shm}; do
    RUN="$DIR/tank1990-run"
    rm -rf "$RUN"
    mkdir -p "$RUN" >> "$LOG" 2>&1 && cp ./Tanks "$RUN/Tanks" >> "$LOG" 2>&1 &&
        chmod +x "$RUN/Tanks" >> "$LOG" 2>&1 || continue
    echo "launcher: $RUN/Tanks" >> "$LOG"
    "$RUN/Tanks" >> "$LOG" 2>&1
    STATUS=$?
    rm -rf "$RUN"
    [ "$STATUS" -ne 126 ] && break
done
if [ "$STATUS" -eq 126 ]; then
    echo "launcher: ./Tanks" >> "$LOG"
    ./Tanks >> "$LOG" 2>&1
    STATUS=$?
fi
exit "$STATUS"
