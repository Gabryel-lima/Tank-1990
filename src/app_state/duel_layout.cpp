#include "duel_layout.h"
#include "../appconfig.h"

#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <set>

namespace DuelLayout
{

namespace
{
    int tile() { return AppConfig::tile_rect.w; }

    // Símbolos dos mapas: os mesmos de Game::loadLevel
    bool known(char c) { return c == '.' || c == '#' || c == '@' || c == '%' || c == '~' || c == '-'; }

    // Tanque passa por vazio, arbusto e gelo; tijolo, pedra e água bloqueiam
    bool passable(char c) { return c == '.' || c == '%' || c == '-'; }

    std::string where(int row, int column)
    {
        return "linha " + std::to_string(row) + ", coluna " + std::to_string(column);
    }
}

int baseRow(int team)
{
    return team == 0 ? TILES - 2 : 0;
}

std::vector<Tile> baseWallTiles(int team)
{
    // Colunas ao lado da águia nas suas duas linhas e na da frente, mais a linha da frente
    int row = baseRow(team);
    int front = (team == 0) ? row - 1 : row + 2;
    std::vector<Tile> tiles;
    for(int r : {row, row + 1, front})
    {
        tiles.push_back({r, BASE_COLUMN - 1});
        tiles.push_back({r, BASE_COLUMN + 2});
    }
    tiles.push_back({front, BASE_COLUMN});
    tiles.push_back({front, BASE_COLUMN + 1});
    return tiles;
}

bool isBaseWall(int team, int row, int column)
{
    for(const Tile& t : baseWallTiles(team))
        if(t.row == row && t.column == column) return true;
    return false;
}

bool isBaseTile(int row, int column)
{
    if(column != BASE_COLUMN && column != BASE_COLUMN + 1) return false;
    for(int team = 0; team < 2; team++)
        if(row == baseRow(team) || row == baseRow(team) + 1) return true;
    return false;
}

bool inBaseZone(int row, int column)
{
    if(column < BASE_COLUMN - ZONE_SIDE || column > BASE_COLUMN + 1 + ZONE_SIDE) return false;
    return row < ZONE_DEPTH || row >= TILES - ZONE_DEPTH;
}

std::vector<SDL_Point> spawnOrder(int team)
{
    // Equipe A: colunas na ordem de AppConfig; equipe B: espelho em ponto (troca os lados)
    std::vector<SDL_Point> order;
    int width = TILES * tile();
    for(int x : AppConfig::duel_spawn_columns)
    {
        int column = (team == 0) ? x : width - 2 * tile() - x;
        order.push_back({column, AppConfig::duel_spawn_rows.at(team)});
    }
    return order;
}

std::vector<SDL_Point> midBonusSpots()
{
    int t = tile(), mid = TILES / 2 - 1;
    return {{BASE_COLUMN * t, mid * t}, {4 * t, mid * t}, {(TILES - 6) * t, mid * t}};
}

std::vector<SDL_Point> halfBonusSpots(int team)
{
    int t = tile();
    if(team == 0) return {{4 * t, 16 * t}, {(TILES - 6) * t, 16 * t}, {BASE_COLUMN * t, 18 * t}};
    return {{4 * t, 8 * t}, {(TILES - 6) * t, 8 * t}, {BASE_COLUMN * t, 6 * t}};
}

std::vector<std::string> readMap(const std::string& path)
{
    std::vector<std::string> grid;
    std::ifstream file(path);
    std::string line;
    while(std::getline(file, line))
    {
        while(!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if(!line.empty()) grid.push_back(line);
    }
    return grid;
}

std::vector<std::string> validate(const std::vector<std::string>& original)
{
    std::vector<std::string> problems;

    // Tamanho e símbolos
    if(original.size() != static_cast<size_t>(TILES))
        problems.push_back("tem " + std::to_string(original.size()) + " linhas (precisa de " + std::to_string(TILES) + ")");
    for(size_t r = 0; r < original.size(); r++)
    {
        if(original[r].size() != static_cast<size_t>(TILES))
            problems.push_back("linha " + std::to_string(r) + " tem " + std::to_string(original[r].size()) +
                               " colunas (precisa de " + std::to_string(TILES) + ")");
        for(size_t c = 0; c < original[r].size(); c++)
            if(!known(original[r][c]))
                problems.push_back(std::string("símbolo desconhecido '") + original[r][c] + "' na " + where(r, c));
    }
    if(!problems.empty()) return problems;

    // O mapa como o jogo o monta: espaço das águias vazio e muralhas de tijolo
    std::vector<std::string> grid = original;
    for(int team = 0; team < 2; team++)
    {
        for(const Tile& t : baseWallTiles(team)) grid[t.row][t.column] = '#';
        for(int r = baseRow(team); r < baseRow(team) + 2; r++)
            for(int c = BASE_COLUMN; c < BASE_COLUMN + 2; c++) grid[r][c] = 'E';
    }

    // Simetria: as duas equipes precisam ter o mesmo terreno
    int asymmetric = 0;
    for(int r = 0; r < TILES; r++)
        for(int c = 0; c < TILES; c++)
            if(original[r][c] != original[r][TILES - 1 - c] || original[r][c] != original[TILES - 1 - r][c])
            {
                if(asymmetric++ == 0)
                    problems.push_back("não é espelhado na horizontal e na vertical (primeira diferença na " + where(r, c) + ")");
            }

    // Posição (canto superior esquerdo, em tiles) de um tanque 2x2 que cabe ali
    auto fits = [&](int r, int c) {
        if(r < 0 || c < 0 || r + 1 >= TILES || c + 1 >= TILES) return false;
        for(int i = 0; i < 2; i++)
            for(int j = 0; j < 2; j++)
                if(!passable(grid[r + i][c + j])) return false;
        return true;
    };
    auto reach = [&](int r0, int c0) {
        std::set<std::pair<int, int>> seen = {{r0, c0}};
        std::deque<std::pair<int, int>> queue = {{r0, c0}};
        while(!queue.empty())
        {
            auto [r, c] = queue.front();
            queue.pop_front();
            const int dr[] = {1, -1, 0, 0}, dc[] = {0, 0, 1, -1};
            for(int k = 0; k < 4; k++)
            {
                std::pair<int, int> next = {r + dr[k], c + dc[k]};
                if(!seen.count(next) && fits(next.first, next.second))
                {
                    seen.insert(next);
                    queue.push_back(next);
                }
            }
        }
        return seen;
    };
    // Um tanque na posição encosta na muralha da equipe (bloco vizinho ao seu 2x2)
    auto touchesWall = [&](int team, int r, int c) {
        for(const Tile& t : baseWallTiles(team))
            for(int i = 0; i < 2; i++)
                for(int j = 0; j < 2; j++)
                    if(std::abs(r + i - t.row) + std::abs(c + j - t.column) == 1) return true;
        return false;
    };

    std::vector<std::pair<int, int>> bonus;
    for(SDL_Point p : midBonusSpots()) bonus.push_back({p.y / tile(), p.x / tile()});
    for(int team = 0; team < 2; team++)
        for(SDL_Point p : halfBonusSpots(team)) bonus.push_back({p.y / tile(), p.x / tile()});
    for(auto& b : bonus)
        if(!fits(b.first, b.second))
            problems.push_back("ponto de bônus bloqueado na " + where(b.first, b.second));

    for(int team = 0; team < 2; team++)
        for(SDL_Point s : spawnOrder(team))
        {
            int r = s.y / tile(), c = s.x / tile();
            std::string name = std::string("nascimento da equipe ") + (team == 0 ? "A" : "B") + " na coluna " + std::to_string(c);
            if(!fits(r, c))
            {
                problems.push_back(name + ": bloqueado");
                continue;
            }
            auto seen = reach(r, c);
            bool reaches_enemy = false;
            for(auto& p : seen)
                if(touchesWall(1 - team, p.first, p.second)) reaches_enemy = true;
            if(!reaches_enemy)
                problems.push_back(name + ": não há caminho da largura de um tanque até a base inimiga");
            for(auto& b : bonus)
                if(fits(b.first, b.second) && !seen.count(b))
                    problems.push_back(name + ": não alcança o ponto de bônus da " + where(b.first, b.second));
        }
    return problems;
}

int protectedStone(const std::vector<std::string>& grid)
{
    int count = 0;
    for(size_t r = 0; r < grid.size(); r++)
        for(size_t c = 0; c < grid[r].size(); c++)
            if(grid[r][c] == '@' && inBaseZone(r, c)) count++;
    return count;
}

int loadMapList()
{
    std::ifstream list(AppConfig::duel_levels_path + "maps.txt");
    if(!list.is_open()) return 0;

    std::vector<std::pair<std::string, std::string>> maps;
    int rejected = 0;
    std::string line;
    while(std::getline(list, line))
    {
        while(!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if(line.empty() || line[0] == '#') continue;
        size_t sep = line.find(';');
        std::string file = line.substr(0, sep);
        std::string name = (sep == std::string::npos) ? file : line.substr(sep + 1);

        std::vector<std::string> problems = validate(readMap(AppConfig::duel_levels_path + file));
        if(!problems.empty())
        {
            std::cerr << "Mapa de duelo \"" << name << "\" (" << file << ") recusado:\n";
            for(const std::string& p : problems) std::cerr << "  - " << p << "\n";
            rejected++;
            continue;
        }
        maps.push_back({file, name});
    }
    if(!maps.empty()) AppConfig::duel_maps = maps;
    return rejected;
}

} // namespace DuelLayout
