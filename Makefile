# ============================================================================
# Tank-1990 - Makefile (Linux / macOS / Windows)
# ============================================================================
#
# INSTRUÇÕES DE USO:
#
# Para compilar o jogo:
#   make build       # Compila o projeto completo
#   make             # Mesmo que "make build"
#
# Para executar o jogo:
#   make run         # Compila (se necessário) e executa o jogo
#
# Para limpar arquivos de build:
#   make clean       # Remove diretório build/
#
# Para gerar documentação:
#   make doc         # Gera documentação com Doxygen
#
# Para ver informações do sistema:
#   make info        # Mostra configurações de compilação
#
# Para instalar dependências:
#   make install-deps
#
# ----------------------------------------------------------------------------
# WINDOWS
# ----------------------------------------------------------------------------
# No Windows, o caminho recomendado é o WSL: execute o install.cmd na raiz do
# projeto. Ele cria uma distribuição Alpine Linux mínima no WSL2, compila o
# jogo com este mesmo Makefile e cria os atalhos play.cmd / uninstall.cmd.
#
#   install.cmd      instala (Alpine + SDL2 + compila o jogo)
#   play.cmd         joga
#   uninstall.cmd   remove o jogo, a distribuição ou o WSL inteiro
#
# O motivo é o Smart App Control do Windows 11, que bloqueia executáveis
# compilados localmente por não terem assinatura digital reconhecida.
#
# Ainda assim, este Makefile também compila um .exe nativo se você tiver
# MSYS2 ou Git Bash com um MinGW-w64. Nesse caso o SDL2 é procurado em:
#   1. $(SDL2_DIR)                          - variável de ambiente/make
#   2. third_party/SDL2/<arch>-w64-mingw32
#   3. $(MINGW_HOME)                        - SDL instalado dentro do MinGW
#   4. resources/SDL/<arch>-w64-mingw32     - cópia versionada no repositório
#   5. pkg-config                           - MSYS2 (pacman)
#
# Variáveis úteis nesse modo:
#   ARCH=i686           gera executável de 32 bits (padrão: x86_64)
#   WIN_CONSOLE=1       mantém a janela de console aberta (para depurar)
#   SDL2_DIR=<caminho>  aponta manualmente para o SDL2 MinGW dev
#
# ============================================================================

# Nome do projeto
PROJECT_NAME = Tanks

# Diretórios de build e binários
BUILD = build
BIN   = $(BUILD)/bin

# pasta-árvore completa
RESOURCES_DIR = resources

# ---------- Configuração plataforma-específica ----------
ifeq ($(OS),Windows_NT)
    # ----------------------------- WINDOWS -----------------------------
    EXE_EXT = .exe

    # Arquitetura alvo: x86_64 (64 bits, padrão) ou i686 (32 bits)
    ARCH ?= x86_64
    MINGW_TRIPLET = $(ARCH)-w64-mingw32

    # O make define CC=cc por padrao; so sobrescrevemos se o usuario nao definiu
    ifeq ($(origin CC),default)
        CC = g++
    endif

    # Procura uma instalação do SDL2 para MinGW nos locais conhecidos
    SDL_SEARCH_DIRS = $(SDL2_DIR) \
                      third_party/SDL2/$(MINGW_TRIPLET) \
                      $(MINGW_HOME) \
                      $(MINGW_HOME)/$(MINGW_TRIPLET) \
                      $(RESOURCES_DIR)/SDL/$(MINGW_TRIPLET)
    SDL_ROOT := $(patsubst %/include/SDL2,%,\
                  $(firstword $(foreach d,$(SDL_SEARCH_DIRS),$(wildcard $(d)/include/SDL2))))

    ifneq ($(SDL_ROOT),)
        SDL_BIN     = $(SDL_ROOT)/bin
        INCLUDEPATH = -I$(SDL_ROOT)/include -I$(SDL_ROOT)/include/SDL2
        LIBSPATH    = -L$(SDL_ROOT)/lib
        SDL_FOUND   = yes
    else
        # Último recurso: SDL2 instalado via pacman no MSYS2
        SDL_PKG_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_image SDL2_mixer SDL2_ttf 2>/dev/null)
        SDL_PKG_LIBS   := $(shell pkg-config --libs sdl2 SDL2_image SDL2_mixer SDL2_ttf 2>/dev/null)
        INCLUDEPATH = $(SDL_PKG_CFLAGS)
        LIBSPATH    =
        ifneq ($(SDL_PKG_LIBS),)
            SDL_FOUND = yes
        else
            SDL_FOUND = no
        endif
    endif

    # -mwindows esconde o console; WIN_CONSOLE=1 mantém a janela aberta.
    # libgcc/libstdc++ são linkadas estaticamente para que o .exe rode em
    # qualquer Windows, sem depender das DLLs do MinGW.
    WIN_CONSOLE ?= 0
    ifeq ($(WIN_CONSOLE),1)
        WIN_SUBSYSTEM =
    else
        WIN_SUBSYSTEM = -mwindows
    endif

    LFLAGS = -O2 $(WIN_SUBSYSTEM) -static-libgcc -static-libstdc++
    CFLAGS = -c -Wall -std=c++17
    LIBS   = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf

    # Recursos individuais copiados para o lado do executável
    APP_RESOURCES = font/prstartk.ttf png/texture.png levels duel_levels
    RESOURCES     = $(APP_RESOURCES) copy_dlls
