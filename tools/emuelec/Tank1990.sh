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

# Tela cheia, e a pasta dos dados: o jogo entra na pasta do executável para achar mapas, sons,
# fonte e textura, e a cópia do binário roda longe deles (ver abaixo)
export TANK_FULLSCREEN=1
export TANK_DATA_DIR="$GAMEDIR"
# Cada botão apertado e o mapeamento do controle vão para o log.txt (para achar por que um
# controle genérico "troca" botões); ver CONTROLES.md
export TANK_PAD_LOG=1

# Os mapeamentos de controles que o EmuELEC já conhece. O arquivo inteiro passa de 128 KiB, o
# máximo de UMA variável de ambiente no Linux (MAX_ARG_STRLEN): com ele exportado, todo exec
# depois (mkdir, cp, o próprio jogo) falha com "Argument list too long". Vão só os mapeamentos
# dos controles ligados (o EmulationStation passa os GUIDs em --controllers) e os de Linux, até
# 100000 bytes
DB=${GAMECONTROLLERDB:-/storage/.config/SDL-GameControllerDB/gamecontrollerdb.txt}
SDL_GAMECONTROLLERCONFIG=
if [ -f "$DB" ]; then
    GUIDS=$(echo "$*" | tr ' "' '\n\n' | grep -E '^[0-9a-fA-F]{32}$' | sort -u)
    SDL_GAMECONTROLLERCONFIG=$({ for g in $GUIDS; do grep -i "^$g" "$DB"; done
        grep 'platform:Linux' "$DB"; } |
        awk '!seen[$0]++ { n += length($0) + 1; if (n > 100000) exit; print }')
fi
# Os mapeamentos do jogo (gamecontrollerdb.extra.txt) vão por último: para um mesmo GUID, o
# último vale. É onde o controle do Y6 ganha o mapeamento conferido com o do fabricante (o mesmo
# do CharyRick), em vez de o que o banco do sistema tiver por último para ele
if [ -f ./gamecontrollerdb.extra.txt ]; then
    SDL_GAMECONTROLLERCONFIG="$SDL_GAMECONTROLLERCONFIG
$(grep -v '^#' ./gamecontrollerdb.extra.txt)"
fi
[ -n "$SDL_GAMECONTROLLERCONFIG" ] && export SDL_GAMECONTROLLERCONFIG

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
