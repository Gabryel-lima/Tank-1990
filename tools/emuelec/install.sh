#!/bin/sh
# Instala o pacote do EmuELEC no stick pela rede (SSH), a partir do PC:
#
#   sh tools/emuelec/install.sh <IP do stick> [pacote.zip]   (ou make emuelec-install HOST=<IP>)
#
# Por que não basta copiar pelo leitor de cartão: no EmuELEC 4.3, o boot (emustation-config) monta
# por cima de roms/ports_scripts um overlay (lowerdir=/usr/bin/ports, upperdir=/emuelec/ports, na
# partição STORAGE). O que foi copiado para EEROMS/ports_scripts no PC fica escondido embaixo
# dele, e o atalho do jogo não aparece em "Ports". Gravado com o stick ligado (por aqui ou pela
# pasta compartilhada \\EMUELEC\roms), cai no overlay e aparece.
#
# Depois de copiar, junta a entrada de cada jogo do pacote ao gamelist.xml dos Ports pela API do
# EmulationStation (gamelist-merge.sh): o jogo aparece na hora, com nome e arte, mesmo com
# "Parse gamelists only" ligado (aí o EmulationStation nem procura arquivo novo na pasta).
#
# Precisa de ssh e tar no PC (Linux, macOS, WSL) e do SSH ligado no stick (Start -> Network
# Settings -> Enable SSH). Usuário root, senha emuelec, se não foi trocada. Mesmo arquivo no
# Tank 1990, no CharyRick e no MK64.
set -eu

HOST=${1:-}
ZIP=${2:-}
[ -n "$HOST" ] || { echo "uso: sh tools/emuelec/install.sh <IP do stick> [pacote.zip]" >&2; exit 2; }
# Os testes trocam o ssh e a pasta das ROMs (gamelist-test.sh)
SSH=${EMUELEC_SSH:-ssh}
ROMS=${EMUELEC_ROMS:-/storage/roms}

# O pacote pode vir com caminho relativo à pasta de onde se chamou
case $ZIP in /*|'') ;; *) ZIP="$(pwd)/$ZIP" ;; esac
cd "$(dirname "$0")/../.."
if [ -z "$ZIP" ]; then
    for z in build-emuelec/*-emuelec.zip build/emuelec/*-emuelec.zip; do
        [ -f "$z" ] && { ZIP=$z; break; }
    done
fi
[ -n "$ZIP" ] && [ -f "$ZIP" ] || { echo "install.sh: pacote não encontrado (rode make emuelec)" >&2; exit 1; }

T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
if command -v unzip >/dev/null 2>&1; then
    unzip -q "$ZIP" -d "$T"
else
    python3 -I -m zipfile -e "$ZIP" "$T"
fi
[ -d "$T/ports" ] && [ -d "$T/ports_scripts" ] || { echo "install.sh: $ZIP não tem ports/ e ports_scripts/" >&2; exit 1; }

# Os jogos do pacote (ports/<nome>/gamelist-entry.xml); a marca sai para a entrada ser refeita
merge=
for e in "$T"/ports/*/gamelist-entry.xml; do
    [ -f "$e" ] || continue
    n=$(basename "$(dirname "$e")")
    merge="$merge
cd '$ROMS/ports/$n' && rm -f .gamelist-stamp &&
    sh ./gamelist-merge.sh ./gamelist-entry.xml '$ROMS/ports_scripts' ./.gamelist-stamp 2>&1 | tee ./gamelist.log"
done

echo ">> copiando $ZIP para $HOST:$ROMS (senha do root: emuelec, se não foi trocada)"
# Sem os ._ do macOS; o -o não tenta pôr no stick o dono dos arquivos no PC
(cd "$T" && COPYFILE_DISABLE=1 tar -cf - ports ports_scripts) |
    "$SSH" "root@$HOST" "set -e
mkdir -p '$ROMS' && cd '$ROMS' && tar -xof -
$merge"
echo ">> pronto: abra Ports no EmuELEC (se o jogo não aparecer, reinicie o EmulationStation)"
