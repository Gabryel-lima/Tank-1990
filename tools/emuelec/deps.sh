#!/bin/sh
# Compila as bibliotecas do SDL para aarch64 em $PREFIX. Roda dentro do container do
# tools/emuelec/build.sh, sem rede: os fontes já vêm baixados em $SRC.
#
#   deps.sh sdl2 image mixer ttf
#
# - sdl2: o SDL 2.0.9, a mesma versão do EmuELEC 4.3 (que a mantém presa, com patches para a GPU
#   Mali). Serve só para linkar: o jogo usa o libSDL2 do stick, e esta cópia nem vai no pacote.
#   Como o link é feito contra ela, uma função mais nova que a 2.0.9 vira erro aqui, e não um
#   "undefined symbol" no stick.
# - image, mixer, ttf: estáticas, entram no binário, com os decodificadores embutidos
#   (stb_image para PNG, stb_vorbis e dr_wav para OGG e WAV, FreeType para as fontes).
#
# Os fontes (e o sha256 de cada um) estão no tools/emuelec/build.sh.
#
# O mesmo arquivo existe no Tank-1990 e no CharyRick; mude os dois juntos.
set -eu

: "${HOST:?}" "${PREFIX:?}" "${SRC:?}"
JOBS=$(nproc)
# Só as bibliotecas daqui, nunca as do sistema do container
export PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig" PKG_CONFIG_PATH=
WORK=$(mktemp -d)
mkdir -p "$PREFIX"

# Instala um pacote só uma vez: o $PREFIX fica no build/ entre uma compilação e outra
built() { [ -f "$PREFIX/.built-$1" ]; }
done_() { touch "$PREFIX/.built-$1"; }
unpack() { tar -xzf "$SRC/$1" -C "$WORK"; }

for dep in "$@"; do
    built "$dep" && continue
    echo ">> $dep"
    case "$dep" in
    sdl2)
        unpack SDL2-2.0.9.tar.gz
        (cd "$WORK/SDL2-2.0.9" && ./configure --host="$HOST" --prefix="$PREFIX" --disable-static \
            --disable-video-x11 --disable-video-wayland --disable-video-kmsdrm \
            --disable-video-vulkan --disable-video-opengl --disable-pulseaudio --disable-alsa \
            --disable-jack --disable-esd --disable-arts --disable-nas --disable-sndio \
            --disable-libsamplerate --disable-dbus --disable-ime --disable-ibus --disable-fcitx \
            --disable-libudev >/dev/null \
         && make -j"$JOBS" >/dev/null && make install >/dev/null) ;;
    image)
        unpack SDL2_image-2.6.3.tar.gz
        (cd "$WORK/SDL2_image-2.6.3" && ./configure --host="$HOST" --prefix="$PREFIX" \
            --disable-shared --with-sdl-prefix="$PREFIX" --enable-png --enable-stb-image \
            --disable-jpg --disable-tif --disable-webp --disable-avif --disable-jxl --disable-qoi \
            --disable-lbm --disable-pcx --disable-pnm --disable-svg --disable-tga --disable-xcf \
            --disable-xpm --disable-xv --disable-gif --disable-bmp >/dev/null \
         && make -j"$JOBS" >/dev/null && make install >/dev/null) ;;
    mixer)
        unpack SDL2_mixer-2.6.3.tar.gz
        (cd "$WORK/SDL2_mixer-2.6.3" && ./configure --host="$HOST" --prefix="$PREFIX" \
            --disable-shared --with-sdl-prefix="$PREFIX" --enable-music-wave \
            --enable-music-ogg --enable-music-ogg-stb --disable-music-flac --disable-music-mp3 \
            --disable-music-mod --disable-music-midi --disable-music-opus \
            --disable-music-cmd >/dev/null \
         && make -j"$JOBS" >/dev/null && make install >/dev/null) ;;
    ttf)
        # SDL2_ttf 2.0.15: o último que compila com o SDL 2.0.9 sem remendos (o 2.20 já usa
        # funções da 2.0.10) e tem tudo o que os jogos usam. O FreeType vem do pacote do 2.20,
        # que o traz junto (external/freetype), sem zlib, PNG, HarfBuzz nem brotli
        tar -xzf "$SRC/SDL2_ttf-2.20.2.tar.gz" -C "$WORK" SDL2_ttf-2.20.2/external/freetype
        cmake -S "$WORK/SDL2_ttf-2.20.2/external/freetype" -B "$WORK/freetype-build" \
            -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON \
            -DFT_DISABLE_PNG=ON -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON >/dev/null
        cmake --build "$WORK/freetype-build" --parallel "$JOBS" >/dev/null
        cmake --install "$WORK/freetype-build" >/dev/null
        unpack SDL2_ttf-2.0.15.tar.gz
        (cd "$WORK/SDL2_ttf-2.0.15" && ./configure --host="$HOST" --prefix="$PREFIX" \
            --disable-shared --with-sdl-prefix="$PREFIX" --with-freetype-prefix="$PREFIX" \
            >/dev/null && make -j"$JOBS" >/dev/null && make install >/dev/null)
        # Estático, ele precisa do FreeType no link, e o .pc dele não diz
        sed -i 's/-lSDL2_ttf$/-lSDL2_ttf -lfreetype/' "$PREFIX/lib/pkgconfig/SDL2_ttf.pc" ;;
    *)
        echo "deps.sh: pacote desconhecido: $dep" >&2; exit 1 ;;
    esac
    done_ "$dep"
done
rm -rf "$WORK"
