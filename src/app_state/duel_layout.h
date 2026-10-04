#ifndef DUEL_LAYOUT_H
#define DUEL_LAYOUT_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>

/**
 * @brief Geometria fixa de todo mapa do modo duelo, num lugar só.
 *
 * O jogo (Duel), o validador de mapas (validate, tools/check_duel_maps.cpp) e os testes
 * usam estas mesmas funções. Um mapa novo só precisa respeitar estas posições; nenhuma
 * regra do duelo depende do desenho de um mapa específico.
 *
 * Mapa de 26x26 tiles (a área de jogo). Equipe A (0) embaixo, B (1) em cima:
 * @li águias nas colunas 12-13, nas duas últimas linhas (A) e nas duas primeiras (B);
 * @li a base é montada pelo jogo (não precisa estar no arquivo): a frente da águia é de
 *     pedra (blindada: ninguém destrói a base atirando de frente, nem de longe pelo meio do
 *     mapa) e as laterais e os cantos são de tijolo, então a base é atacada pelos flancos;
 * @li à frente da base fica o pátio, uma passagem livre por onde o defensor contorna a águia
 *     e chega ao flanco de onde o inimigo vier, de um lado ou do outro;
 * @li zona da base (colunas 9-16, 7 linhas do lado de cada águia): ali o canhão não vale;
 * @li pontos de nascimento e de bônus fixos (ver spawnOrder, midBonusSpots, halfBonusSpots).
 */
namespace DuelLayout
{
    /** Tiles de cada lado de um mapa do duelo (a área de jogo tem 26x26 tiles). */
    const int TILES = 26;

    /** Coluna esquerda das águias (ocupam esta coluna e a seguinte). */
    const int BASE_COLUMN = 12;

    /** Zona da base: colunas BASE_COLUMN - 3 .. BASE_COLUMN + 4 (9 a 16)... */
    const int ZONE_SIDE = 3;

    /** ...e as 7 linhas do lado de cada águia. */
    const int ZONE_DEPTH = 7;

    /** Bloco do mapa (linha, coluna). */
    struct Tile
    {
        int row;
        int column;
    };

    /** Primeira linha da águia da equipe (A: TILES - 2, B: 0). */
    int baseRow(int team);

    /** Blocos da muralha em volta da águia da equipe (montados pelo jogo). */
    std::vector<Tile> baseWallTiles(int team);

    /**
     * Blocos da muralha bem na frente da águia (colunas 12-13): sempre de pedra. O resto da
     * muralha (laterais e cantos) é de tijolo: a base é atacada pelos flancos.
     */
    std::vector<Tile> baseFrontTiles(int team);

    /**
     * Centro do pátio (canto superior esquerdo de um tanque 2x2, em tiles): as duas linhas
     * livres logo à frente da muralha, por onde o defensor passa de um flanco para o outro.
     */
    Tile yardCenter(int team);

    /**
     * Um tanque na posição (canto superior esquerdo, em tiles) acerta a águia da equipe pelo
     * flanco: está nas duas linhas da águia, à esquerda (@a side = -1) ou à direita (+1) da
     * muralha, e entre ele e a muralha não há pedra (o resto o tiro atravessa ou derruba).
     * @param grid - mapa como o jogo o monta
     * @return -1 esquerda, +1 direita, 0 se não acerta
     */
    int flankSide(const std::vector<std::string>& grid, int team, int row, int column);

    /**
     * Distância máxima (em passos de 1 tile) de cada nascimento até os DOIS flancos da própria
     * base: o defensor contorna a águia pelo pátio e chega rápido ao lado atacado.
     */
    const int DEFENDER_REACH = 20;

    /** O bloco faz parte da muralha da base da equipe (inclui a frente). */
    bool isBaseWall(int team, int row, int column);

    /** O bloco é um dos da frente da águia da equipe (pedra fixa). */
    bool isBaseFront(int team, int row, int column);

    /** O bloco faz parte do espaço da águia (2x2) de alguma equipe. */
    bool isBaseTile(int row, int column);

    /** O bloco fica na zona de uma das bases (onde o canhão não vale). */
    bool inBaseZone(int row, int column);

    /** Equipe dona da zona de base onde fica o bloco, ou -1 fora das zonas. */
    int zoneTeam(int row, int column);

    /**
     * Pontos de nascimento da equipe (em pixels), em ordem de preferência, alternando os
     * lados da base. A equipe B é o espelho em ponto da A (AppConfig::duel_spawn_columns).
     */
    std::vector<SDL_Point> spawnOrder(int team);

    /** Pontos de bônus no meio do mapa, à mesma distância das duas bases (pixels). */
    std::vector<SDL_Point> midBonusSpots();

    /** Pontos de bônus dentro da metade do mapa da equipe (pixels); o último é o central. */
    std::vector<SDL_Point> halfBonusSpots(int team);

    /** Lê um mapa (uma linha de texto por linha do mapa; ignora '\r' e linhas vazias). */
    std::vector<std::string> readMap(const std::string& path);

    /**
     * Verifica se o mapa respeita a geometria do duelo: 26x26, só símbolos conhecidos,
     * espelhado na horizontal e na vertical (as duas equipes com o mesmo terreno), pontos de
     * nascimento e de bônus livres, e caminho com a largura de um tanque (2 tiles) de cada
     * nascimento até um ponto de tiro num flanco da base inimiga (flankSide), até os dois
     * flancos da própria base em no máximo DEFENDER_REACH passos e até cada ponto de bônus.
     * @return lista de problemas (vazia se o mapa é válido)
     */
    std::vector<std::string> validate(const std::vector<std::string>& grid);

    /** Quantos blocos de pedra do arquivo do mapa ficam na zona das bases, coloridos (informativo). */
    int protectedStone(const std::vector<std::string>& grid);

    /**
     * Carrega a lista de mapas de duel_levels/maps.txt ("arquivo;Nome" por linha, '#' para
     * comentário) em AppConfig::duel_maps, deixando de fora, com aviso no terminal, os mapas
     * que não passam em validate. Sem o arquivo, mantém a lista padrão de AppConfig.
     * @return quantidade de mapas recusados
     */
    int loadMapList();
}

#endif // DUEL_LAYOUT_H
