#include "navgrid.h"

#include <functional>
#include <queue>
#include <utility>

namespace
{
    // Custo de cada célula: 1 para andar; um bloco a derrubar no caminho custa como
    // andar várias células (o tanque para, atira e espera o tiro abrir a passagem)
    const int MOVE_COST = 1;
    const int BREAK_COST = 4;
}

void NavGrid::resize(int rows, int columns)
{
    m_rows = rows;
    m_columns = columns;
    m_tiles.assign(rows * columns, TILE_FREE);
}

void NavGrid::setTile(int row, int column, Tile tile)
{
    if(row < 0 || column < 0 || row >= m_rows || column >= m_columns) return;
    m_tiles[row * m_columns + column] = tile;
}

NavGrid::Tile NavGrid::tile(int row, int column) const
{
    if(row < 0 || column < 0 || row >= m_rows || column >= m_columns) return TILE_BLOCKED;
    return m_tiles[row * m_columns + column];
}

bool NavGrid::validCell(int row, int column) const
{
    return row >= 0 && column >= 0 && row < cellRows() && column < cellColumns();
}

int NavGrid::cellCost(int row, int column, const Abilities& abilities) const
{
    if(!validCell(row, column)) return UNREACHABLE;

    bool must_break = false;
    for(int r = row; r < row + 2; r++)
        for(int c = column; c < column + 2; c++)
            switch(tile(r, c))
            {
            case TILE_FREE:
                break;
            case TILE_BRICK:
                must_break = true;
                break;
            case TILE_STONE:
                if(!abilities.break_stone) return UNREACHABLE;
                must_break = true;
                break;
            case TILE_WATER:
                if(!abilities.boat) return UNREACHABLE;
                break;
            case TILE_BLOCKED:
            case TILE_ZONE_STONE:
                return UNREACHABLE;
            }
    return must_break ? MOVE_COST + BREAK_COST : MOVE_COST;
}

std::vector<int> NavGrid::distanceField(const std::vector<int>& goals, const Abilities& abilities) const
{
    int count = cellRows() * cellColumns();
    std::vector<int> dist(count > 0 ? count : 0, UNREACHABLE);
    if(count <= 0) return dist;

    // Custo de cada célula calculado uma vez
    std::vector<int> cost(count);
    for(int r = 0; r < cellRows(); r++)
        for(int c = 0; c < cellColumns(); c++)
            cost[cellIndex(r, c)] = cellCost(r, c, abilities);

    // Dijkstra a partir dos destinos: o custo de um passo é o da célula de onde se sai
    // em direção ao destino (ou seja, o custo de entrar nela vindo do tanque)
    typedef std::pair<int, int> Entry; // (distância, célula)
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    for(int goal : goals)
    {
        if(goal < 0 || goal >= count || cost[goal] >= UNREACHABLE) continue;
        dist[goal] = 0;
        open.push({0, goal});
    }

    const int dr[4] = {-1, 0, 1, 0};
    const int dc[4] = {0, 1, 0, -1};
    while(!open.empty())
    {
        Entry top = open.top();
        open.pop();
        if(top.first > dist[top.second]) continue;

        int r = top.second / cellColumns(), c = top.second % cellColumns();
        for(int k = 0; k < 4; k++)
        {
            int nr = r + dr[k], nc = c + dc[k];
            if(!validCell(nr, nc)) continue;
            int next = cellIndex(nr, nc);
            if(cost[next] >= UNREACHABLE) continue;
            // Quem está em "next" precisa entrar em "top": paga o custo da célula de destino
            int d = top.first + cost[top.second];
            if(d < dist[next])
            {
                dist[next] = d;
                open.push({d, next});
            }
        }
    }
    return dist;
}
