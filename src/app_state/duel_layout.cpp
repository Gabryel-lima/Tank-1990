#include "duel_layout.h"
#include "../appconfig.h"

#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
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

    // Uma volta de 90 graus no sentido horário: o que estava embaixo vai para a esquerda
    Tile turnCell(Tile t) { return {t.column, TILES - 1 - t.row}; }
    Tile turnBlock(Tile t) { return {t.column, TILES - 2 - t.row}; }

    // Linha da águia da equipe A (a última dupla de linhas do mapa)
    const int A_BASE_ROW = TILES - 2;

    const char TEAM_LETTER[4] = {'A', 'B', 'C', 'D'};
}

int quarterTurns(int team)
{
    static const int turns[4] = {0, 2, 1, 3};
    return turns[team & 3];
}

Tile placeCell(int team, Tile a_frame)
{
    for(int k = 0; k < quarterTurns(team); k++) a_frame = turnCell(a_frame);
    return a_frame;
}

Tile placeBlock(int team, Tile a_frame)
{
    for(int k = 0; k < quarterTurns(team); k++) a_frame = turnBlock(a_frame);
    return a_frame;
}

Tile frameCell(int team, Tile map)
{
    for(int k = 0; k < (4 - quarterTurns(team)) % 4; k++) map = turnCell(map);
    return map;
}

Tile frameBlock(int team, Tile map)
{
    for(int k = 0; k < (4 - quarterTurns(team)) % 4; k++) map = turnBlock(map);
    return map;
}

Tile baseOrigin(int team)
{
    return placeBlock(team, {A_BASE_ROW, BASE_COLUMN});
}

std::vector<Tile> baseWallTiles(int team)
{
    // Equipe A: colunas ao lado da águia nas suas duas linhas e na da frente, mais a linha da frente
    int front = A_BASE_ROW - 1;
    std::vector<Tile> tiles;
    for(int r : {A_BASE_ROW, A_BASE_ROW + 1, front})
    {
        tiles.push_back(placeCell(team, {r, BASE_COLUMN - 1}));
        tiles.push_back(placeCell(team, {r, BASE_COLUMN + 2}));
    }
    tiles.push_back(placeCell(team, {front, BASE_COLUMN}));
    tiles.push_back(placeCell(team, {front, BASE_COLUMN + 1}));
    return tiles;
}

std::vector<Tile> baseFrontTiles(int team)
{
    int front = A_BASE_ROW - 1;
    return {placeCell(team, {front, BASE_COLUMN}), placeCell(team, {front, BASE_COLUMN + 1})};
}

Tile yardCenter(int team)
{
    return placeBlock(team, {A_BASE_ROW - 3, BASE_COLUMN});
}

std::vector<Tile> guardPosts(int team)
{
    // Um defensor fica no centro do pátio, à mesma distância dos dois flancos; os outros,
    // duas linhas à frente dele, fora do pátio
    Tile center = {A_BASE_ROW - 3, BASE_COLUMN};
    int outside_row = center.row - 2;
    return {placeBlock(team, center), placeBlock(team, {outside_row, center.column - 2}),
            placeBlock(team, {outside_row, center.column + 2})};
}

Tile lookTile(int team, Tile post)
{
    // Do outro lado do mapa, na coluna do posto (vista da equipe)
    Tile a = frameBlock(team, post);
    return placeBlock(team, {0, a.column});
}

bool inSpawnExit(int team, SDL_Point spawn, int row, int column)
{
    // As quatro linhas à frente do nascimento e as duas dele (na visão da equipe A): sobreposição
    // do tanque 2x2 com esse retângulo, que é a mesma em qualquer rotação
    Tile s = frameBlock(team, {spawn.y / tile(), spawn.x / tile()});
    Tile t = frameBlock(team, {row, column});
    int r0 = s.row - 4, r1 = s.row + 1;
    return t.row + 1 >= r0 && t.row <= r1 && t.column + 1 >= s.column && t.column <= s.column + 1;
}