else
    # -------------------------- LINUX / MACOS --------------------------
    EXE_EXT =

    UNAME_S := $(shell uname -s)
    UNAME_M := $(shell uname -m)

    ifeq ($(UNAME_S),Darwin)
        CC = g++
        ifeq ($(UNAME_M),arm64)
            INCLUDEPATH = -I/opt/homebrew/include
            LIBSPATH    = -L/opt/homebrew/lib
        else
            INCLUDEPATH = -I/usr/local/include
            LIBSPATH    = -L/usr/local/lib
        endif
    else
        CC = g++
        INCLUDEPATH =
        LIBSPATH =
    endif

    LFLAGS = -O
    CFLAGS = -c -Wall -std=c++17
    LIBS   = -lSDL2main -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf
    APP_RESOURCES = font/prstartk.ttf png/texture.png levels duel_levels
    RESOURCES     = $(APP_RESOURCES)
    SDL_FOUND     = yes
endif
# --------------------------------------------------------

# Caminho final do executável (recebe .exe no Windows)
EXE = $(BIN)/$(PROJECT_NAME)$(EXE_EXT)

# Módulos do projeto
MODULES    = engine app_state objects
SRC_DIRS   = src $(addprefix src/,$(MODULES))
BUILD_DIRS = $(BIN) $(addprefix $(BUILD)/,$(MODULES))

SOURCES = $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.cpp))
OBJS    = $(patsubst src/%.cpp,$(BUILD)/%.o,$(SOURCES))

vpath %.cpp $(SRC_DIRS)

# ============================================================================
# ALVOS PRINCIPAIS
# ============================================================================

# Alvo padrão - compila o projeto
all: build

# Compila o projeto completo
build: print check-sdl $(BUILD_DIRS) copy_resources $(RESOURCES) compile
	@echo ""
	@echo "✅ Compilação concluída com sucesso!"
	@echo "📁 Executável criado em: $(EXE)"
	@echo ""
	@echo "Para executar o jogo, use:"
	@echo "  make run"
	@echo "  ou"
	@echo "  cd $(BIN) && ./$(PROJECT_NAME)$(EXE_EXT)"
	@echo ""

# Executa o jogo (compila se necessário)
run: build
	@echo "🎮 Iniciando Tank-1990..."
	@cd $(BIN) && ./$(PROJECT_NAME)$(EXE_EXT)

