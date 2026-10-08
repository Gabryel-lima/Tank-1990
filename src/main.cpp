/**
 * \mainpage
 * \par Tanks
 * Jogo de tanques inspirado em Battle City / Tank 1990, para 1 a 4 jogadores.
 *
 * \par Projeto original
 * Escrito por Krystian Kałużny (2015) em C++11 + SDL2, sob licença MIT.
 * Repositório: https://github.com/krystiankaluzny/Tanks
 * Contato: \a k.kaluzny141@gmail.com
 *
 * \par Este fork
 * Mantido por Gabryel Lima: correção de bugs, suporte a 3 e 4 jogadores,
 * cores por jogador, suporte a controles e build para Windows/Linux/macOS.
 * Repositório: https://github.com/Gabryel-lima/Tank-1990
 *
 * \author Krystian Kałużny (original), Gabryel Lima (fork)
 * \version 1.2.1
 */

// SDL_main.h (via SDL.h) renomeia main() para SDL_main() no Windows.
// Sem este include a linkagem com -lSDL2main/-mwindows falha no MinGW.
#include <SDL2/SDL.h>

#include <cstdlib>

#include "app.h"

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

// Função principal do programa.
// Inicializa a aplicação e executa o loop principal do jogo.
int main(int argc, char* args[])
{
    // Os arquivos do jogo (texture.png, fonte, levels/...) são lidos por caminho relativo:
    // entra na pasta deles para funcionar rodado de qualquer pasta (ex.: ./build/bin/Tanks a
    // partir da raiz do projeto). Por padrão é a pasta do executável; TANK_DATA_DIR escolhe
    // outra, para o executável poder morar longe dos dados (no console de TV, o lançador
    // roda uma cópia do binário numa pasta que executa, e os dados ficam no cartão)
    const char* data_dir = std::getenv("TANK_DATA_DIR");
    if(data_dir != nullptr && data_dir[0] != '\0')
    {
        if(chdir(data_dir) != 0) SDL_Log("Nao consegui entrar em %s", data_dir);
    }
    else if(char* base = SDL_GetBasePath())
    {
        if(chdir(base) != 0) SDL_Log("Nao consegui entrar em %s", base);
        SDL_free(base);
    }

    // Cria a instância principal da aplicação
    App app;
    // Inicia o loop principal do jogo
    app.run();

    // Retorna 0 indicando execução bem-sucedida
    return 0;
}
