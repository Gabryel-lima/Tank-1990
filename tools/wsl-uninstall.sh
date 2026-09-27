#!/bin/sh
# ============================================================================
# Tank-1990 - remove o jogo de dentro da distribuicao WSL
# ============================================================================
#
# Apaga o jogo, o compilador e todas as bibliotecas que o install.cmd
# instalou; a distribuicao Alpine continua instalada. Para remover tudo,
# use o uninstall.cmd no Windows.
#
# Uso:  sh tools/wsl-uninstall.sh
#
# ============================================================================

set -e

PREFIX="/opt/tank1990"
LAUNCHER="/usr/local/bin/tank1990"
# Mesma lista do wsl-setup.sh (compilador + bibliotecas do jogo)
PKGS="g++ make sdl2-dev sdl2_image-dev sdl2_mixer-dev sdl2_ttf-dev
      sdl2 sdl2_image sdl2_mixer sdl2_ttf mesa-dri-gallium libpulse
      libx11 libxext libxcursor libxi libxrandr libxfixes libxscrnsaver
      mesa-gl mesa-egl libusb libudev-zero
      cmake samurai linux-headers libusb-dev libudev-zero-dev pulseaudio-dev
      libx11-dev libxext-dev libxcursor-dev libxi-dev libxrandr-dev libxfixes-dev libxscrnsaver-dev"

say() { printf '\033[36m==>\033[0m %s\n' "$1"; }
ok()  { printf '\033[32m[ OK ]\033[0m %s\n' "$1"; }

[ "$(id -u)" = "0" ] || { echo "Precisa rodar como root." >&2; exit 1; }

say "Removendo os arquivos do jogo"
rm -rf "$PREFIX" "$LAUNCHER" /opt/tank1990-sdl2
ok "$PREFIX removido"

say "Removendo o compilador e as bibliotecas"
# shellcheck disable=SC2086
apk del --purge $PKGS >/dev/null 2>&1 || true
rm -rf /var/cache/apk/* 2>/dev/null || true
ok "compilador e bibliotecas removidos"

printf '\n'
ok "Jogo desinstalado. A distribuicao Alpine continua no WSL."
echo "    Para remove-la tambem:  wsl --unregister Tank1990"
printf '\n'