int flankSide(const std::vector<std::string>& grid, int team, int row, int column)
{
    // Tudo na visão da equipe A (as outras giram): a águia embaixo, o tanque nas duas linhas dela
    Tile t = frameBlock(team, {row, column});
    if(t.row != A_BASE_ROW) return 0;
    int wall_left = BASE_COLUMN - 1, wall_right = BASE_COLUMN + 2;
    int from, to, side;
    if(t.column + 1 < wall_left) { from = t.column + 2; to = wall_left - 1; side = -1; }
    else if(t.column > wall_right) { from = wall_right + 1; to = t.column - 1; side = 1; }
    else return 0;
    for(int r = A_BASE_ROW; r < A_BASE_ROW + 2; r++)
        for(int c = from; c <= to; c++)
        {
            Tile m = placeCell(team, {r, c});
            if(grid[m.row][m.column] == '@') return 0;
        }
    return side;
}

bool isBaseFront(int team, int row, int column)
{
    for(const Tile& t : baseFrontTiles(team))
        if(t.row == row && t.column == column) return true;
    return false;
}

bool isBaseWall(int team, int row, int column)
{
    for(const Tile& t : baseWallTiles(team))
        if(t.row == row && t.column == column) return true;
    return false;
}

bool isBaseTile(int row, int column, int teams)
{
    for(int team = 0; team < teams; team++)
    {
        Tile origin = baseOrigin(team);
        if(row >= origin.row && row < origin.row + 2 && column >= origin.column && column < origin.column + 2) return true;
    }
    return false;
}

int zoneTeam(int row, int column, int teams)
{
    // A zona da equipe A: colunas BASE_COLUMN - ZONE_SIDE .. BASE_COLUMN + 1 + ZONE_SIDE e as
    // ZONE_DEPTH últimas linhas; as das outras equipes são ela girada
    for(int team = 0; team < teams; team++)
    {
        Tile a = frameCell(team, {row, column});
        if(a.column >= BASE_COLUMN - ZONE_SIDE && a.column <= BASE_COLUMN + 1 + ZONE_SIDE && a.row >= TILES - ZONE_DEPTH)
            return team;
    }
    return -1;
}

bool inBaseZone(int row, int column, int teams)
{
    return zoneTeam(row, column, teams) >= 0;
}

std::vector<SDL_Point> spawnOrder(int team)
{
    // Equipe A: colunas na ordem de AppConfig; as outras são a A girada (a B troca os lados)
    std::vector<SDL_Point> order;
    int a_row = AppConfig::duel_spawn_rows.at(0) / tile();
    for(int x : AppConfig::duel_spawn_columns)
    {
        Tile t = placeBlock(team, {a_row, x / tile()});
        order.push_back({t.column * tile(), t.row * tile()});
    }
    return order;
}

std::vector<SDL_Point> midBonusSpots(int teams)
{
    int t = tile(), mid = TILES / 2 - 1;
    if(teams <= 2)
        return {{BASE_COLUMN * t, mid * t}, {4 * t, mid * t}, {(TILES - 6) * t, mid * t}};

    // Quatro bases: o centro (invariante por rotação) e as quatro diagonais, uma órbita da rotação
    std::vector<SDL_Point> spots = {{(TILES / 2 - 1) * t, (TILES / 2 - 1) * t}};
    Tile diagonal = {6, 6};
    for(int k = 0; k < 4; k++)
    {
        spots.push_back({diagonal.column * t, diagonal.row * t});
        diagonal = turnBlock(diagonal);
    }
    return spots;
}

