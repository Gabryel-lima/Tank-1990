#!/bin/sh
# Testa o gamelist-merge.sh: a entrada do jogo entra no gamelist.xml dos Ports sem apagar a dos
# outros jogos, sem duplicar e sem perder o que a pessoa já tinha (favorito, partidas). Roda no
# PC, sem o EmuELEC; a API do EmulationStation é imitada com um servidor em Python, se houver.
# Testa também o install.sh (o pacote vai para o stick pela rede), com um ssh de mentira.
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

echo "gamelist-test: a entrada entra no gamelist.xml sem apagar nem duplicar nada, também pelo install.sh"
