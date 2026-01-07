# ============================================================================
# Tank-1990 - Makefile
# ============================================================================
# 
# INSTRUÇÕES DE USO:
# 
# Para compilar o jogo:
#   make build       # Compila o projeto completo
#   make             # Mesmo que 'make build'
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
# Para instalar dependências (Ubuntu/Debian):
#   make install-deps
# 
# ============================================================================

# Nome do projeto
PROJECT_NAME = Tanks

# Diretórios de build e binários
BUILD = build
BIN   = $(BUILD)/bin

# pasta‑árvore completa
RESOURCES_DIR = resources

# ---------- Configuração plataforma‑específica ----------
ifeq ($(OS),Windows_NT)
    CC = $(MINGW_HOME)/bin/mingw32-g++.exe
    INCLUDEPATH = -I$(RESOURCES_DIR)/SDL/i686-w64-mingw32/include
    LFLAGS  = -mwindows -O
    CFLAGS  = -c -Wall
    LIBS    = -L$(RESOURCES_DIR)/SDL/i686-w64-mingw32/lib \
              -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf
    # recursos individuais (DLLs, fontes, texturas, níveis)
    APP_RESOURCES = SDL/i686-w64-mingw32/bin/*.dll dll/*.dll \
                    font/prstartk.ttf png/texture.png levels
    RESOURCES = $(APP_RESOURCES) mingw_resources
else
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
    APP_RESOURCES = font/prstartk.ttf png/texture.png levels
    RESOURCES = $(APP_RESOURCES)
endif
# --------------------------------------------------------

# Módulos do projeto
MODULES    = engine app_state objects
SRC_DIRS   = src $(addprefix src/,$(MODULES))
BUILD_DIRS = $(BUILD) $(BIN) $(addprefix $(BUILD)/,$(MODULES))

SOURCES = $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.cpp))
OBJS    = $(patsubst src/%.cpp,$(BUILD)/%.o,$(SOURCES))

vpath %.cpp $(SRC_DIRS)

# ============================================================================
# ALVOS PRINCIPAIS
# ============================================================================

# Alvo padrão - compila o projeto
all: build

# Compila o projeto completo
build: print $(BUILD_DIRS) copy_resources $(RESOURCES) compile
	@echo ""
	@echo "✅ Compilação concluída com sucesso!"
	@echo "📁 Executável criado em: $(BIN)/$(PROJECT_NAME)"
	@echo ""
	@echo "Para executar o jogo, use:"
	@echo "  make run"
	@echo "  ou"
	@echo "  cd $(BIN) && ./$(PROJECT_NAME)"
	@echo ""

# Executa o jogo (compila se necessário)
run: build
	@echo "🎮 Iniciando Tank-1990..."
	@cd $(BIN) && ./$(PROJECT_NAME)

# Mostra informações do sistema e configuração
info:
	@echo ""
	@echo "📋 INFORMAÇÕES DO SISTEMA:"
	@echo "=========================="
	@echo "OS: $(shell uname -s) $(shell uname -r)"
	@echo "Arquitetura: $(shell uname -m)"
	@echo "Compilador: $(CC)"
	@echo "Flags de compilação: $(CFLAGS)"
	@echo "Flags de linkagem: $(LFLAGS)"
	@echo "Bibliotecas: $(LIBS)"
	@echo "Diretório de build: $(BUILD)"
	@echo "Executável: $(BIN)/$(PROJECT_NAME)"
	@echo ""

# Instala dependências no Ubuntu/Debian
install-deps:
	@echo "📦 Instalando dependências SDL2..."
	sudo apt update
	sudo apt install -y libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
	@echo "✅ Dependências instaladas!"
	@echo ""

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
	$(CC) $(OBJS) $(INCLUDEPATH) $(LIBSPATH) $(LIBS) $(LFLAGS) -o $(BIN)/$(PROJECT_NAME)

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

# DLLs extras no Windows
ifeq ($(OS),Windows_NT)
mingw_resources:
	cp $(MINGW_HOME)/bin/libstdc++-6.dll $(BIN)
	cp $(MINGW_HOME)/bin/libgcc_s_dw2-1.dll $(BIN)
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
	@echo "  make install-deps - Instala dependências (Ubuntu/Debian)"
	@echo "  make help        - Mostra esta ajuda"
	@echo ""
	@echo "INÍCIO RÁPIDO:"
	@echo "  1. make install-deps  # (primeira vez, no Ubuntu/Debian)"
	@echo "  2. make run           # Compila e executa o jogo"
	@echo ""
	@echo "ESTRUTURA DO PROJETO:"
	@echo "  src/              - Código fonte C++"
	@echo "  resources/        - Recursos (imagens, sons, fontes)"
	@echo "  build/            - Arquivos de build (gerado)"
	@echo "  build/bin/        - Executável final"
	@echo ""

# Declara alvos que não são arquivos
.PHONY: all build run clean doc info install-deps help print copy_resources compile mingw_resources

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
	doxywizard Doxyfile && doxygen
	@echo "✅ Documentação gerada em doc/"

# ============================================================================
# ALVOS INTERNOS (não usar diretamente)
# ============================================================================
