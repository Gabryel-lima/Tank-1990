#!/bin/sh
# Instala o pacote do EmuELEC no cartão de memória do stick, com o cartão num leitor do PC (Linux):
#
#   sh tools/emuelec/card-install.sh [pacote.zip] [pasta onde o cartão montou]
#   (ou make emuelec-card; a pasta padrão é /media/$USER)
#
# Por que não basta descompactar o pacote na partição das ROMs: no EmuELEC 4.3 o boot monta um
# overlay por cima de roms/ports_scripts (lowerdir=/usr/bin/ports, upperdir=/emuelec/ports, que é
# /storage/.config/emuelec/ports na partição STORAGE). O ports_scripts da partição das ROMs fica
# embaixo da montagem e o menu nunca o vê. A camada de cima do overlay é uma pasta comum da
# partição STORAGE (ext4), e o Linux escreve nela com o cartão no leitor: os atalhos e as imagens
# vão para lá, e o stick os mostra em "Ports" no próximo boot.
#
# - ports/<jogo>/ vai para a partição das ROMs (sem overlay por cima);
# - ports_scripts/ (atalhos e imagens) vai para a camada de cima do overlay, na STORAGE, e também
#   para o ports_scripts das ROMs, que é o que vale num EmuELEC sem o overlay (4.5 em diante);
# - a entrada de cada jogo entra no gamelist.xml dos dois lugares (gamelist-merge.sh), sem apagar
#   a dos outros ports. Um arquivo da camada de cima que é um "whiteout" do overlay (dispositivo
#   de caractere 0,0, o que o overlay grava quando um arquivo de baixo é apagado) dá lugar ao
#   arquivo do pacote.
#
# A STORAGE é do root: as cópias para lá usam sudo. Mesmo arquivo no Tank 1990, no CharyRick e no MK64.
set -eu

ZIP=${1:-}
BASE=${2:-/media/${SUDO_USER:-${USER:-}}}
# Os testes trocam o sudo e apontam as partições direto (gamelist-test.sh)
SUDO=${CARD_SUDO-sudo}
ROMS=${CARD_ROMS:-}
UPPER=${CARD_UPPER:-}

case $ZIP in /*|'') ;; *) ZIP="$(pwd)/$ZIP" ;; esac
cd "$(dirname "$0")/../.."
if [ -z "$ZIP" ]; then
    for z in build-emuelec/*-emuelec.zip build/emuelec/*-emuelec.zip; do
        [ -f "$z" ] && { ZIP=$z; break; }
    done
fi
[ -n "$ZIP" ] && [ -f "$ZIP" ] || { echo "card-install.sh: pacote não encontrado (rode make emuelec)" >&2; exit 1; }

# As partições: a das ROMs tem roms/, a STORAGE tem .config/emuelec/
for d in "$BASE"/*; do
    [ -z "$ROMS" ] && [ -d "$d/roms" ] && ROMS="$d/roms"
    [ -z "$UPPER" ] && [ -d "$d/.config/emuelec" ] && UPPER="$d/.config/emuelec/ports"
done
[ -n "$ROMS" ] || { echo "card-install.sh: não achei a partição das ROMs (a pasta roms/) em $BASE" >&2; exit 1; }
if [ -z "$UPPER" ]; then
    echo "card-install.sh: não achei a partição STORAGE (.config/emuelec) em $BASE." >&2
    echo "  Monte-a (abra-a no gerenciador de arquivos) e rode de novo. Sem ela, o atalho fica" >&2
    echo "  escondido pelo overlay do EmuELEC 4.3 e o jogo não aparece em Ports." >&2
    exit 1
fi

T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
if command -v unzip >/dev/null 2>&1; then
    unzip -q "$ZIP" -d "$T"
else
    python3 -I -m zipfile -e "$ZIP" "$T"
fi
[ -d "$T/ports" ] && [ -d "$T/ports_scripts" ] || { echo "card-install.sh: $ZIP não tem ports/ e ports_scripts/" >&2; exit 1; }

echo ">> jogo em $ROMS/ports"
mkdir -p "$ROMS/ports" "$ROMS/ports_scripts"
cp -r "$T"/ports/* "$ROMS/ports/"
cp -r "$T"/ports_scripts/* "$ROMS/ports_scripts/"

# Copia ports_scripts/ para a camada de cima do overlay, trocando whiteouts por arquivos
echo ">> atalhos e imagens em $UPPER (sudo)"
$SUDO mkdir -p "$UPPER/images"
(cd "$T/ports_scripts" && find . -type f) | while read -r f; do
    [ -c "$UPPER/$f" ] && $SUDO rm -f "$UPPER/$f"
    $SUDO cp "$T/ports_scripts/$f" "$UPPER/$f"
    case $f in *.sh) $SUDO chmod 755 "$UPPER/$f" ;; esac
done

# A entrada de cada jogo nos dois gamelist.xml (sem a API: o stick está desligado)
[ -c "$UPPER/gamelist.xml" ] && $SUDO rm -f "$UPPER/gamelist.xml"
for e in "$T"/ports/*/gamelist-entry.xml; do
    [ -f "$e" ] || continue
    for dir in "$UPPER" "$ROMS/ports_scripts"; do
        run=
        [ "$dir" = "$UPPER" ] && run=$SUDO
        $run env GAMELIST_API= HOME="$T" sh tools/emuelec/gamelist-merge.sh "$e" "$dir" "$T/stamp"
        rm -f "$T/stamp"
    done
done

sync
echo ">> pronto: ejete as partições do cartão, ponha-o no stick e ligue. O jogo aparece em Ports."
