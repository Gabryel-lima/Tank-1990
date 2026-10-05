#include "survival_layout.h"
#include "duel_layout.h"
#include "../appconfig.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <set>

namespace SurvivalLayout
{

namespace
{
    const int TILES = 26;

    int tile() { return AppConfig::tile_rect.w; }

    bool known(char c) { return c == '.' || c == '#' || c == '@' || c == '%' || c == '~' || c == '-'; }

    // Tanque passa por vazio, arbusto e gelo; tijolo, pedra e água bloqueiam
    bool passable(char c) { return c == '.' || c == '%' || c == '-'; }

    // O mapa como o jogo monta: águia no lugar ('E') e muralha de tijolos em volta
    std::vector<std::string> assembled(std::vector<std::string> grid)
    {
        for(const Tile& t : baseWallTiles()) grid[t.row][t.column] = '#';
        for(int r = TILES - 2; r < TILES; r++)
            for(int c = 12; c < 14; c++) grid[r][c] = 'E';
        return grid;
    }

    // Um tanque (2x2 tiles) cabe com o canto superior esquerdo em (r, c)
    bool fits(const std::vector<std::string>& grid, int r, int c)
    {
        if(r < 0 || c < 0 || r + 1 >= TILES || c + 1 >= TILES) return false;
        for(int i = 0; i < 2; i++)
            for(int j = 0; j < 2; j++)
                if(!passable(grid[r + i][c + j])) return false;
        return true;
    }

    // Posições (canto superior esquerdo) que um tanque alcança a partir de (row, column)
    std::set<std::pair<int, int>> reach(const std::vector<std::string>& grid, int row, int column)
    {
        std::set<std::pair<int, int>> seen = {{row, column}};
        std::deque<std::pair<int, int>> queue = {{row, column}};
        while(!queue.empty())
        {
            auto [r, c] = queue.front();
            queue.pop_front();
            const int dr[] = {1, -1, 0, 0}, dc[] = {0, 0, 1, -1};
            for(int k = 0; k < 4; k++)
            {
                std::pair<int, int> next = {r + dr[k], c + dc[k]};
                if(!seen.count(next) && fits(grid, next.first, next.second))
                {
                    seen.insert(next);
                    queue.push_back(next);
                }
            }
        }
        return seen;
    }

