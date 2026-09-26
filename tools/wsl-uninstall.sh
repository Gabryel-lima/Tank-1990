#!/bin/sh
# ============================================================================
# Tank-1990 - remove o jogo de dentro da distribuicao WSL
# ============================================================================
#
# Apaga apenas o jogo e suas bibliotecas; a distribuicao Alpine continua
# instalada. Para remover tudo, use o desinstalar.cmd no Windows.
#
# Uso:  sh tools/wsl-uninstall.sh
#
# ============================================================================

set -e

PREFIX="/opt/tank1990"
LAUNCHER="/usr/local/bin/tank1990"
PKGS="sdl2 sdl2_image sdl2_mixer sdl2_ttf mesa-dri-gallium"

say() { printf '\033[36m==>\033[0m %s\n' "$1"; }
ok()  { printf '\033[32m[ OK ]\033[0m %s\n' "$1"; }

[ "$(id -u)" = "0" ] || { echo "Precisa rodar como root." >&2; exit 1; }

say "Removendo os arquivos do jogo"
rm -rf "$PREFIX" "$LAUNCHER"
ok "$PREFIX removido"

say "Removendo as bibliotecas do SDL2"
# shellcheck disable=SC2086
apk del --purge $PKGS >/dev/null 2>&1 || true
rm -rf /var/cache/apk/* 2>/dev/null || true
ok "bibliotecas removidas"

printf '\n'
ok "Jogo desinstalado. A distribuicao Alpine continua no WSL."
echo "    Para remove-la tambem:  wsl --unregister Tank1990"
printf '\n'