# Verifica se o SDL2 foi localizado
check-sdl:
ifeq ($(SDL_FOUND),no)
	@echo ""
	@echo "❌ SDL2 não encontrado para MinGW ($(MINGW_TRIPLET))."
	@echo ""
	@echo "Escolha uma das opções:"
	@echo "  1) Use o WSL:      rode o install.cmd na raiz do projeto (recomendado)"
	@echo "  2) No MSYS2:       pacman -S mingw-w64-$(ARCH)-SDL2 mingw-w64-$(ARCH)-SDL2_image mingw-w64-$(ARCH)-SDL2_mixer mingw-w64-$(ARCH)-SDL2_ttf"
	@echo "  3) Manualmente:    make build SDL2_DIR=C:/caminho/para/SDL2/$(MINGW_TRIPLET)"
	@echo ""
	@exit 1
else
	@true
endif

# Mostra informações do sistema e configuração
info:
	@echo ""
	@echo "📋 INFORMAÇÕES DO SISTEMA:"
	@echo "=========================="
ifeq ($(OS),Windows_NT)
	@echo "OS: Windows_NT"
	@echo "Arquitetura alvo: $(ARCH) ($(MINGW_TRIPLET))"
	@echo "SDL2: $(if $(SDL_ROOT),$(SDL_ROOT),pkg-config)"
else
	@echo "OS: $(shell uname -s) $(shell uname -r)"
	@echo "Arquitetura: $(shell uname -m)"
endif
	@echo "Compilador: $(CC)"
	@echo "Flags de compilação: $(CFLAGS) $(INCLUDEPATH)"
	@echo "Flags de linkagem: $(LFLAGS)"
	@echo "Bibliotecas: $(LIBSPATH) $(LIBS)"
	@echo "Diretório de build: $(BUILD)"
	@echo "Executável: $(EXE)"
	@echo ""

# Instala dependências
install-deps:
ifeq ($(OS),Windows_NT)
	@echo "📦 Instalando dependências SDL2 no Windows..."
	@if command -v pacman >/dev/null 2>&1; then \
		pacman -S --needed --noconfirm mingw-w64-$(ARCH)-gcc mingw-w64-$(ARCH)-SDL2 \
			mingw-w64-$(ARCH)-SDL2_image mingw-w64-$(ARCH)-SDL2_mixer mingw-w64-$(ARCH)-SDL2_ttf ; \
		echo "✅ Dependências instaladas!" ; \
	else \
		echo "" ; \
		echo "Este shell não é o MSYS2 (pacman não encontrado)." ; \
		echo "No Windows, use o WSL: rode o install.cmd na raiz do projeto." ; \
		echo "" ; \
		exit 1 ; \
	fi
else
	@echo "📦 Instalando dependências SDL2..."
	@if command -v apk >/dev/null 2>&1; then \
		apk add --no-cache g++ make sdl2-dev sdl2_image-dev sdl2_mixer-dev sdl2_ttf-dev mesa-dri-gallium ; \
	elif command -v apt >/dev/null 2>&1; then \
		sudo apt update && \
		sudo apt install -y build-essential libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev ; \
	elif command -v dnf >/dev/null 2>&1; then \
		sudo dnf install -y gcc-c++ make SDL2-devel SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel ; \
	elif command -v brew >/dev/null 2>&1; then \
		brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf ; \
	else \
		echo "Gerenciador de pacotes não reconhecido." ; \
		echo "Instale manualmente: SDL2, SDL2_image, SDL2_mixer, SDL2_ttf e g++." ; \
		exit 1 ; \
	fi
	@echo "✅ Dependências instaladas!"
	@echo ""
endif

print:
	@echo ""
	@echo "🔨 Compilando Tank-1990..."
	@echo "========================="
	@echo "Arquivos fonte encontrados: $(words $(SOURCES))"
	@echo "Objetos a compilar: $(words $(OBJS))"
	@echo ""

# Cria diretórios de build
$(BUILD_DIRS):
	mkdir -p $@

# Copia árvore completa de recursos => build/bin/resources/
copy_resources: | $(BIN)
	cp -r $(RESOURCES_DIR) $(BIN)/

