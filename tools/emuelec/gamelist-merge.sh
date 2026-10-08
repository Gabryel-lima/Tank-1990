#!/bin/sh
# Junta a entrada deste jogo ao gamelist.xml dos Ports do EmuELEC, sem apagar a dos outros jogos.
# O lançador (ports_scripts/<Jogo>.sh) chama este script a cada vez que abre o jogo:
#
#   sh gamelist-merge.sh <entrada.xml> <pasta ports_scripts> <arquivo de marca>
#
# O pacote não traz mais um ports_scripts/gamelist.xml: descompactado por cima, ele apagaria as
# entradas dos outros ports. A entrada do jogo vai dentro da pasta dele (ports/<jogo>/) e entra
# no gamelist.xml por aqui. Mesmo arquivo no Tank 1990 e no CharyRick.
#
# Caminho principal: a API local do EmulationStation (POST /addgames/ports, porta 1234, presente
# desde o EmuELEC 4.3). O EmulationStation põe a entrada na memória e regrava só ela no arquivo.
# Editar o arquivo direto não basta com ele aberto: ao sair, ele regrava a entrada de todo jogo
# que mudou (o jogo que acabou de rodar, pela contagem de partidas) a partir do que tem na
# memória, e a descrição e a arte sumiriam. Sem a API (EmulationStation fechado, versão antiga),
# edita o arquivo direto.
#
# O que a pessoa marcou ou jogou (favorito, oculto, partidas, último jogo, tempo) passa para a
# entrada nova. Só refaz quando a entrada sumiu ou mudou (a marca guarda a soma da entrada).
set -u

ENTRY=$1 DIR=$2 STAMP=$3
API=${GAMELIST_API-http://127.0.0.1:1234}
[ -f "$ENTRY" ] || { echo "gamelist-merge: falta $ENTRY" >&2; exit 1; }

# O EmulationStation lê o gamelist.xml da pasta do sistema ou, se lá não houver, o da pasta de
# configuração dele (SystemData::getGamelistPath)
LIST="$DIR/gamelist.xml"
ESLIST="${HOME:-/storage}/.emulationstation/gamelists/ports/gamelist.xml"
[ ! -f "$LIST" ] && [ -f "$ESLIST" ] && LIST=$ESLIST

game_path=$(sed -n 's|.*<path>\(.*\)</path>.*|\1|p' "$ENTRY" | head -n 1)
image=$(sed -n 's|.*<image>\(.*\)</image>.*|\1|p' "$ENTRY" | head -n 1)
# Soma da entrada, para saber se mudou. O busybox do EmuELEC não tem cksum: usa o que houver
checksum() {
    if command -v cksum >/dev/null 2>&1; then cksum
    elif command -v md5sum >/dev/null 2>&1; then md5sum
    elif command -v sha1sum >/dev/null 2>&1; then sha1sum
    else wc -c; fi
}
sum=$(checksum < "$ENTRY")

# O bloco <game>...</game> cujo <path> é $1 (vazio: o primeiro), de um gamelist.xml. O
# EmulationStation e os nossos arquivos põem cada etiqueta numa linha
block() { # block <caminho> <arquivo>
    awk -v p="<path>$1</path>" '
        !inside && /<game[ >]/ { inside = 1; buf = "" }
        inside { buf = buf $0 "\n" }
        inside && /<\/game>/ {
            inside = 0
            if (p == "<path></path>" || index(buf, p)) { printf "%s", buf; exit }
        }' "$2"
}

TMP="${TMPDIR:-/tmp}/gamelist-merge.$$"
trap 'rm -f "$TMP".*' EXIT
: > "$TMP.old"
[ -f "$LIST" ] && block "$game_path" "$LIST" > "$TMP.old"
if [ "$(cat "$STAMP" 2>/dev/null)" = "$sum" ] && grep -qF "<image>$image</image>" "$TMP.old"; then
    exit 0
fi

# A entrada a gravar: a do pacote, com o que a pessoa já tinha na entrada antiga
grep -E '<(favorite|hidden|kidgame|playcount|lastplayed|gametime)>' "$TMP.old" > "$TMP.keep"
block "" "$ENTRY" | awk -v keep="$TMP.keep" '
    /<\/game>/ { while ((getline line < keep) > 0) print line }
    { print }' > "$TMP.game"
{ echo '<?xml version="1.0"?>'; echo '<gameList>'; cat "$TMP.game"; echo '</gameList>'; } > "$TMP.body"

# 1) Pela API do EmulationStation (curl ou wget, o que houver; o EmuELEC tem os dois)
answer=
if [ -n "$API" ]; then
    if command -v curl >/dev/null 2>&1; then
        answer=$(curl -s -m 5 -H 'Content-Type: application/xml' --data-binary "@$TMP.body" \
            "$API/addgames/ports" 2>/dev/null)
    elif command -v wget >/dev/null 2>&1; then
        answer=$(wget -q -T 5 -O - --header='Content-Type: application/xml' \
            --post-file="$TMP.body" "$API/addgames/ports" 2>/dev/null)
    fi
fi
if [ "$answer" = OK ]; then
    echo "gamelist-merge: entrada de $game_path gravada pelo EmulationStation"
    echo "$sum" > "$STAMP"
    exit 0
fi

# 2) Direto no arquivo: tira a entrada antiga deste jogo e põe a nova antes do </gameList>.
# Um arquivo que não dá para entender fica como está
if [ -f "$LIST" ] && grep -q '</gameList>' "$LIST"; then
    awk -v p="<path>$game_path</path>" -v game="$TMP.game" '
        !inside && /<game[ >]/ { inside = 1; buf = "" }
        inside {
            buf = buf $0 "\n"
            if (/<\/game>/) { inside = 0; if (!index(buf, p)) printf "%s", buf }
            next
        }
        /<\/gameList>/ { while ((getline line < game) > 0) print line }
        { print }' "$LIST" > "$TMP.list"
elif [ ! -f "$LIST" ] || ! grep -q '[^[:space:]]' "$LIST" || grep -q '<gameList */>' "$LIST"; then
    cp "$TMP.body" "$TMP.list"
else
    echo "gamelist-merge: $LIST não tem </gameList>; não mexi nele" >&2
    exit 1
fi
cat "$TMP.list" > "$LIST" || exit 1
echo "gamelist-merge: entrada de $game_path gravada em $LIST"
echo "$sum" > "$STAMP"
