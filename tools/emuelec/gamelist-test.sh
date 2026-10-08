#!/bin/sh
# Testa o gamelist-merge.sh: a entrada do jogo entra no gamelist.xml dos Ports sem apagar a dos
# outros jogos, sem duplicar e sem perder o que a pessoa já tinha (favorito, partidas). Roda no
# PC, sem o EmuELEC; a API do EmulationStation é imitada com um servidor em Python, se houver.
# Testa também o install.sh (o pacote vai para o stick pela rede), com um ssh de mentira, e o
# card-install.sh (o pacote vai para o cartão no PC), com um cartão de mentira, e o lançador com um binário sem permissão de executar.
#
#   sh tools/emuelec/gamelist-test.sh   (ou make gamelist-test)
set -eu

cd "$(dirname "$0")"
MERGE="$(pwd)/gamelist-merge.sh"
ENTRY="$(pwd)/$(ls gamelist*.xml | head -n 1)"
T=$(mktemp -d)
trap 'kill "$server" 2>/dev/null; rm -rf "$T"' EXIT
server=
fail() { echo "gamelist-test: $*" >&2; exit 1; }
count() { grep -c "$1" "$2" || true; }

game_path=$(sed -n 's|.*<path>\(.*\)</path>.*|\1|p' "$ENTRY" | head -n 1)
desc_line=$(grep -m 1 '<desc>' "$ENTRY")
export HOME="$T/home" GAMELIST_API=""
mkdir -p "$T/ps" "$HOME"
merge() { sh "$MERGE" "$1" "$T/ps" "$T/stamp" > /dev/null; }

# Outro port, que já estava no cartão
cat > "$T/other.xml" <<'EOF'
<?xml version="1.0"?>
<gameList>
	<game>
		<path>./Other.sh</path>
		<name>Other Port</name>
		<desc>Not ours &amp; never touched.</desc>
	</game>
</gameList>
EOF

# Sem gamelist.xml: cria um só com o nosso jogo
merge "$ENTRY"
grep -qF "<path>$game_path</path>" "$T/ps/gamelist.xml" || fail "não criou o gamelist.xml"
grep -qF "$desc_line" "$T/ps/gamelist.xml" || fail "a descrição não entrou"

# Com o gamelist.xml de outro port e uma entrada velha nossa, que a pessoa já jogou
cat > "$T/ps/gamelist.xml" <<EOF
<?xml version="1.0"?>
<gameList>
	<game>
		<path>./Other.sh</path>
		<name>Other Port</name>
		<desc>Not ours &amp; never touched.</desc>
	</game>
	<game>
		<path>$game_path</path>
		<name>Old name</name>
		<favorite>true</favorite>
		<playcount>7</playcount>
	</game>
	<folder>
		<path>./images</path>
		<hidden>true</hidden>
	</folder>
</gameList>
EOF
rm -f "$T/stamp"
merge "$ENTRY"
L="$T/ps/gamelist.xml"
[ "$(count "<path>$game_path</path>" "$L")" = 1 ] || fail "a entrada ficou duplicada ou sumiu"
grep -qF 'Not ours &amp; never touched.' "$L" || fail "apagou ou mudou o outro port"
grep -q '<path>./images</path>' "$L" || fail "apagou a pasta"
grep -q 'Old name' "$L" && fail "a entrada velha ficou"
grep -q '<playcount>7</playcount>' "$L" || fail "perdeu as partidas"
grep -q '<favorite>true</favorite>' "$L" || fail "perdeu o favorito"
grep -qF "$desc_line" "$L" || fail "a descrição não entrou"
tail -n 1 "$L" | grep -q '^</gameList>$' || fail "o arquivo não fecha no </gameList>"

# De novo, com a marca: não mexe no arquivo
cp "$L" "$T/before"
merge "$ENTRY"
cmp -s "$L" "$T/before" || fail "mexeu num gamelist.xml que já estava certo"

# O EmulationStation regravou a entrada sem a arte: refaz, mesmo com a marca
sed -i.bak '/<image>/d' "$L"
merge "$ENTRY"
[ "$(count '<image>' "$L")" = 1 ] || fail "não refez a entrada sem a arte"

# Um segundo jogo, com a sua marca: os dois ficam
STAMP2="$T/stamp2"
sh "$MERGE" "$T/other.xml" "$T/ps" "$STAMP2" > /dev/null
[ "$(count '<path>./Other.sh</path>' "$L")" = 1 ] || fail "o outro jogo duplicou"
grep -qF "<path>$game_path</path>" "$L" || fail "o segundo jogo apagou o primeiro"

