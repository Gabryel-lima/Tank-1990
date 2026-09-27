#!/bin/sh
# ============================================================================
# Tank-1990 - instalacao dentro do WSL (Alpine Linux)
# ============================================================================
#
# Este script roda DENTRO da distribuicao Alpine importada no WSL2.
# Normalmente voce nao o executa a mao: quem chama e o install.cmd (Windows).
#
# O que ele faz:
#   1. configura os repositorios do Alpine
#   2. instala o SDL2 e o compilador C++
#   3. compila o jogo
#   4. instala em /opt/tank1990 com o atalho /usr/local/bin/tank1990
#   5. mantem o compilador para recompilar (ou o remove, com --slim)
#
# Uso:
#   sh tools/wsl-setup.sh          # instala e mantem o compilador (padrao)
#   sh tools/wsl-setup.sh --slim   # remove o compilador no fim (~150 MB a menos)
#
# ============================================================================

set -e

PREFIX="/opt/tank1990"
LAUNCHER="/usr/local/bin/tank1990"
KEEP_TOOLCHAIN=1

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
# O SDL2 carrega o suporte a X11 com dlopen: sem estas bibliotecas ele nao
# acha nenhum driver de video, roda sem janela e o jogo parece travado.
RUNTIME_PKGS="$RUNTIME_PKGS libx11 libxext libxcursor libxi libxrandr libxfixes libxscrnsaver"
# Sem o libGL/libEGL o SDL cai no modo de software do X11, que no WSLg
# deixa a janela cinza e parada. Com eles, renderiza via OpenGL (Mesa).
RUNTIME_PKGS="$RUNTIME_PKGS mesa-gl mesa-egl"

say()  { printf '\033[36m==>\033[0m %s\n' "$1"; }
ok()   { printf '\033[32m[ OK ]\033[0m %s\n' "$1"; }
die()  { printf '\033[31m[ERRO]\033[0m %s\n' "$1" >&2; exit 1; }

[ "$(id -u)" = "0" ] || die "Este script precisa rodar como root dentro do WSL."
[ -f Makefile ] && [ -d src ] || die "Rode a partir da pasta do projeto (onde esta o Makefile)."

# ---------------------------------------------------------------------------
# Barra de progresso
# ---------------------------------------------------------------------------
# track PID ROTULO SONDA
#   Acompanha o processo PID em segundo plano ate ele terminar. A cada volta
#   chama a funcao SONDA, que imprime "feito total"; com isso desenha uma
#   barra com porcentagem, tempo decorrido e estimativa do que falta. Se a
#   sonda nao imprimir nada (progresso desconhecido), mostra um spinner.
#   Retorna o codigo de saida do processo.

BAR_W=30
T_TOTAL=$(date +%s)
TMP_PROG="$(mktemp -d)"
trap 'rm -rf "$TMP_PROG"' EXIT
[ -t 1 ] && TTY=1 || TTY=0

fmt_time() { printf '%02d:%02d' $(($1 / 60)) $(($1 % 60)); }

repeat() { # repeat N CARACTERE
    _r=''; _i=0
    while [ "$_i" -lt "$1" ]; do _r="$_r$2"; _i=$((_i + 1)); done
    printf '%s' "$_r"
}

draw() { # draw FEITO TOTAL DECORRIDO ROTULO QUADRO
    _elapsed=$(fmt_time "$3")
    if [ -z "$1" ] || [ "${2:-0}" -le 0 ]; then
        printf '\r\033[K    %s %s  %s' "$5" "$4" "$_elapsed"
        return
    fi
    _done=$1; [ "$_done" -gt "$2" ] && _done=$2
    _pct=$((_done * 100 / $2))
    _fill=$((_pct * BAR_W / 100))
    _eta=''
    if [ "$_done" -gt 0 ] && [ "$_done" -lt "$2" ]; then
        _eta="  resta ~$(fmt_time $(($3 * ($2 - _done) / _done)))"
    fi
    printf '\r\033[K    \033[32m%s\033[90m%s\033[0m %3d%%  %s  %s%s' \
        "$(repeat "$_fill" '█')" "$(repeat $((BAR_W - _fill)) '░')" \
        "$_pct" "$4" "$_elapsed" "$_eta"
}

