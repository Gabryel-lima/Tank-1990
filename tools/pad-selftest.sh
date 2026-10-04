#!/bin/sh
# Teste de ponta a ponta da ponte de controles, sem controle de verdade:
#   padbridge --fake  →  TCP  →  NetPad (dentro do padprobe)  →  controle virtual do SDL
# O padprobe confere se o controle chegou como gamepad, se o botão A, o analógico e o
# gatilho chegaram e se ele some quando a conexão fecha. Funciona no Linux, no macOS e no
# Windows (MSYS2). Ver CONTROLES.md.
#
#   sh tools/pad-selftest.sh [pasta dos executáveis] [porta]
#   make pad-selftest
#
# Sai com 0 se passou.

BIN="${1:-build/bin}"
PORT="${2:-47991}"

EXT=""
[ -f "$BIN/padprobe.exe" ] && EXT=".exe"
if [ ! -f "$BIN/padprobe$EXT" ] || [ ! -f "$BIN/padbridge$EXT" ]; then
    echo "pad-selftest: compile antes: make pad-tools" >&2
    exit 1
fi
cd "$BIN" || exit 1

"./padprobe$EXT" --selftest "$PORT" --seconds 30 &
PROBE=$!
# Dá tempo de o padprobe abrir a porta; a ponte tenta de novo a cada 0,5 s de qualquer jeito
sleep 2
"./padbridge$EXT" --fake --port "$PORT" --once --wait 25
BRIDGE=$?
wait "$PROBE"
RC=$?
echo "pad-selftest: padprobe saiu com $RC, padbridge com $BRIDGE"
exit "$RC"
