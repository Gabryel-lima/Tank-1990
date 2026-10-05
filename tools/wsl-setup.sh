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
#   4. compila um SDL2 com suporte a controles Xbox (libusb), uma vez so
#   5. compila a ponte de controles Bluetooth (padbridge.exe, para o Windows)
#   6. instala em /opt/tank1990 com o atalho /usr/local/bin/tank1990
#   7. mantem o compilador para recompilar (ou o remove, com --slim)
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
# ... e para compilar o SDL2 com suporte a controles (ver "SDL2 com controles")
BUILD_PKGS="$BUILD_PKGS cmake samurai linux-headers libusb-dev libudev-zero-dev pulseaudio-dev
            libx11-dev libxext-dev libxcursor-dev libxi-dev libxrandr-dev libxfixes-dev libxscrnsaver-dev"
# Pacotes necessarios para rodar o jogo
RUNTIME_PKGS="libstdc++ sdl2 sdl2_image sdl2_mixer sdl2_ttf mesa-dri-gallium libpulse"
# O SDL2 carrega o suporte a X11 com dlopen: sem estas bibliotecas ele nao
# acha nenhum driver de video, roda sem janela e o jogo parece travado.
RUNTIME_PKGS="$RUNTIME_PKGS libx11 libxext libxcursor libxi libxrandr libxfixes libxscrnsaver"
# Sem o libGL/libEGL o SDL cai no modo de software do X11, que no WSLg
# deixa a janela cinza e parada. Com eles, renderiza via OpenGL (Mesa).
RUNTIME_PKGS="$RUNTIME_PKGS mesa-gl mesa-egl"
# Controles: o SDL usa o libudev para achar os controles (o libudev-zero
# funciona sem o daemon udev, que o WSL nao tem) e o libusb para falar com
# controles Xbox, que nao tem driver no kernel do WSL.
RUNTIME_PKGS="$RUNTIME_PKGS libusb libudev-zero"

# SDL2 compilado com libusb, guardado fora de $PREFIX para sobreviver
# as reinstalacoes (so e recompilado quando a versao do SDL2 muda)
SDL_CACHE="/opt/tank1990-sdl2"

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

cp -r "$SRC_DIR/src" "$SRC_DIR/resources" "$SRC_DIR/tools" "$SRC_DIR/Makefile" "$WORK/"
cd "$WORK"

# Clone feito no Windows com CRLF (antes do .gitattributes, ou com core.autocrlf=true): o sh
# para nos scripts que o make chama. O compilador e o jogo aceitam CRLF nos .cpp e nos
# mapas; os scripts de shell, nao (o Makefile vai junto, para nao depender da versao do make)
find . \( -name Makefile -o -name '*.sh' \) -type f | while read -r _f; do
    tr -d '\r' < "$_f" > "$_f.lf" && mv "$_f.lf" "$_f"
done

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
say "SDL2 com suporte a controles"
# ---------------------------------------------------------------------------
# O SDL2 do Alpine nao vem com libusb, e sem ele os controles Xbox nao
# funcionam no WSL (o kernel do WSL nao tem o driver xpad). Compilamos a
# MESMA versao do SDL2 do sistema, com libusb, e o jogo a usa no lugar da
# original (LD_LIBRARY_PATH no atalho). Mesma versao = mesma ABI, entao o
# SDL2_image/mixer/ttf do sistema continuam funcionando com ela.
SDL_VER="$(pkg-config --modversion sdl2)"
SDL_DIR="$SDL_CACHE/$SDL_VER"
if [ -f "$SDL_DIR/lib/libSDL2-2.0.so.0" ]; then
    ok "SDL2 $SDL_VER com controles ja compilado (reaproveitado)"
else
    sdl_build() {
        set -e
        cd "$WORK"
        wget -q "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VER/SDL2-$SDL_VER.tar.gz"
        tar xzf "SDL2-$SDL_VER.tar.gz"
        cmake -S "SDL2-$SDL_VER" -B sdl-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$SDL_DIR" -DSDL_STATIC=OFF -DSDL_TEST=OFF \
            -DSDL_HIDAPI=ON -DSDL_HIDAPI_LIBUSB=ON \
            -DSDL_WAYLAND=OFF -DSDL_KMSDRM=OFF -DSDL_JACK=OFF -DSDL_PIPEWIRE=OFF \
            -DSDL_ALSA=OFF -DSDL_SNDIO=OFF
        cmake --build sdl-build -j"$(nproc)"
        rm -rf "$SDL_CACHE"
        cmake --install sdl-build
    }
    no_probe() { :; }
    ( sdl_build ) >"$TMP_PROG/sdl.log" 2>&1 &
    if track $! "baixando e compilando o SDL2 $SDL_VER" no_probe; then
        ok "SDL2 $SDL_VER com controles compilado em $LAST_TIME"
    else
        tail -n 20 "$TMP_PROG/sdl.log" >&2
        printf '\033[33m[AVISO]\033[0m %s\n' "Falha ao compilar o SDL2 com controles; o jogo usa o do sistema (sem controles Xbox)."
    fi
