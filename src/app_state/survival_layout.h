#ifndef SURVIVAL_LAYOUT_H
#define SURVIVAL_LAYOUT_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>

/**
 * @brief Geometria fixa dos mapas do modo sobrevivência, num lugar só.
 *
 * O jogo (Survival), o validador (tools/check_duel_maps.cpp) e os testes usam estas
 * mesmas funções. Mapa de 26x26 tiles, com os mesmos símbolos das fases da campanha:
 * @li águia nas colunas 12-13 das duas últimas linhas, com a muralha de tijolos em volta
 *     (colunas 11 e 14 e a linha da frente), montada pelo jogo;
 * @li inimigos surgem no topo, nos pontos de AppConfig::enemy_starting_point;
 * @li jogadores nascem nos pontos de AppConfig::player_starting_point.
 */
namespace SurvivalLayout
{
    /** Bloco do mapa (linha, coluna). */
    struct Tile
    {
        int row;
        int column;
    };

    /** Blocos da muralha em volta da águia (montados pelo jogo a cada onda). */
    std::vector<Tile> baseWallTiles();

    /**
     * Verifica se o mapa pode ser jogado: 26x26, só símbolos conhecidos, pontos de surgimento
     * dos inimigos e de nascimento dos jogadores livres, todos ligados por caminhos da
     * largura de um tanque (2 tiles), e a muralha da águia alcançável por esses caminhos
     * (os inimigos precisam conseguir ameaçar a base).
     * @return lista de problemas (vazia se o mapa é válido)
     */
    std::vector<std::string> validate(const std::vector<std::string>& grid);

    /**
     * Carrega a lista de mapas de survival_levels/maps.txt ("arquivo;Nome" por linha, '#'
     * para comentário) em AppConfig::survival_maps, deixando de fora, com aviso no terminal,
     * os mapas que não passam em validate. Sem o arquivo, mantém a lista padrão de AppConfig.
     * @return quantidade de mapas recusados
     */
    int loadMapList();
}

#endif // SURVIVAL_LAYOUT_H