# Compila e linka
compile: $(OBJS)
	$(CC) $(OBJS) $(INCLUDEPATH) $(LIBSPATH) $(LIBS) $(LFLAGS) -o $(EXE)

# Compila cada .cpp
build/%.o: src/%.cpp
	$(CC) $(CFLAGS) $(INCLUDEPATH) $< -o $@

# Copia arquivos/diretórios específicos listados em APP_RESOURCES
$(APP_RESOURCES): | $(BIN)
	@if [ -d "$(RESOURCES_DIR)/$@" ]; then \
		cp -r "$(RESOURCES_DIR)/$@" "$(BIN)/" ; \
	elif [ -f "$(RESOURCES_DIR)/$@" ]; then \
		cp    "$(RESOURCES_DIR)/$@" "$(BIN)/" ; \
	else \
		echo "Recurso não encontrado: $(RESOURCES_DIR)/$@" ; \
		exit 1 ; \
	fi

# DLLs do SDL2 ao lado do .exe (Windows)
ifeq ($(OS),Windows_NT)
copy_dlls: | $(BIN)
	@if [ -n "$(SDL_ROOT)" ] && [ -d "$(SDL_BIN)" ]; then \
		cp -f $(SDL_BIN)/*.dll "$(BIN)/" ; \
		echo "📦 DLLs do SDL2 copiadas de $(SDL_BIN)" ; \
	else \
		echo "⚠️  DLLs do SDL2 não localizadas automaticamente." ; \
		echo "   O jogo só rodará se as DLLs estiverem no PATH ou em $(BIN)/" ; \
	fi
endif

# ============================================================================
# AJUDA
# ============================================================================

# Mostra ajuda com todos os comandos disponíveis
help:
	@echo ""
	@echo "🎮 Tank-1990 - Sistema de Build"
	@echo "==============================="
	@echo ""
	@echo "COMANDOS PRINCIPAIS:"
	@echo "  make build       - Compila o projeto completo"
	@echo "  make run         - Compila e executa o jogo"
	@echo "  make clean       - Remove arquivos de build"
	@echo ""
	@echo "COMANDOS AUXILIARES:"
	@echo "  make info        - Mostra informações do sistema"
	@echo "  make doc         - Gera documentação (Doxygen)"
	@echo "  make install-deps - Instala dependências"
	@echo "  make help        - Mostra esta ajuda"
	@echo ""
	@echo "INÍCIO RÁPIDO:"
	@echo "  1. make install-deps  # (primeira vez)"
	@echo "  2. make run           # Compila e executa o jogo"
	@echo ""
	@echo "WINDOWS:"
	@echo "  Use o WSL - rode o install.cmd na raiz do projeto."
	@echo "  Depois: play.cmd para jogar, uninstall.cmd para remover."
	@echo ""
	@echo "  Para compilar um .exe nativo (MSYS2 / Git Bash + MinGW-w64),"
	@echo "  este Makefile funciona normalmente. Opções:"
	@echo "    ARCH=i686 (32 bits), WIN_CONSOLE=1 (mostra o console)"
	@echo ""
	@echo "ESTRUTURA DO PROJETO:"
	@echo "  src/              - Código fonte C++"
	@echo "  resources/        - Recursos (imagens, sons, fontes)"
	@echo "  build/            - Arquivos de build (gerado)"
	@echo "  build/bin/        - Executável final"
	@echo ""

# Declara alvos que não são arquivos
.PHONY: all build run clean doc info install-deps help print copy_resources compile copy_dlls check-sdl

# ============================================================================
# ALVOS DE LIMPEZA E DOCUMENTAÇÃO
# ============================================================================

# Remove arquivos de build
clean:
	@echo "🧹 Limpando arquivos de build..."
	rm -rf $(BUILD) doc
	@echo "✅ Limpeza concluída!"

# Gera documentação com Doxygen
doc:
	@echo "📚 Gerando documentação..."
	doxygen Doxyfile
	@echo "✅ Documentação gerada em doc/"

# ============================================================================
# ALVOS INTERNOS (não usar diretamente)
# ============================================================================