fi

# ---------------------------------------------------------------------------
say "Ponte de controles Bluetooth (padbridge.exe para o Windows)"
# ---------------------------------------------------------------------------
# O WSL nao enxerga os controles Bluetooth (so o Windows os enxerga). A ponte
# roda no Windows, le os controles com o SDL e manda o estado para o jogo aqui
# dentro (ver CONTROLES.md). Ela e compilada aqui mesmo, com o MinGW, para nao
# pedir compilador no Windows. Sem ela, o jogo funciona com teclado e com
# controles por cabo (gamepads.cmd).
BRIDGE_SDL_VER="2.32.10"
BRIDGE_SDL="/opt/tank1990-sdl2-mingw/$BRIDGE_SDL_VER"
BUILD_PKGS="$BUILD_PKGS mingw-w64-gcc"
bridge_build() {
    set -e
    cd "$WORK"
    # SDL2 para MinGW (o oficial, ~14 MB), guardado fora de $PREFIX como o outro SDL2
    if [ ! -d "$BRIDGE_SDL/x86_64-w64-mingw32/include/SDL2" ]; then
        wget -q "https://github.com/libsdl-org/SDL/releases/download/release-$BRIDGE_SDL_VER/SDL2-devel-$BRIDGE_SDL_VER-mingw.tar.gz"
        rm -rf /opt/tank1990-sdl2-mingw
        mkdir -p "$BRIDGE_SDL"
        tar xzf "SDL2-devel-$BRIDGE_SDL_VER-mingw.tar.gz" -C "$BRIDGE_SDL" --strip-components=1
    fi
    make padbridge-win SDL2_MINGW="$BRIDGE_SDL/x86_64-w64-mingw32"
}
no_probe() { :; }
BRIDGE_OK=0
if apk add --no-cache mingw-w64-gcc >"$TMP_PROG/bridge.log" 2>&1; then
    ( bridge_build ) >>"$TMP_PROG/bridge.log" 2>&1 &
    if track $! "compilando a ponte" no_probe; then
        BRIDGE_OK=1
        ok "ponte compilada em $LAST_TIME"
    fi
fi
if [ "$BRIDGE_OK" = "0" ]; then
    tail -n 15 "$TMP_PROG/bridge.log" >&2
    printf '\033[33m[AVISO]\033[0m %s\n' "A ponte nao compilou; controles Bluetooth nao chegam ao jogo (teclado e cabo USB funcionam)."
fi

# ---------------------------------------------------------------------------
say "Instalando em $PREFIX"
# ---------------------------------------------------------------------------
rm -rf "$PREFIX"
mkdir -p "$PREFIX"
cp -r build/bin/. "$PREFIX/"
# A ponte vai para o Windows: o install.cmd a copia de $PREFIX/windows
if [ "$BRIDGE_OK" = "1" ]; then
    mkdir -p "$PREFIX/windows"
    cp build/win/padbridge.exe build/win/SDL2.dll "$PREFIX/windows/"
fi
if [ -f "$SDL_DIR/lib/libSDL2-2.0.so.0" ]; then
    mkdir -p "$PREFIX/lib"
    cp -P "$SDL_DIR"/lib/libSDL2-2.0.so* "$PREFIX/lib/"
fi

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
# Com o WSL recem-iniciado o WSLg leva ~1-2 s para criar o socket do
# PulseAudio: espera ate 5 s antes de desistir, senao o jogo abre mudo.
_i=0
while [ ! -S /mnt/wslg/PulseServer ] && [ "$_i" -lt 50 ]; do
    sleep 0.1
    _i=$((_i + 1))
done
[ -S /mnt/wslg/PulseServer ] || export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}"

# Controles repassados pelo usbipd (gamepads.cmd / play.cmd no Windows).
# Sem udev no WSL, ninguem carrega os drivers quando o controle chega:
# carrega antes, para que o kernel ja os use no momento da conexao.
for _m in vhci-hcd usbhid hid-generic hid-microsoft evdev joydev; do
    modprobe -q "$_m" 2>/dev/null || true
done
# SDL2 compilado com libusb (controles Xbox). O SDL so usa o libusb para
# uma lista minima de aparelhos; desligar a lista libera os Xbox.
[ -d /opt/tank1990/lib ] && export LD_LIBRARY_PATH="/opt/tank1990/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export SDL_HIDAPI_LIBUSB_WHITELIST="${SDL_HIDAPI_LIBUSB_WHITELIST:-0}"

# Caminho absoluto: o play.cmd acha o processo por ele (pkill -f)
exec /opt/tank1990/Tanks "$@"
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
