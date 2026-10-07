#!/bin/sh
# Gera o pacote do Tank 1990 para o EmuELEC (GameStick / Powkiddy Y6 e outros Amlogic aarch64):
#
#   build/emuelec/Tank1990-emuelec.zip
#     ports_scripts/Tank1990.sh   o atalho que aparece em "Ports" no menu do EmuELEC
#     ports/tank1990/             o jogo (binário aarch64 + mapas, imagens, fonte e sons)
#
# Descompactado na raiz da partição de ROMs do cartão (a pasta "roms"), fica pronto para jogar.
# Ver EMUELEC.md. Uso: make emuelec (ou sh tools/emuelec/build.sh). Precisa de Docker e de rede
# só para baixar o toolchain e os fontes; a compilação roda num container sem rede.
set -eu

cd "$(dirname "$0")/../.."
OUT=build/emuelec

# Toolchain aarch64 com glibc 2.27 (o EmuELEC 4.3 tem a 2.29: a glibc do binário não pode ser
# mais nova que a do stick). Fixada por digest para a build não mudar sozinha
IMAGE=dockcross/linux-arm64-lts@sha256:1c7d47bc448cb7b4c28850f10f45203a7e20ea8f7032cb2ccd059d3ce4104f8f

# Fontes das bibliotecas (deps.sh): nome, sha256 e endereço
SOURCES="
SDL2-2.0.9.tar.gz 255186dc676ecd0c1dbf10ec8a2cc5d6869b5079d8a38194c2aecdff54b324b1 https://github.com/libsdl-org/SDL/releases/download/release-2.0.9/SDL2-2.0.9.tar.gz
SDL2_image-2.6.3.tar.gz 931c9be5bf1d7c8fae9b7dc157828b7eee874e23c7f24b44ba7eff6b4836312c https://github.com/libsdl-org/SDL_image/releases/download/release-2.6.3/SDL2_image-2.6.3.tar.gz
SDL2_mixer-2.6.3.tar.gz 7a6ba86a478648ce617e3a5e9277181bc67f7ce9876605eea6affd4a0d6eea8f https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.6.3/SDL2_mixer-2.6.3.tar.gz
SDL2_ttf-2.0.15.tar.gz a9eceb1ad88c1f1545cd7bd28e7cbc0b2c14191d40238f531a15b01b1b22cd33 https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.0.15/SDL2_ttf-2.0.15.tar.gz
SDL2_ttf-2.20.2.tar.gz 9dc71ed93487521b107a2c4a9ca6bf43fb62f6bddd5c26b055e6b91418a22053 https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.20.2/SDL2_ttf-2.20.2.tar.gz
"

command -v docker >/dev/null 2>&1 || { echo "build.sh: precisa do Docker (ver EMUELEC.md)" >&2; exit 1; }

mkdir -p "$OUT/src"
echo "$SOURCES" | while read -r name sum url; do
    [ -n "$name" ] || continue
    file="$OUT/src/$name"
    if [ ! -f "$file" ]; then
        echo ">> baixando $name"
        curl -fsSL -o "$file.part" "$url"
        mv "$file.part" "$file"
    fi
    echo "$sum  $file" | sha256sum -c --quiet - || { rm -f "$file"; echo "build.sh: $name corrompido, baixe de novo" >&2; exit 1; }
done

docker run --rm --network none -u "$(id -u):$(id -g)" -v "$PWD:/work" -w /work \
    -e OUT="/work/$OUT" "$IMAGE" sh tools/emuelec/inside.sh

echo ""
echo "Pacote: $OUT/Tank1990-emuelec.zip (ver EMUELEC.md para copiar para o cartão)"