# Arquivo que não dá para entender: não mexe
echo 'lixo' > "$T/ps/gamelist.xml"
rm -f "$T/stamp"
if merge "$ENTRY" 2> /dev/null; then fail "aceitou um gamelist.xml estragado"; fi
[ "$(cat "$T/ps/gamelist.xml")" = lixo ] || fail "mexeu num gamelist.xml estragado"

# Sem o gamelist.xml na pasta e com o da configuração do EmulationStation: usa esse
rm -f "$T/ps/gamelist.xml" "$T/stamp"
mkdir -p "$HOME/.emulationstation/gamelists/ports"
cp "$T/other.xml" "$HOME/.emulationstation/gamelists/ports/gamelist.xml"
merge "$ENTRY"
[ ! -f "$T/ps/gamelist.xml" ] || fail "criou outro gamelist.xml em vez de usar o do EmulationStation"
grep -qF "<path>$game_path</path>" "$HOME/.emulationstation/gamelists/ports/gamelist.xml" ||
    fail "não gravou no gamelist.xml do EmulationStation"
rm -rf "$HOME/.emulationstation"

# Pela API do EmulationStation: manda a entrada (com as partidas) e não mexe no arquivo
if command -v python3 > /dev/null && command -v curl > /dev/null; then
    cp "$T/before" "$T/ps/gamelist.xml"
    rm -f "$T/stamp"
    python3 -I - "$T" <<'EOF' &
import http.server, sys, os
out = sys.argv[1]
class H(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        body = self.rfile.read(int(self.headers["Content-Length"]))
        open(os.path.join(out, "posted-" + self.path.strip("/").replace("/", "-")), "wb").write(body)
        self.send_response(200); self.end_headers(); self.wfile.write(b"OK")
    def log_message(self, *a): pass
s = http.server.HTTPServer(("127.0.0.1", 0), H)
open(os.path.join(out, "port"), "w").write(str(s.server_port))
s.serve_forever()
EOF
    server=$!
    i=0; while [ ! -s "$T/port" ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i + 1)); done
    GAMELIST_API="http://127.0.0.1:$(cat "$T/port")" merge "$ENTRY"
    P="$T/posted-addgames-ports"
    [ -f "$P" ] || fail "não chamou /addgames/ports"
    cmp -s "$T/ps/gamelist.xml" "$T/before" || fail "mexeu no arquivo com a API respondendo"
    grep -q '<playcount>7</playcount>' "$P" || fail "a API não recebeu as partidas"
    grep -qF "$desc_line" "$P" || fail "a API não recebeu a descrição"
    [ "$(count '<game>' "$P")" = 1 ] || fail "a API recebeu mais de um jogo"
else
    echo "gamelist-test: sem python3 ou curl, a API do EmulationStation não foi testada"
fi

# O install.sh: copia o pacote para as ROMs do stick e junta a entrada ao gamelist.xml dos
# Ports, sem apagar o que já estava lá. O ssh de mentira roda o comando aqui, em $T/roms
INSTALL="$(pwd)/install.sh"
R="$T/roms" P="$T/pkg"
n=$(sed -n 's|.*<path>\./\(.*\)\.sh</path>.*|\1|p' "$ENTRY" | head -n 1 | tr 'A-Z' 'a-z')
mkdir -p "$P/ports/$n" "$P/ports_scripts" "$R/ports/$n" "$R/ports/other" "$R/ports_scripts"
cp "$ENTRY" "$P/ports/$n/gamelist-entry.xml"
cp "$MERGE" "$P/ports/$n/gamelist-merge.sh"
echo jogo > "$P/ports/$n/bin"
echo atalho > "$P/ports_scripts/$(basename "$game_path")"
mkdir -p "$P/ports_scripts/images" && echo imagem > "$P/ports_scripts/images/x.png"
echo outro > "$R/ports/other/bin"
cp "$T/other.xml" "$R/ports_scripts/gamelist.xml"
cksum < "$ENTRY" > "$R/ports/$n/.gamelist-stamp"
(cd "$P" && python3 -I -m zipfile -c "$T/pkg.zip" ports ports_scripts) ||
    fail "sem python3 para montar o pacote de teste"
printf '%s\n' '#!/bin/sh' '[ "$1" = root@stick ] || exit 255' 'exec sh -c "$2"' > "$T/fake-ssh"
chmod +x "$T/fake-ssh"
EMUELEC_SSH="$T/fake-ssh" EMUELEC_ROMS="$R" sh "$INSTALL" stick "$T/pkg.zip" > /dev/null ||
    fail "o install.sh falhou"