std::vector<SDL_Point> halfBonusSpots(int team)
{
    // Equipe A (o central, à frente do pátio da base, por último); as outras são a A girada
    std::vector<SDL_Point> spots;
    for(Tile a : {Tile{16, 4}, Tile{16, TILES - 6}, Tile{17, BASE_COLUMN}})
    {
        Tile p = placeBlock(team, a);
        spots.push_back({p.column * tile(), p.row * tile()});
    }
    return spots;
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

namespace
{
// Erros (o mapa não funciona: é recusado) e avisos (funciona, mas vale rever) do mapa
void analyze(const std::vector<std::string>& original, int teams, std::vector<std::string>& problems, std::vector<std::string>& warnings)
{

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
    if(!problems.empty()) return;

    // O mapa como o jogo o monta: espaço das águias vazio, muralha de tijolo e a frente de pedra
    std::vector<std::string> grid = original;
    for(int team = 0; team < teams; team++)
    {
        for(const Tile& t : baseWallTiles(team)) grid[t.row][t.column] = '#';
        for(const Tile& t : baseFrontTiles(team)) grid[t.row][t.column] = '@';
        Tile origin = baseOrigin(team);
        for(int r = origin.row; r < origin.row + 2; r++)
            for(int c = origin.column; c < origin.column + 2; c++) grid[r][c] = 'E';
    }

    // Justiça: as equipes precisam ver o mesmo terreno. As bases, as zonas, os pontos de
    // nascimento (spawnOrder) e os de bônus de cada equipe são os da A girados, então basta o
    // mapa girado ser igual a si mesmo: 180 graus com duas equipes, 90 com quatro. Espelho
    // também passa (é um caso particular), mas não é exigido: cata-ventos, diagonais e
    // outros desenhos girados valem
    for(int r = 0; r < TILES; r++)
        for(int c = 0; c < TILES; c++)
        {
            int r2 = teams <= 2 ? TILES - 1 - r : c, c2 = teams <= 2 ? TILES - 1 - c : TILES - 1 - r;
            if(original[r][c] != original[r2][c2])
            {
                problems.push_back(std::string("não é simétrico girando ") + (teams <= 2 ? "180" : "90") +
                                   " graus (as equipes teriam terrenos diferentes): a " + where(r, c) +
                                   " e a " + where(r2, c2) + " precisam ter o mesmo bloco");
                r = TILES; break;
            }
        }

    // Posição (canto superior esquerdo, em tiles) de um tanque 2x2 que cabe ali
    auto fits = [&](int r, int c) {
        if(r < 0 || c < 0 || r + 1 >= TILES || c + 1 >= TILES) return false;
        for(int i = 0; i < 2; i++)
            for(int j = 0; j < 2; j++)
                if(!passable(grid[r + i][c + j])) return false;
        return true;
    };
    // Posições alcançáveis e a distância (em passos de 1 tile) até cada uma
    auto reach = [&](int r0, int c0) {
        std::map<std::pair<int, int>, int> seen = {{{r0, c0}, 0}};
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
                    seen[next] = seen[{r, c}] + 1;
                    queue.push_back(next);
                }
            }
        }
        return seen;
    };
    // Menor distância das posições alcançadas até um ponto de tiro no flanco @a side
    // (-1 esquerdo, +1 direito, 0 qualquer um) da base da equipe; -1 se nenhum é alcançado
    auto flankDistance = [&](const std::map<std::pair<int, int>, int>& seen, int team, int side) {
        int best = -1;
        for(auto& p : seen)
        {
            int s = flankSide(grid, team, p.first.first, p.first.second);
            if(s == 0 || (side != 0 && s != side)) continue;
            if(best < 0 || p.second < best) best = p.second;
        }
        return best;
    };

    std::vector<std::pair<int, int>> bonus;
    for(SDL_Point p : midBonusSpots(teams)) bonus.push_back({p.y / tile(), p.x / tile()});
    for(int team = 0; team < teams; team++)
        for(SDL_Point p : halfBonusSpots(team)) bonus.push_back({p.y / tile(), p.x / tile()});
    for(auto& b : bonus)
        if(!fits(b.first, b.second))
            warnings.push_back("ponto de bônus na " + where(b.first, b.second) + " coberto: o bônus não surge ali enquanto o bloco estiver de pé");

    for(int team = 0; team < teams; team++)
        for(SDL_Point s : spawnOrder(team))
        {
            int r = s.y / tile(), c = s.x / tile();
            std::string name = std::string("nascimento da equipe ") + TEAM_LETTER[team] + " na " + where(r, c);
            if(!fits(r, c))
            {
                problems.push_back(name + ": bloqueado");
                continue;
            }
            auto seen = reach(r, c);
            for(int enemy = 0; enemy < teams; enemy++)
                if(enemy != team && flankDistance(seen, enemy, 0) < 0)
                    problems.push_back(name + ": não há caminho da largura de um tanque até um flanco da base da equipe " +
                                       TEAM_LETTER[enemy]);
            // O defensor precisa chegar aos dois flancos da própria base (contornando a águia)
            for(int side : {-1, 1})
            {
                std::string flank = side < 0 ? "esquerdo" : "direito";
                int home = flankDistance(seen, team, side);
                if(home < 0)
                    warnings.push_back(name + ": não alcança o flanco " + flank + " da própria base (o defensor não contorna a águia)");
                else if(home > DEFENDER_REACH)
                    warnings.push_back(name + ": flanco " + flank + " da própria base a " + std::to_string(home) + " passos (sugerido: até " +
                                       std::to_string(DEFENDER_REACH) + "): o defensor demora para contornar a águia");
            }
            for(auto& b : bonus)
                if(fits(b.first, b.second) && !seen.count(b))
                    warnings.push_back(name + ": não alcança, sem barco nem tiro forte, o ponto de bônus da " + where(b.first, b.second));
        }
}
}

