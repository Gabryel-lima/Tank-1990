#ifndef NAVGRID_H
#define NAVGRID_H

#include <vector>

/**
 * @brief Grade de navegação dos tanques controlados pelo computador.
 *
 * O mapa tem blocos de 16 px e o tanque ocupa 2x2 blocos; uma "célula" da grade é a
 * posição do tanque alinhada aos blocos, identificada pelo bloco do canto superior
 * esquerdo. A grade calcula, com Dijkstra, a distância de cada célula até um conjunto
 * de células de destino; o tanque anda para a vizinha de menor distância.
 *
 * Tijolo não bloqueia: custa mais caro, porque o tanque precisa atirar para abrir caminho.
 * Pedra só passa quem quebra pedra (3 estrelas); água, só quem tem barco.
 */
class NavGrid
{
public:
    /** Conteúdo de um bloco do mapa, do ponto de vista de quem anda. */
    enum Tile
    {
        TILE_FREE,    ///< vazio, gelo ou arbusto
        TILE_BRICK,   ///< tijolo: atirando, passa
        TILE_STONE,   ///< pedra: só com tiro forte
        TILE_WATER,   ///< água: só com barco
        TILE_BLOCKED  ///< nunca passa (águia, muralha que a equipe não pode derrubar)
    };

    /** O que o tanque consegue atravessar. */
    struct Abilities
    {
        bool boat = false;        ///< atravessa água
        bool break_stone = false; ///< tiro forte: pedra vira obstáculo destrutível
    };

    static constexpr int UNREACHABLE = 1 << 29;

    /** Define o tamanho da grade (em blocos); todos os blocos começam livres. */
    void resize(int rows, int columns);

    void setTile(int row, int column, Tile tile);
    Tile tile(int row, int column) const;

    int rows() const { return m_rows; }
    int columns() const { return m_columns; }

    /** Quantidade de células (posições possíveis do tanque) em cada eixo. */
    int cellRows() const { return m_rows - 1; }
    int cellColumns() const { return m_columns - 1; }
    int cellIndex(int row, int column) const { return row * cellColumns() + column; }
    bool validCell(int row, int column) const;

    /** Custo para o tanque ocupar a célula, ou UNREACHABLE. */
    int cellCost(int row, int column, const Abilities& abilities) const;

    /**
     * Distância de cada célula até o destino mais próximo (UNREACHABLE se não há caminho).
     * @param goals - índices de células (cellIndex) de destino
     */
    std::vector<int> distanceField(const std::vector<int>& goals, const Abilities& abilities) const;

private:
    int m_rows = 0;
    int m_columns = 0;
    std::vector<Tile> m_tiles;
};

#endif // NAVGRID_H