[ "$(cat "$R/ports/$n/bin")" = jogo ] || fail "o install.sh não copiou ports/$n"
[ -f "$R/ports_scripts/$(basename "$game_path")" ] || fail "o install.sh não copiou o atalho"
[ "$(cat "$R/ports/other/bin")" = outro ] || fail "o install.sh apagou outro port"
grep -qF "<path>$game_path</path>" "$R/ports_scripts/gamelist.xml" ||
    fail "o install.sh não juntou a entrada"
grep -q '<path>./Other.sh</path>' "$R/ports_scripts/gamelist.xml" ||
    fail "o install.sh apagou a entrada de outro port"
if EMUELEC_SSH="$T/fake-ssh" sh "$INSTALL" "" "$T/pkg.zip" > /dev/null 2>&1; then
    fail "o install.sh rodou sem o IP do stick"
fi

# O card-install.sh: com o cartão no PC, o jogo vai para a partição das ROMs e os atalhos para a
# camada de cima do overlay (STORAGE/.config/emuelec/ports), onde um whiteout do overlay dá
# lugar ao arquivo do pacote e os dos outros ports ficam como estavam
CARD="$T/card"
CR="$CARD/ROMS/roms" CU="$CARD/STORAGE/.config/emuelec/ports"
mkdir -p "$CR/ports/other" "$CR/ports_scripts" "$CU"
echo outro > "$CR/ports/other/bin"
echo vendido > "$CU/Vendor.sh"
whiteout() { mknod "$1" c 0 0 2>/dev/null || sudo -n mknod "$1" c 0 0 2>/dev/null; }
wo=
if whiteout "$CU/gamelist.xml" && whiteout "$CU/Removed.sh"; then wo=1; fi
CARD_SUDO= sh "$(pwd)/card-install.sh" "$T/pkg.zip" "$CARD" > /dev/null || fail "o card-install.sh falhou"
sh_name=$(basename "$game_path")
[ "$(cat "$CR/ports/$n/bin")" = jogo ] || fail "o card-install.sh não copiou ports/$n"
[ "$(cat "$CR/ports/other/bin")" = outro ] || fail "o card-install.sh apagou outro port"
[ "$(cat "$CU/$sh_name")" = atalho ] || fail "o card-install.sh não pôs o atalho na STORAGE"
[ -f "$CU/images/x.png" ] || fail "o card-install.sh não pôs as imagens na STORAGE"
[ -f "$CR/ports_scripts/$sh_name" ] || fail "o card-install.sh não pôs o atalho nas ROMs"
[ "$(cat "$CU/Vendor.sh")" = vendido ] || fail "o card-install.sh mexeu no atalho de outro port"
for L in "$CU/gamelist.xml" "$CR/ports_scripts/gamelist.xml"; do
    [ -f "$L" ] || fail "o card-install.sh não criou $L"
    [ "$(count "<path>$game_path</path>" "$L")" = 1 ] || fail "a entrada não entrou em $L"
done
if [ -n "$wo" ]; then
    [ -c "$CU/Removed.sh" ] || fail "o card-install.sh mexeu num whiteout que não era dele"
else
    echo "gamelist-test: sem mknod, o whiteout do overlay não foi testado"
fi
# De novo: não duplica
CARD_SUDO= sh "$(pwd)/card-install.sh" "$T/pkg.zip" "$CARD" > /dev/null || fail "o card-install.sh falhou na segunda vez"
[ "$(count "<path>$game_path</path>" "$CU/gamelist.xml")" = 1 ] || fail "o card-install.sh duplicou a entrada"

