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

# ------ Alvo principal ------
all: print $(BUILD_DIRS) copy_resources $(RESOURCES) compile
# ----------------------------

print:
	@echo "\nOS: $(OS)"
	@echo "SOURCES: $(SOURCES)"
	@echo "OBJS: $(OBJS)"
	@echo "LIBS: $(LIBS)\n"

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

clean:
	rm -rf $(BUILD) doc

doc:
	doxywizard Doxyfile && doxygen
