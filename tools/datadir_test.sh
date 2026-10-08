#!/bin/sh
# O jogo acha os arquivos dele (texture.png, fonte, levels/, sons) pela pasta de dados, mesmo com o
# executável noutro lugar. No console de TV (EmuELEC) o lançador roda uma cópia do binário em
# /storage/.tmp, porque a partição do cartão é FAT e não executa; sem TANK_DATA_DIR o jogo entrava
# na pasta da cópia, que está vazia, e ficava sem textura, fonte e sons (tela preta).
#
#   sh tools/datadir_test.sh <pasta com Tanks e os arquivos do jogo>   (ou make datadir-test)
#
# Roda o jogo com vídeo e áudio "dummy" por 2 s, sem janela, e procura o erro de som que ele
# imprime quando um arquivo falta.
set -eu

BIN=${1:?uso: datadir_test.sh <pasta do jogo>}
BIN=$(cd "$BIN" && pwd)
ROOT=$(cd "$(dirname "$0")/.." && pwd)
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
fail() { echo "datadir-test: $*" >&2; exit 1; }

cp "$BIN/Tanks" "$T/Tanks"
cd "$T"

run() { # run <TANK_DATA_DIR ou vazio>
    TANK_DATA_DIR=${1:-} SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        timeout 2 ./Tanks > "$T/out.txt" 2>&1 || true
    grep -c 'Erro ao carregar' "$T/out.txt" || true
}

# O executável longe dos dados e sem a variável: o jogo não acha nada (é o que o teste vigia)
[ "$(run '')" -gt 0 ] || fail "sem TANK_DATA_DIR o jogo devia ficar sem os arquivos (o teste não pega nada)"
# Com a variável apontando para os dados: acha tudo
n=$(run "$BIN")
[ "$n" = 0 ] || fail "com TANK_DATA_DIR o jogo ainda não achou $n sons: $(head -n 2 "$T/out.txt")"
# O jogo diz que áudio abriu (o log.txt do console de TV mostra se o driver é o "dummy" ou outro)
grep -q '^Audio: driver=' "$T/out.txt" || fail "o jogo não registrou o áudio que abriu: $(head -n 3 "$T/out.txt")"
# E o lançador do console de TV a define (a cópia do binário roda longe do cartão)
grep -q '^export TANK_DATA_DIR="\$GAMEDIR"' "$ROOT/tools/emuelec/Tank1990.sh" ||
    fail "o lançador do EmuELEC não exporta TANK_DATA_DIR"
echo "datadir-test: o jogo acha os arquivos pela pasta de dados, com o executável em outro lugar"