# O lançador: a partição das ROMs é FAT e, montada sem permissão de executar, o binário dá
# "Permission denied" (saída 126). Um binário sem o bit de execução tem de rodar mesmo assim, de
# uma cópia fora do cartão, com a pasta de trabalho no jogo
L=$(grep -l '^GAMEDIR=' ./*.sh | head -n 1)
L="$(pwd)/$(basename "$L")"
ln_name=$(basename "$L" .sh | tr 'A-Z' 'a-z')
bin=$(sed -n 's|.*cp \./\([A-Za-z0-9]*\) "\$RUN.*|\1|p' "$L" | head -n 1)
[ -f "$L" ] && [ -n "$bin" ] || fail "não achei o binário no lançador $L"
PL="$T/launch"
mkdir -p "$PL/ports_scripts" "$PL/ports/$ln_name"
cp "$L" "$PL/ports_scripts/"
cp "$ENTRY" "$PL/ports/$ln_name/gamelist-entry.xml"
cp "$MERGE" "$PL/ports/$ln_name/"
# O gamecontrollerdb.txt do EmuELEC passa de 128 KiB, o máximo de UMA variável de ambiente no
# Linux: exportado inteiro, todo exec depois (mkdir, cp, o jogo) falha com "Argument list too long".
# O jogo recebe só os mapeamentos dos controles ligados (os GUIDs vêm no --controllers) e os de Linux
G=03000000bc2000000055000011010000
awk -v g="$G" 'BEGIN { for (i = 0; i < 2500; i++) printf "%032x,Pad %d,a:b0,b:b1,x:b2,y:b3,platform:Linux,\n", i + 1, i
    printf "%s,Twin USB,a:b0,b:b1,platform:Linux,\n", g }' > "$T/db.txt"
[ "$(wc -c < "$T/db.txt")" -gt 131072 ] || fail "o gamecontrollerdb.txt de teste devia passar de 128 KiB"
printf '#!/bin/sh\necho "rodou em $(pwd)"\necho "cfg=${#SDL_GAMECONTROLLERCONFIG}"\necho "primeira=$(echo "$SDL_GAMECONTROLLERCONFIG" | head -n 1)"\nexit 7\n' > "$PL/ports/$ln_name/$bin"
chmod 644 "$PL/ports/$ln_name/$bin"
launch_status=0
(cd "$PL" && GAMECONTROLLERDB="$T/db.txt" EMUELEC_RUN_DIRS="/proc/nao-existe $T/run" \
    bash "ports_scripts/$(basename "$L")" -Pports --core= --emulator= \
    "--controllers=-p1index 0 -p1guid $G -p2index 1 -p2guid $G " > /dev/null 2>&1) || launch_status=$?
grep -q "rodou em .*/ports/$ln_name\$" "$PL/ports/$ln_name/log.txt" 2> /dev/null ||
    fail "o lançador não rodou o binário sem permissão de executar: $(cat "$PL/ports/$ln_name/log.txt" 2> /dev/null)"
# O primeiro lugar falhou (o erro vai para o log) e o jogo rodou no segundo; o código de saída do
# jogo (7) passa adiante, e só 126 ("não executa") tenta outro lugar
grep -q "nao-existe" "$PL/ports/$ln_name/log.txt" || fail "o lançador não registrou o erro do primeiro lugar"
grep -q "^launcher: $T/run/" "$PL/ports/$ln_name/log.txt" || fail "o lançador não usou o segundo lugar"
cfg=$(sed -n 's/^cfg=//p' "$PL/ports/$ln_name/log.txt")
[ -n "$cfg" ] && [ "$cfg" -gt 0 ] && [ "$cfg" -le 100000 ] || fail "o jogo recebeu um SDL_GAMECONTROLLERCONFIG de $cfg bytes (esperado de 1 a 100000)"
grep -q "^primeira=$G,Twin USB" "$PL/ports/$ln_name/log.txt" || fail "o mapeamento do controle ligado não veio primeiro"
[ "$launch_status" = 7 ] || fail "o lançador não devolveu o código de saída do jogo: $launch_status"
[ "$(count 'rodou em' "$PL/ports/$ln_name/log.txt")" = 1 ] || fail "o lançador rodou o jogo mais de uma vez"

# O gamelist-merge.sh num sistema sem cksum (o busybox do EmuELEC): usa md5sum ou sha1sum
NB="$T/nocksum"
mkdir -p "$NB"
for c in sed head awk grep cat rm cp md5sum sha1sum cut echo; do
    p=$(command -v "$c" 2> /dev/null) && [ -x "$p" ] && ln -sf "$p" "$NB/$c"
done
rm -f "$T/stamp" "$T/ps/gamelist.xml"
out=$(PATH="$NB" "$(command -v sh)" "$MERGE" "$ENTRY" "$T/ps" "$T/stamp" 2>&1) || fail "o merge falhou sem cksum: $out"
case $out in *"not found"*) fail "o merge reclamou de comando faltando: $out" ;; esac
[ -s "$T/stamp" ] || fail "o merge sem cksum não gravou a marca"

echo "gamelist-test: a entrada entra no gamelist.xml sem apagar nem duplicar nada, também pelo install.sh e pelo card-install.sh"
