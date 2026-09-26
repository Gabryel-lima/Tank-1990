#!/bin/sh
# ============================================================================
# Tank-1990 - instalacao dentro do WSL (Alpine Linux)
# ============================================================================
#
# Este script roda DENTRO da distribuicao Alpine importada no WSL2.
# Normalmente voce nao o executa a mao: quem chama e o instalar.cmd (Windows).
#
# O que ele faz:
#   1. configura os repositorios do Alpine
#   2. instala o SDL2 e um compilador C++
#   3. compila o jogo
#   4. instala em /opt/tank1990 com o atalho /usr/local/bin/tank1990
#   5. remove o compilador (modo enxuto, padrao) para liberar espaco
#
# Uso:
#   sh tools/wsl-setup.sh                  # instala e remove o compilador
#   sh tools/wsl-setup.sh --keep-toolchain # mantem o compilador (para recompilar)
#
# ============================================================================

set -e

PREFIX="/opt/tank1990"
LAUNCHER="/usr/local/bin/tank1990"
KEEP_TOOLCHAIN=0

for arg in "$@"; do
    case "$arg" in
        --keep-toolchain) KEEP_TOOLCHAIN=1 ;;
        --slim)           KEEP_TOOLCHAIN=0 ;;
        *) echo "Argumento desconhecido: $arg" >&2; exit 2 ;;
    esac
done

# Pacotes necessarios apenas para compilar
BUILD_PKGS="g++ make sdl2-dev sdl2_image-dev sdl2_mixer-dev sdl2_ttf-dev"
# Pacotes necessarios para rodar o jogo
RUNTIME_PKGS="libstdc++ sdl2 sdl2_image sdl2_mixer sdl2_ttf mesa-dri-gallium libpulse"

say()  { printf '\033[36m==>\033[0m %s\n' "$1"; }
ok()   { printf '\033[32m[ OK ]\033[0m %s\n' "$1"; }
die()  { printf '\033[31m[ERRO]\033[0m %s\n' "$1" >&2; exit 1; }

[ "$(id -u)" = "0" ] || die "Este script precisa rodar como root dentro do WSL."
[ -f Makefile ] && [ -d src ] || die "Rode a partir da pasta do projeto (onde esta o Makefile)."

# ---------------------------------------------------------------------------
say "Configurando os repositorios do Alpine"
# ---------------------------------------------------------------------------
BRANCH="$(cut -d. -f1,2 /etc/alpine-release 2>/dev/null || echo latest-stable)"
if [ "$BRANCH" = "latest-stable" ]; then
    REPO_BASE="https://dl-cdn.alpinelinux.org/alpine/latest-stable"
else
    REPO_BASE="https://dl-cdn.alpinelinux.org/alpine/v$BRANCH"
fi
cat > /etc/apk/repositories <<EOF
$REPO_BASE/main
$REPO_BASE/community
EOF
apk update >/dev/null
ok "repositorios prontos ($REPO_BASE)"

# ---------------------------------------------------------------------------
say "Instalando as bibliotecas do jogo (SDL2)"
# ---------------------------------------------------------------------------
# Instalados explicitamente para que continuem no sistema quando o
# compilador for removido no fim.
apk add --no-cache $RUNTIME_PKGS >/dev/null
ok "SDL2 instalado"

say "Instalando o compilador C++ (temporario)"
apk add --no-cache $BUILD_PKGS >/dev/null
ok "compilador instalado"

# ---------------------------------------------------------------------------
say "Compilando o Tank-1990"
# ---------------------------------------------------------------------------
# Compila numa pasta temporaria dentro do Linux: compilar direto em /mnt/c
# (disco do Windows) e varias vezes mais lento.
SRC_DIR="$(pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

cp -r "$SRC_DIR/src" "$SRC_DIR/resources" "$SRC_DIR/Makefile" "$WORK/"
cd "$WORK"
make build

[ -x build/bin/Tanks ] || die "A compilacao nao gerou build/bin/Tanks."
ok "compilado"

# ---------------------------------------------------------------------------
say "Instalando em $PREFIX"
# ---------------------------------------------------------------------------
rm -rf "$PREFIX"
mkdir -p "$PREFIX"
cp -r build/bin/. "$PREFIX/"

cat > "$LAUNCHER" <<'EOF'
#!/bin/sh
# Atalho do Tank-1990 (gerado por tools/wsl-setup.sh)
# O jogo procura os recursos a partir do diretorio atual, por isso o cd.
cd /opt/tank1990 || exit 1

# Ambiente grafico e de audio fornecidos pelo WSLg
export DISPLAY="${DISPLAY:-:0}"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/mnt/wslg/runtime-dir}"
export PULSE_SERVER="${PULSE_SERVER:-unix:/mnt/wslg/PulseServer}"

# O jogo encerra silenciosamente se o SDL_mixer nao abrir o audio
# (ver FIXES.md, item 23). Sem servidor de som, usa o driver mudo.
[ -S /mnt/wslg/PulseServer ] || export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}"

exec ./Tanks "$@"
EOF
chmod +x "$LAUNCHER"
ok "instalado em $PREFIX"

# ---------------------------------------------------------------------------
if [ "$KEEP_TOOLCHAIN" = "0" ]; then
    say "Removendo o compilador para liberar espaco"
    cd /
    apk del --no-cache --purge $BUILD_PKGS >/dev/null 2>&1 || true
    # remove orfaos que sobraram da compilacao
    apk cache clean >/dev/null 2>&1 || true
    rm -rf /var/cache/apk/* /root/.cache 2>/dev/null || true
    ok "compilador removido (rode o instalar.cmd de novo para recompilar)"
else
    say "Compilador mantido (--keep-toolchain)"
    echo "    Para recompilar depois de mexer no codigo:"
    echo "      wsl -d Tank1990 --cd <pasta-do-projeto> -- sh tools/wsl-setup.sh --keep-toolchain"
fi

printf '\n'
ok "Tank-1990 pronto!"
echo "    Espaco usado pela distribuicao: $(du -shx / 2>/dev/null | cut -f1)"
echo "    Para jogar, use o jogar.cmd no Windows (ou 'tank1990' aqui dentro)."
printf '\n'
