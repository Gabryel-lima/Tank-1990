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
 * Mapa de 26x26 tiles (a área de jogo). Equipe A (0) embaixo, B (1) em cima e, no formato
 * de quatro equipes (cada um por si), C (2) à esquerda e D (3) à direita. Tudo é definido
 * uma vez para a equipe A e as outras são a A girada: B em 180 graus, C em 90 graus no
 * sentido horário e D em 270 (quarterTurns). Com isso, o terreno de uma equipe é o de todas
 * quando o mapa é igual a si mesmo girado (180 graus com duas equipes, 90 com quatro).
 * Descrição da equipe A (as outras giram):
 * @li águias nas colunas 12-13, nas duas últimas linhas (A) e nas duas primeiras (B);
 * @li a base é montada pelo jogo (não precisa estar no arquivo): a frente da águia é de
 *     pedra (blindada: ninguém destrói a base atirando de frente, nem de longe pelo meio do
 *     mapa) e as laterais e os cantos são de tijolo, então a base é atacada pelos flancos;
 * @li à frente da base fica o pátio, uma passagem livre por onde o defensor contorna a águia
 *     e chega ao flanco de onde o inimigo vier, de um lado ou do outro;
 * @li zona da base (colunas 9-16, 7 linhas do lado de cada águia): ali o canhão não vale;
 * @li pontos de nascimento e de bônus fixos (ver spawnOrder, midBonusSpots, halfBonusSpots).
 *
 * As funções que dependem de quantas equipes jogam recebem @a teams (2 ou 4; o padrão é 2):
 * a zona de uma base só existe se a equipe joga.
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

    /** Quantidade de equipes do formato cada um por si (um mapa dele precisa de todas as bases). */
    const int FFA_TEAMS = 4;

    /** Voltas de 90 graus (sentido horário) que levam a equipe A à equipe: A 0, B 2, C 1, D 3. */
    int quarterTurns(int team);

    /** O bloco (1x1) da equipe A vira o bloco correspondente da equipe @a team. */
    Tile placeCell(int team, Tile a_frame);

    /** O canto superior esquerdo de um bloco 2x2 da equipe A vira o da equipe @a team. */
    Tile placeBlock(int team, Tile a_frame);

    /** Inverso de placeCell: o bloco do mapa visto do lado da equipe A. */
    Tile frameCell(int team, Tile map);

    /** Inverso de placeBlock. */
    Tile frameBlock(int team, Tile map);

    /** Canto superior esquerdo da águia da equipe (ocupa 2x2 blocos). */
    Tile baseOrigin(int team);

    /** Blocos da muralha em volta da águia da equipe (montados pelo jogo). */
    std::vector<Tile> baseWallTiles(int team);

    /**
     * Blocos da muralha bem na frente da águia: sempre de pedra. O resto da muralha
     * (laterais e cantos) é de tijolo: a base é atacada pelos flancos.
     */
    std::vector<Tile> baseFrontTiles(int team);

    /**
     * Centro do pátio (canto superior esquerdo de um tanque 2x2, em tiles): as duas linhas
     * livres logo à frente da muralha, por onde o defensor passa de um flanco para o outro.
     */
    Tile yardCenter(int team);

    /**
     * Postos de guarda do defensor, do melhor para o pior: o centro do pátio e dois pontos
     * duas linhas à frente dele, fora do pátio (canto superior esquerdo de um tanque 2x2).
     */
    std::vector<Tile> guardPosts(int team);

    /**
     * Para onde o defensor da equipe olha estando no posto @a post: o outro lado do mapa,
     * na mesma coluna (do ponto de vista da equipe), como um bloco 2x2.
     */
    Tile lookTile(int team, Tile post);

    /**
     * Um tanque 2x2 em (@a row, @a column) fica na saída do ponto de nascimento @a spawn
     * (pixels): as quatro linhas à frente de quem nasce. O defensor não estaciona ali.
     */
    bool inSpawnExit(int team, SDL_Point spawn, int row, int column);

    /**
     * Um tanque na posição (canto superior esquerdo, em tiles) acerta a águia da equipe pelo
     * flanco: está nas duas linhas da águia, à esquerda (@a side = -1) ou à direita (+1) da
     * muralha (do ponto de vista da equipe, olhando para o centro do mapa), e entre ele e a
     * muralha não há pedra (o resto o tiro atravessa ou derruba).
     * @param grid - mapa como o jogo o monta
     * @return -1 esquerda, +1 direita, 0 se não acerta
     */
    int flankSide(const std::vector<std::string>& grid, int team, int row, int column);

    /**
     * Distância sugerida (em passos de 1 tile) de cada nascimento até os DOIS flancos da própria
     * base: o defensor contorna a águia pelo pátio e chega rápido ao lado atacado. Passar
     * disso é aviso (advise), não erro: o mapa funciona, só fica mais difícil de defender.
     */
    const int DEFENDER_REACH = 20;

    /** O bloco faz parte da muralha da base da equipe (inclui a frente). */
    bool isBaseWall(int team, int row, int column);

    /** O bloco é um dos da frente da águia da equipe (pedra fixa). */
    bool isBaseFront(int team, int row, int column);

    /** O bloco faz parte do espaço da águia (2x2) de alguma das @a teams equipes. */
    bool isBaseTile(int row, int column, int teams = 2);

    /** O bloco fica na zona de uma das bases (onde o canhão não vale). */
    bool inBaseZone(int row, int column, int teams = 2);

    /** Equipe dona da zona de base onde fica o bloco, ou -1 fora das zonas. */
    int zoneTeam(int row, int column, int teams = 2);

    /**
     * Pontos de nascimento da equipe (em pixels), em ordem de preferência, alternando os
     * lados da base. As colunas (AppConfig::duel_spawn_columns) são as da equipe A, e as
     * outras equipes são a A girada.
     */
    std::vector<SDL_Point> spawnOrder(int team);

    /**
     * Pontos de bônus no meio do mapa, à mesma distância das bases (pixels). Com duas equipes,
     * três pontos na linha do meio; com quatro, o centro e as quatro diagonais.
     */
    std::vector<SDL_Point> midBonusSpots(int teams = 2);

    /** Pontos de bônus dentro da metade do mapa da equipe (pixels); o último é o central. */
    std::vector<SDL_Point> halfBonusSpots(int team);

    /** Lê um mapa (uma linha de texto por linha do mapa; ignora '\r' e linhas vazias). */
    std::vector<std::string> readMap(const std::string& path);

    /**
     * Erros do mapa: o que impede o duelo de funcionar ou de ser justo. O mapa com erro é
     * recusado. Só entra aqui o indispensável, para limitar o mínimo quem desenha mapas:
     * 26x26 com símbolos conhecidos; simétrico girando 180 graus com duas equipes (as equipes
     * com o mesmo terreno) ou 90 graus com quatro; pontos de nascimento livres; e, de cada
     * nascimento, caminho da largura de um tanque (2 tiles) até um ponto de tiro num flanco
     * da base de cada adversário (flankSide).
     * @param teams - 2 (A embaixo e B em cima) ou FFA_TEAMS (as quatro bases)
     * @return lista de erros (vazia se o mapa é aceito)
     */
    std::vector<std::string> validate(const std::vector<std::string>& grid, int teams = 2);

    /**
     * Avisos de desenho de um mapa aceito (vazio se há erros): ponto de bônus coberto (o jogo
     * o pula), ponto de bônus inalcançável a pé, defensor que não contorna a águia ou passa de
     * DEFENDER_REACH passos até um flanco da própria base. O mapa funciona; vale rever.
     */
    std::vector<std::string> advise(const std::vector<std::string>& grid, int teams = 2);

    /**
     * Dono de cada bloco de pedra do mapa (fora a muralha da águia): a equipe, se a parede
     * (os blocos de pedra ligados lado com lado) fica inteira dentro da zona da base dela;
     * -1 para pedra comum (a parede cruza a borda da zona ou fica fora) e para o que não é
     * pedra. A pedra com dono fica na cor da equipe; o adversário não a derruba.
     */
    std::vector<std::vector<int>> stoneOwners(const std::vector<std::string>& grid, int teams = 2);

    /** Quantos blocos de pedra do mapa têm dono (stoneOwners), fora a muralha (informativo). */
    int protectedStone(const std::vector<std::string>& grid, int teams = 2);

    /**
     * Carrega as listas de mapas ("arquivo;Nome" por linha, '#' para comentário): a de
     * duel_levels/maps.txt (duas equipes) em AppConfig::duel_maps e a de duel_levels/ffa/maps.txt
     * (cada um por si, quatro equipes) em AppConfig::duel_ffa_maps, deixando de fora, com aviso
     * no terminal, os mapas que não passam em validate. Sem o arquivo, mantém a lista padrão.
     * @return quantidade de mapas recusados
     */
    int loadMapList();
}

#endif // DUEL_LAYOUT_H
