#!/bin/sh
# Segunda metade do tools/emuelec/build.sh: roda dentro do container do toolchain aarch64, sem
# rede. Compila as bibliotecas (deps.sh), o jogo, e monta o pacote em $OUT.
set -eu

: "${OUT:?}"
export HOST=aarch64-unknown-linux-gnu PREFIX="$OUT/prefix" SRC="$OUT/src"
sh tools/emuelec/deps.sh sdl2 image mixer ttf

# O jogo, com o mesmo Makefile do PC, em $OUT/obj. Linka contra o SDL 2.0.9 (dinâmico: no stick
# é o do sistema) e as outras bibliotecas estáticas; libstdc++ e libgcc vão dentro do binário.
# --no-undefined: uma função do SDL mais nova que a 2.0.9 para a build aqui
mkdir -p "$OUT/obj/bin" "$OUT/obj/engine" "$OUT/obj/app_state" "$OUT/obj/objects" "$OUT/obj/input"
make --no-print-directory -j"$(nproc)" BUILD="$OUT/obj" CC="$CXX" \
    CFLAGS="-c -Wall -std=c++17 -MMD -MP -O2" \
    INCLUDEPATH="-I$PREFIX/include -I$PREFIX/include/SDL2 -D_REENTRANT" \
    LIBSPATH="-L$PREFIX/lib" \
    LIBS="-lSDL2_mixer -lSDL2_image -lSDL2_ttf -lfreetype -lSDL2 -lpthread -lm" \
    LFLAGS="-O2 -s -static-libstdc++ -static-libgcc -Wl,--no-undefined" \
    compile

# Pacote: o mesmo arranjo do build/bin do PC (mapas, imagem e fonte ao lado do executável, os
# sons em resources/sound), só com o que o jogo lê
PKG="$OUT/package"
GAME="$PKG/ports/tank1990"
rm -rf "$PKG"
mkdir -p "$GAME/resources" "$PKG/ports_scripts"
cp "$OUT/obj/bin/Tanks" "$GAME/"
cp -r resources/levels resources/duel_levels resources/survival_levels "$GAME/"
cp resources/font/prstartk.ttf resources/png/texture.png "$GAME/"
cp -r resources/sound "$GAME/resources/"
cp tools/emuelec/Tank1990.sh "$PKG/ports_scripts/"
chmod +x "$GAME/Tanks" "$PKG/ports_scripts/Tank1990.sh"

rm -f "$OUT/Tank1990-emuelec.zip"
(cd "$PKG" && zip -qr ../Tank1990-emuelec.zip ports ports_scripts)
