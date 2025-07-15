# Nome do projeto
PROJECT_NAME = Tanks

# Diretórios de build e binários
BUILD = build
BIN = $(BUILD)/bin
# Dir de recursos
RESOURCES_DIR = resources

# Configuração específica para Windows
ifeq ($(OS),Windows_NT)
	CC = $(MINGW_HOME)/bin/mingw32-g++.exe  # Compilador MinGW para Windows
	INCLUDEPATH = -I$(RESOURCES_DIR)/SDL/i686-w64-mingw32/include  # Includes do SDL
	LFLAGS = -mwindows -O  # Flags de link (janela Windows, otimização)
	CFLAGS = -c -Wall  # Flags de compilação
	LIBS = -L$(RESOURCES_DIR)/SDL/i686-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf  # Bibliotecas para linkar
	APP_RESOURCES = SDL/i686-w64-mingw32/bin/*.dll dll/*.dll font/prstartk.ttf png/texture.png levels  # Recursos necessários para rodar no Windows
	RESOURCES = $(APP_RESOURCES) mingw_resources  # Inclui recursos e DLLs do MinGW
else
	# Detecta sistema operacional e arquitetura
	UNAME_S := $(shell uname -s)
	UNAME_M := $(shell uname -m)

	ifeq ($(UNAME_S),Darwin)
		ifeq ($(UNAME_M),arm64)
			# Configuração para Mac M1 (Apple Silicon)
			CC = g++
			INCLUDEPATH = -I/opt/homebrew/include
			LIBSPATH = -L/opt/homebrew/lib
		else
			# Configuração para Mac Intel (x86_64)
			CC = g++
			INCLUDEPATH = -I/usr/local/include
			LIBSPATH = -L/usr/local/lib
		endif
		LFLAGS = -O  # Otimização
		CFLAGS = -c -Wall -std=c++11  # Compilação C++11
		LIBS = -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf  # Bibliotecas SDL
		APP_RESOURCES = font/prstartk.ttf png/texture.png levels  # Recursos para Mac
		RESOURCES = $(APP_RESOURCES)
	else
		# Configuração padrão para Linux
		CC = g++
		INCLUDEPATH =
		LFLAGS = -O
		CFLAGS = -c -Wall -std=c++11
		LIBS = -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf
		APP_RESOURCES = font/prstartk.ttf png/texture.png levels
		RESOURCES = $(APP_RESOURCES)
	endif
endif

# Módulos do projeto (subdiretórios)
MODULES = engine app_state objects
SRC_DIRS = src $(addprefix src/,$(MODULES))  # Diretórios de código-fonte
BUILD_DIRS = $(BUILD) $(BIN) $(addprefix $(BUILD)/,$(MODULES))  # Diretórios de build

# Busca todos os arquivos .cpp nos diretórios de código-fonte
SOURCES = $(foreach sdir,$(SRC_DIRS),$(wildcard $(sdir)/*.cpp))
# Gera os nomes dos arquivos objeto correspondentes
OBJS = $(patsubst src/%.cpp,$(BUILD)/%.o,$(SOURCES))

# Permite que o make encontre os .cpp nos diretórios listados
vpath %.cpp $(SRC_DIRS)

# Alvo principal: mostra informações, cria diretórios, copia recursos e compila
all: print $(BUILD_DIRS) $(RESOURCES) compile

# Alvo para imprimir variáveis de ambiente e configuração
print:
	@echo
	@echo OS: $(OS)
	@echo MODULES: $(MODULES)
	@echo SRC_DIRS: $(SRC_DIRS)
	@echo BUILD_DIRS: $(BUILD_DIRS)
	@echo SOURCES: $(SOURCES)
	@echo RESOURCES: $(RESOURCES)
	@echo OBJS: $(OBJS)
	@echo INCLUDEPATH: $(INCLUDEPATH)
	@echo LIBSPATH: $(LIBSPATH)
	@echo LIBS: $(LIBS)
	@echo LFLAGS: $(LFLAGS)
	@echo

# Cria os diretórios de build se não existirem
$(BUILD_DIRS):
	mkdir -p $@

# Compila o projeto final
compile: $(OBJS)
	$(CC) $(OBJS) $(INCLUDEPATH) $(LIBSPATH) $(LIBS) $(LFLAGS) -o $(BIN)/$(PROJECT_NAME)

# Regra para compilar cada arquivo .cpp em .o
build/%.o: src/%.cpp
	$(CC) $(CFLAGS) $(INCLUDEPATH) $< -o $@

# Copia os recursos necessários para o diretório binário
$(APP_RESOURCES): | $(BIN)
	@echo "DEBUG: $(RESOURCES_DIR)/$@"
	@if [ -d "$(RESOURCES_DIR)/$@" ]; then \
		cp -r "$(RESOURCES_DIR)/$@" "$(BIN)/" ; \
	elif [ -f "$(RESOURCES_DIR)/$@" ]; then \
		cp -f "$(RESOURCES_DIR)/$@" "$(BIN)/" ; \
	else \
		echo "Arquivo ou diretório não encontrado: $(RESOURCES_DIR)/$@" ; \
		exit 1 ; \
	fi

# Regras específicas para Windows (copiar DLLs do MinGW)
ifeq ($(OS),Windows_NT)

mingw_resources:
	cp $(MINGW_HOME)/bin/libstdc++-6.dll $(BIN)
	cp $(MINGW_HOME)/bin/libgcc_s_dw2-1.dll $(BIN)

endif

# Limpa arquivos de build e documentação
clean:
	rm -rf $(BUILD)
	rm -rf doc

# Gera documentação com Doxygen
doc:
	doxywizard Doxyfile
	doxygen