    // Os 2x2 de (r1, c1) e (r2, c2) se encostam ou se cobrem
    bool overlaps(int r1, int c1, int r2, int c2) { return std::abs(r1 - r2) < 2 && std::abs(c1 - c2) < 2; }
}

std::vector<Tile> baseWallTiles()
{
    std::vector<Tile> tiles;
    for(int r = TILES - 3; r < TILES; r++)
    {
        tiles.push_back({r, 11});
        tiles.push_back({r, 14});
    }
    tiles.push_back({TILES - 3, 12});
    tiles.push_back({TILES - 3, 13});
    return tiles;
}

std::vector<std::string> validate(const std::vector<std::string>& original)
{
    std::vector<std::string> problems;
    if(original.size() != static_cast<size_t>(TILES))
        problems.push_back("tem " + std::to_string(original.size()) + " linhas (precisa de " + std::to_string(TILES) + ")");
    for(size_t r = 0; r < original.size(); r++)
    {
        if(original[r].size() != static_cast<size_t>(TILES))
            problems.push_back("linha " + std::to_string(r) + " tem " + std::to_string(original[r].size()) +
                               " colunas (precisa de " + std::to_string(TILES) + ")");
        for(size_t c = 0; c < original[r].size(); c++)
            if(!known(original[r][c]))
                problems.push_back(std::string("símbolo desconhecido '") + original[r][c] + "' na linha " +
                                   std::to_string(r) + ", coluna " + std::to_string(c));
    }
    if(!problems.empty()) return problems;

    // Como o jogo monta: águia no lugar e muralha de tijolos em volta
    std::vector<std::string> grid = assembled(original);

    // Pontos de surgimento (inimigos) e de nascimento (jogadores), em tiles
    struct Spot { std::string name; int row, column; };
    std::vector<Spot> spots;
    for(size_t i = 0; i < AppConfig::enemy_starting_point.size(); i++)
    {
        SDL_Point p = AppConfig::enemy_starting_point[i];
        spots.push_back({"surgimento de inimigos " + std::to_string(i + 1), p.y / tile(), p.x / tile()});
    }
    for(size_t i = 0; i < AppConfig::player_starting_point.size(); i++)
    {
        SDL_Point p = AppConfig::player_starting_point[i];
        spots.push_back({"nascimento do P" + std::to_string(i + 1), p.y / tile(), p.x / tile()});
    }
    for(const Spot& s : spots)
        if(!fits(grid, s.row, s.column)) problems.push_back(s.name + " bloqueado");
    if(!problems.empty()) return problems;

    // Alcance a partir do primeiro ponto: todos os outros têm de estar ligados a ele
    std::set<std::pair<int, int>> seen = reach(grid, spots[0].row, spots[0].column);
    for(size_t i = 1; i < spots.size(); i++)
        if(!seen.count({spots[i].row, spots[i].column}))
            problems.push_back(spots[i].name + ": sem caminho da largura de um tanque até " + spots[0].name);

    // Algum tanque encosta na muralha da águia
    bool touches = false;
    for(auto& p : seen)
        for(const Tile& t : baseWallTiles())
            for(int i = 0; i < 2; i++)
                for(int j = 0; j < 2; j++)
                    if(std::abs(p.first + i - t.row) + std::abs(p.second + j - t.column) == 1) touches = true;
    if(!touches) problems.push_back("nenhum caminho da largura de um tanque chega à muralha da águia");
    return problems;
}

std::vector<Tile> shopSpots(const std::vector<std::string>& original)
{
    std::vector<Tile> spots;
    if(original.size() != static_cast<size_t>(TILES)) return spots;
    for(const std::string& line : original)
        if(line.size() != static_cast<size_t>(TILES)) return spots;

    std::vector<std::string> grid = assembled(original);
    SDL_Point p1 = AppConfig::player_starting_point.at(0);
    std::set<std::pair<int, int>> seen = reach(grid, p1.y / tile(), p1.x / tile());

    std::vector<SDL_Point> taken(AppConfig::player_starting_point.begin(), AppConfig::player_starting_point.end());
    taken.insert(taken.end(), AppConfig::enemy_starting_point.begin(), AppConfig::enemy_starting_point.end());
    for(auto [r, c] : seen)
    {
        // Metade de baixo, ao lado da base (à esquerda ou à direita das colunas 11-14 dela,
        // nunca na frente), em chão à vista (arbusto esconderia a loja), sem cobrir ponto de
        // nascimento
        if(r < TILES / 2 || (c + 1 >= 11 && c <= 14)) continue;
        bool floor = true;
        for(int i = 0; i < 2; i++)
            for(int j = 0; j < 2; j++)
                if(grid[r + i][c + j] != '.' && grid[r + i][c + j] != '-') floor = false;
        if(!floor) continue;
        bool free = true;
        for(SDL_Point p : taken)
            if(overlaps(r, c, p.y / tile(), p.x / tile())) free = false;
        if(free) spots.push_back({r, c});
    }
    // Do mais perto para o mais longe da águia (centro em 24,5 x 12,5, em meios tiles)
    auto distance = [](const Tile& t) {
        int dr = 2 * t.row + 1 - 49, dc = 2 * t.column + 1 - 25;
        return dr * dr + dc * dc;
    };
    std::stable_sort(spots.begin(), spots.end(), [&](const Tile& a, const Tile& b) { return distance(a) < distance(b); });
    return spots;
}

int loadMapList()
{
    std::ifstream list(AppConfig::survival_levels_path + "maps.txt");
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

        std::vector<std::string> problems = validate(DuelLayout::readMap(AppConfig::survival_levels_path + file));
        if(!problems.empty())
        {
            std::cerr << "Mapa de sobrevivência \"" << name << "\" (" << file << ") recusado:\n";
            for(const std::string& p : problems) std::cerr << "  - " << p << "\n";
            rejected++;
            continue;
        }
        maps.push_back({file, name});
    }
    if(!maps.empty()) AppConfig::survival_maps = maps;
    return rejected;
}

} // namespace SurvivalLayout