track() { # track PID ROTULO SONDA
    _pid=$1; _label=$2; _probe=$3
    _start=$(date +%s); _n=0
    while kill -0 "$_pid" 2>/dev/null; do
        if [ "$TTY" = 1 ]; then
            set -- $($_probe)
            case $((_n % 4)) in 0) _f='|' ;; 1) _f='/' ;; 2) _f='-' ;; 3) _f='\' ;; esac
            draw "$1" "$2" $(($(date +%s) - _start)) "$_label" "$_f"
        fi
        _n=$((_n + 1))
        sleep 0.2
    done
    _rc=0; wait "$_pid" || _rc=$?
    [ "$TTY" = 1 ] && printf '\r\033[K'
    LAST_TIME=$(fmt_time $(($(date +%s) - _start)))
    return "$_rc"
}

# apk escreve "feito/total" no descritor passado em --progress-fd
apk_probe() { tail -n 1 "$TMP_PROG/apk" 2>/dev/null | tr '/' ' '; }

# apk_bar ROTULO ARGS... -> roda "apk ARGS" com barra de progresso
apk_bar() {
    _l=$1; shift
    : > "$TMP_PROG/apk"
    apk --progress-fd 3 "$@" 3>"$TMP_PROG/apk" >"$TMP_PROG/log" 2>&1 &
    if ! track $! "$_l" apk_probe; then
        cat "$TMP_PROG/log" >&2
        die "falha ao executar: apk $*"
    fi
}

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
apk_bar "baixando indice de pacotes" update
ok "repositorios prontos ($REPO_BASE) em $LAST_TIME"

# ---------------------------------------------------------------------------
say "Instalando as bibliotecas do jogo (SDL2)"
# ---------------------------------------------------------------------------
# Instalados explicitamente para que continuem no sistema quando o
# compilador for removido no fim.
apk_bar "SDL2" add --no-cache $RUNTIME_PKGS
ok "SDL2 instalado em $LAST_TIME"

say "Instalando o compilador C++"
apk_bar "compilador" add --no-cache $BUILD_PKGS
ok "compilador instalado em $LAST_TIME"

# ---------------------------------------------------------------------------
say "Compilando o Tank-1990"
# ---------------------------------------------------------------------------
# Compila numa pasta temporaria dentro do Linux: compilar direto em /mnt/c
# (disco do Windows) e varias vezes mais lento.
SRC_DIR="$(pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK" "$TMP_PROG"' EXIT

cp -r "$SRC_DIR/src" "$SRC_DIR/resources" "$SRC_DIR/Makefile" "$WORK/"
cd "$WORK"

# Progresso = arquivos .o gerados + 1 passo de linkagem (o executavel)
N_SRC=$(find src -name '*.cpp' | wc -l)
make_probe() {
    _o=$(find build -name '*.o' 2>/dev/null | wc -l)
    [ -x build/bin/Tanks ] && _o=$((_o + 1))
    echo "$_o $((N_SRC + 1))"
}
make build -j"$(nproc)" >"$TMP_PROG/make.log" 2>&1 &
if ! track $! "compilando $N_SRC arquivos" make_probe; then
    tail -n 40 "$TMP_PROG/make.log" >&2
    die "A compilacao falhou (mensagens acima)."
fi

[ -x build/bin/Tanks ] || die "A compilacao nao gerou build/bin/Tanks."
ok "compilado em $LAST_TIME"

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

# Usa o X11 do WSLg. Se ele nao estiver disponivel o SDL falha e o jogo
# fecha, em vez de cair num driver sem janela e parecer travado.
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}"

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
    apk --progress-fd 3 del --no-cache --purge $BUILD_PKGS \
        3>"$TMP_PROG/apk" >/dev/null 2>&1 &
    track $! "removendo pacotes" apk_probe || true
    # remove orfaos que sobraram da compilacao
    apk cache clean >/dev/null 2>&1 || true
    rm -rf /var/cache/apk/* /root/.cache 2>/dev/null || true
    ok "compilador removido em $LAST_TIME (rode o install.cmd sem --slim para recompilar)"
else
    say "Compilador mantido (use --slim para remove-lo)"
    echo "    Para recompilar depois de mexer no codigo, rode o install.cmd de novo"
    echo "    (ele reaproveita o compilador e so recompila o jogo)."
fi

printf '\n'
ok "Tank-1990 pronto! (tempo total no WSL: $(fmt_time $(($(date +%s) - T_TOTAL))))"
echo "    Espaco usado pela distribuicao: $(du -shx / 2>/dev/null | cut -f1)"
echo "    Para jogar, use o play.cmd no Windows (ou 'tank1990' aqui dentro)."
printf '\n'
