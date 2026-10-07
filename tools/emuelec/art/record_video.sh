#!/bin/sh
# Grava o vídeo que o menu do EmuELEC mostra com o Tank 1990 selecionado (gamelist.xml,
# <video>): as partidas de demonstração do menu (campanha, duelo e sobrevivência, todos os
# tanques na IA), sem o menu por cima, com o som do jogo. Ver EMUELEC.md.
#
#   sh tools/emuelec/art/record_video.sh     (grava tools/emuelec/art/Tank1990-video.mp4)
#
# Precisa do Xvfb, do xdotool e do ffmpeg com libx264 (Linux). Roda o tools/attract.cpp (make
# attract) numa tela virtual: o ffmpeg grava a imagem e o driver "disk" do SDL grava o áudio
# em tempo real num arquivo. O tamanho desse arquivo quando a imagem começa a ser gravada diz
# quanto do áudio cortar para os dois ficarem juntos.
set -eu

cd "$(dirname "$0")/../../.."
OUT=tools/emuelec/art/Tank1990-video.mp4
SEED=1990
SWITCH=9000                 # ms de cada modo: 3 modos, ~27 s de vídeo
SECONDS_TOTAL=27
W=928 H=832                 # a tela lógica (464x416) em 2x, como a janela do attract
RATE=48000 CHANNELS=2       # o Mix_OpenAudio do SoundManager: 48 kHz, 16 bits, estéreo

make --no-print-directory attract >/dev/null
TMP=$(mktemp -d)
trap 'kill $XVFB 2>/dev/null; rm -rf "$TMP"' EXIT

# Uma tela virtual livre
DISP=97
while [ -e "/tmp/.X11-unix/X$DISP" ]; do DISP=$((DISP + 1)); done
Xvfb ":$DISP" -screen 0 "${W}x${H}x24" >/dev/null 2>&1 &
XVFB=$!
sleep 1

(cd build/bin && DISPLAY=":$DISP" SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE="$TMP/audio.raw" \
    ./attract --seed "$SEED" --switch "$SWITCH" --scale 2 >/dev/null 2>&1) &
GAME=$!
DISPLAY=":$DISP" xdotool search --sync --name TANKS >/dev/null
DISPLAY=":$DISP" xdotool search --name TANKS windowmove 0 0 >/dev/null 2>&1 || true
sleep 0.3
AUDIO_SKIP=$(wc -c < "$TMP/audio.raw")
ffmpeg -loglevel error -y -f x11grab -framerate 30 -video_size "${W}x${H}" -i ":$DISP+0,0" \
    -t "$SECONDS_TOTAL" -c:v libx264 -preset ultrafast -qp 0 "$TMP/video.mkv"
wait "$GAME" || true

# H.264 (main, yuv420p) com AAC, 640 px de largura, sem suavizar os pixels: o VLC do EmuELEC
# toca isso em qualquer Amlogic. O áudio começa no ponto em que a imagem começou
SKIP_SECONDS=$(awk "BEGIN { printf \"%.3f\", $AUDIO_SKIP / ($RATE * $CHANNELS * 2) }")
ffmpeg -loglevel error -y -i "$TMP/video.mkv" \
    -f s16le -ar "$RATE" -ac "$CHANNELS" -ss "$SKIP_SECONDS" -i "$TMP/audio.raw" \
    -map 0:v -map 1:a -t "$SECONDS_TOTAL" \
    -vf "scale=640:-2:flags=neighbor,format=yuv420p" -c:v libx264 -profile:v main -preset slow \
    -crf 26 -r 30 -c:a aac -b:a 96k -ac 2 -movflags +faststart "$OUT"
echo "Vídeo: $OUT ($(du -h "$OUT" | cut -f1))"
