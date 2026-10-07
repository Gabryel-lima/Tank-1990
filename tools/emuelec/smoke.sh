#!/bin/sh
# Abre o binário do pacote do EmuELEC num emulador de ARM (qemu-aarch64), com vídeo e áudio de
# mentira, e confere que ele carrega tudo (fonte, textura, sons, mapas) e fica de pé no menu,
# com a demonstração rodando no fundo. Não substitui o teste no stick (a GPU e os controles de
# verdade só lá), mas pega binário que não abre, recurso faltando no pacote e erro de carga.
#
#   sh tools/emuelec/smoke.sh   (depois do make emuelec; precisa do qemu-user e do Docker)
set -eu

cd "$(dirname "$0")/../.."
OUT=build/emuelec
IMAGE=$(sed -n 's/^IMAGE=//p' tools/emuelec/build.sh)
command -v qemu-aarch64 >/dev/null 2>&1 || { echo "smoke.sh: precisa do qemu-aarch64 (pacote qemu-user)" >&2; exit 1; }

# Bibliotecas de sistema do ARM: a glibc do toolchain e o SDL 2.0.9 que a build usou
SYSROOT="$OUT/sysroot"
if [ ! -f "$SYSROOT/lib/libc.so.6" ]; then
    mkdir -p "$SYSROOT"
    id=$(docker create "$IMAGE")
    docker cp "$id:/usr/xcc/aarch64-unknown-linux-gnu/aarch64-unknown-linux-gnu/sysroot/lib" "$SYSROOT/"
    docker rm "$id" >/dev/null
fi
cp -P "$OUT"/prefix/lib/libSDL2-2.0.so.0* "$SYSROOT/lib/"
SYSROOT=$(cd "$SYSROOT" && pwd)

cd "$OUT/package/ports/tank1990"
status=0
TANK_FULLSCREEN=1 SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy LD_LIBRARY_PATH="$SYSROOT/lib" \
    timeout 15 qemu-aarch64 -L "$SYSROOT" ./Tanks > ../smoke.log 2>&1 || status=$?

# 124: o timeout fechou o jogo, que ainda estava rodando (o esperado)
if [ "$status" -ne 124 ] || [ -s ../smoke.log ]; then
    echo "smoke.sh: o jogo saiu com $status ou reclamou de algo:" >&2
    cat ../smoke.log >&2
    exit 1
fi
rm -f ../smoke.log
echo "smoke.sh: o binário aarch64 abriu e rodou 15 s sem erros"