std::vector<std::string> validate(const std::vector<std::string>& grid, int teams)
{
    std::vector<std::string> problems, warnings;
    analyze(grid, teams, problems, warnings);
    return problems;
}

std::vector<std::string> advise(const std::vector<std::string>& grid, int teams)
{
    std::vector<std::string> problems, warnings;
    analyze(grid, teams, problems, warnings);
    return problems.empty() ? warnings : std::vector<std::string>();
}

std::vector<std::vector<int>> stoneOwners(const std::vector<std::string>& grid, int teams)
{
    // Blocos de pedra ligados (lado com lado) formam uma parede. A parede inteira dentro da
    // zona de uma base é da equipe; a que cruza a borda (ou fica fora) é pedra comum. Assim
    // uma parede nunca fica metade de cada jeito, e o mapa não precisa evitar a borda. A
    // muralha da águia não entra aqui: é sempre da equipe (ver Duel::stoneOwner)
    std::vector<std::vector<int>> owner(TILES, std::vector<int>(TILES, -1));
    std::vector<std::vector<bool>> seen(TILES, std::vector<bool>(TILES, false));
    auto stone = [&](int r, int c) {
        if(r < 0 || c < 0 || r >= TILES || c >= TILES || r >= static_cast<int>(grid.size()) ||
           c >= static_cast<int>(grid[r].size()) || grid[r][c] != '@') return false;
        for(int team = 0; team < teams; team++)
            if(isBaseWall(team, r, c)) return false;
        return true;
    };
    for(int r0 = 0; r0 < TILES; r0++)
        for(int c0 = 0; c0 < TILES; c0++)
        {
            if(seen[r0][c0] || !stone(r0, c0)) continue;
            std::vector<std::pair<int, int>> wall = {{r0, c0}};
            seen[r0][c0] = true;
            int team = zoneTeam(r0, c0, teams);
            for(size_t k = 0; k < wall.size(); k++)
            {
                auto [r, c] = wall[k];
                if(zoneTeam(r, c, teams) != team) team = -1;
                const int dr[] = {1, -1, 0, 0}, dc[] = {0, 0, 1, -1};
                for(int d = 0; d < 4; d++)
                {
                    int nr = r + dr[d], nc = c + dc[d];
                    if(stone(nr, nc) && !seen[nr][nc]) { seen[nr][nc] = true; wall.push_back({nr, nc}); }
                }
            }
            for(auto [r, c] : wall) owner[r][c] = team;
        }
    return owner;
}


int protectedStone(const std::vector<std::string>& grid, int teams)
{
    int count = 0;
    for(const auto& row : stoneOwners(grid, teams))
        for(int owner : row)
            if(owner >= 0) count++;
    return count;
}

namespace
{
    // Uma lista de mapas: acrescenta em @a maps os que passam em validate para @a teams equipes
    int loadList(const std::string& folder, int teams, std::vector<std::pair<std::string, std::string>>& maps)
    {
        std::ifstream list(folder + "maps.txt");
        if(!list.is_open()) return 0;

        int rejected = 0;
        std::string line;
        while(std::getline(list, line))
        {
            while(!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
            if(line.empty() || line[0] == '#') continue;
            size_t sep = line.find(';');
            std::string file = line.substr(0, sep);
            std::string name = (sep == std::string::npos) ? file : line.substr(sep + 1);

            std::vector<std::string> problems = validate(readMap(folder + file), teams);
            if(!problems.empty())
            {
                std::cerr << "Mapa de duelo \"" << name << "\" (" << file << ") recusado:\n";
                for(const std::string& p : problems) std::cerr << "  - " << p << "\n";
                rejected++;
                continue;
            }
            maps.push_back({file, name});
        }
        return rejected;
    }
}

int loadMapList()
{
    std::vector<std::pair<std::string, std::string>> maps, ffa_maps;
    int rejected = loadList(AppConfig::duel_levels_path, 2, maps);
    rejected += loadList(AppConfig::duel_ffa_levels_path, FFA_TEAMS, ffa_maps);
    if(!maps.empty()) AppConfig::duel_maps = maps;
    if(!ffa_maps.empty()) AppConfig::duel_ffa_maps = ffa_maps;
    return rejected;
}

} // namespace DuelLayout
