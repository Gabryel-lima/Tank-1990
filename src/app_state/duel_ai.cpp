// IA do modo duelo: bots de reforço e jogadores do computador (simulação).
//
// Cada tanque escolhe um destino (base inimiga, um inimigo, a guarda da própria base ou
// um bônus), calcula com NavGrid a distância de cada célula do mapa até ele e anda para
// a célula vizinha mais próxima. Assim contorna paredes e rios em qualquer mapa, em vez
// de empurrar a parede na direção do alvo (o que deixava o bot preso nos cantos).

#include "duel.h"
#include "duel_layout.h"
#include "../appconfig.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>

namespace
{
    // Distâncias em blocos de 16 px
    const int CHASE_RANGE = 10;   ///< atacante troca a base por um inimigo a esta distância...
    const int CHASE_KEEP = 14;    ///< ...e só desiste dele se passar desta (senão a decisão oscila)
    const int INVADER_RANGE = 13; ///< defensor caça quem chega a esta distância da base...
    const int INVADER_KEEP = 17;  ///< ...e só volta ao posto se o invasor se afastar além desta
    const int BASE_FIRE_RANGE = 12;
    const int TANK_FIRE_RANGE = 9;
    const int SHOT_RANGE = 14;    ///< atira em quem estiver alinhado até esta distância
    const int TURN_RANGE = 6;     ///< para e vira para atirar em quem estiver ao lado até aqui
    const int BONUS_RANGE = 45;   ///< jogador do computador busca bônus a este custo de caminho

    const Uint32 REPLAN_TIME = 250;
    const Uint32 STUCK_TIME = 350;
    const Uint32 AIM_MAX = 2000;      ///< segura a mira num alvo no máximo este tempo e segue o caminho
    const Uint32 AIM_COOLDOWN = 500;  ///< depois de soltar a mira, espera para virar de lado de novo
    const Uint32 MIN_BEFORE_REVERSE = 300; ///< anda ao menos isto numa direção antes de dar meia-volta

    int tile() { return AppConfig::tile_rect.w; }

    SDL_Point centerOf(const SDL_Rect& r)
    {
        return {r.x + r.w / 2, r.y + r.h / 2};
    }

    int manhattan(SDL_Point a, SDL_Point b)
    {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }

    // Direção do ponto a até o ponto b, pelo eixo de maior distância
    Direction directionTo(SDL_Point a, SDL_Point b)
    {
        int dx = b.x - a.x, dy = b.y - a.y;
        if(std::abs(dx) > std::abs(dy)) return dx < 0 ? D_LEFT : D_RIGHT;
        return dy < 0 ? D_UP : D_DOWN;
    }

    const int DR[4] = {-1, 0, 1, 0}; // indexados por Direction (D_UP, D_RIGHT, D_DOWN, D_LEFT)
    const int DC[4] = {0, 1, 0, -1};
}

bool Duel::isAI(Tank* tank) const
{
    if(dynamic_cast<Bot*>(tank) != nullptr) return true;
    Player* player = dynamic_cast<Player*>(tank);
    return player != nullptr && player->cpu;
}

Bot::Role Duel::roleOf(Tank* tank) const
{
    if(Bot* bot = dynamic_cast<Bot*>(tank)) return bot->role;

    // Jogador do computador: o primeiro da equipe ataca, o segundo defende, e assim por diante
    Player* player = dynamic_cast<Player*>(tank);
    int before = 0;
    for(int i = 0; player != nullptr && i < player->playerIndex(); i++)
        if(m_config.human_team[i] == player->team) before++;
    return before % 2 == 0 ? Bot::ROLE_ATTACK : Bot::ROLE_DEFEND;
}

NavGrid::Abilities Duel::abilitiesOf(Tank* tank) const
{
    NavGrid::Abilities abilities;
    abilities.boat = tank->testFlag(TSF_BOAT);
    Player* player = dynamic_cast<Player*>(tank);
    abilities.break_stone = (player != nullptr && player->stars() >= 3);
    return abilities;
}

void Duel::buildNavGrids()
{
    std::vector<Eagle*> all_bases = bases();
    for(int team = 0; team < 2; team++)
    {
        NavGrid& nav = m_nav[team];
        nav.resize(m_level_rows_count, m_level_columns_count);
        for(int row = 0; row < m_level_rows_count; row++)
            for(int column = 0; column < m_level_columns_count; column++)
            {
                Object* o = m_level.at(row).at(column);
                if(o == nullptr) continue;

                NavGrid::Tile t = NavGrid::TILE_FREE;
                if(o->type == ST_BRICK_WALL) t = NavGrid::TILE_BRICK;
                else if(o->type == ST_STONE_WALL) t = NavGrid::TILE_STONE;
                else if(o->type == ST_WATER) t = NavGrid::TILE_WATER;
                // A IA não abre a muralha da própria base (o jogador pode, a IA evita: ver
                // firesAtOwnBase); e pedra na zona das bases não cai nem com o canhão
                // (powerAppliesAt), então não adianta planejar atravessá-la
                if(t != NavGrid::TILE_FREE && t != NavGrid::TILE_WATER && isBaseWall(team, row, column))
                    t = NavGrid::TILE_BLOCKED;
                if(t == NavGrid::TILE_STONE && isInBaseZone(row, column))
                    t = NavGrid::TILE_BLOCKED;
                nav.setTile(row, column, t);
            }

        // As águias bloqueiam a passagem (e o tiro, para fireGoals)
        for(Eagle* base : all_bases)
        {
            int row = base->collision_rect.y / tile(), column = base->collision_rect.x / tile();
            for(int r = row; r < row + 2; r++)
                for(int c = column; c < column + 2; c++)
                    nav.setTile(r, c, NavGrid::TILE_BLOCKED);
        }
    }
}

std::vector<int> Duel::fireGoals(const SDL_Rect& target, int team, const NavGrid::Abilities& abilities, int range) const
{
    const NavGrid& nav = m_nav[team];
    int tr = static_cast<int>(std::lround(static_cast<double>(target.y) / tile()));
    int tc = static_cast<int>(std::lround(static_cast<double>(target.x) / tile()));

    std::vector<int> goals;
    for(int d = 0; d < 4; d++)
    {
        int bricks = 0;
        for(int k = 2; k <= range; k++)
        {
            // Faixa de blocos entre o tanque (a k células) e o alvo, por onde o tiro passa
            if(k >= 3)
            {
                int r0, c0, r1, c1;
                if(DC[d] == 0)
                {
                    r0 = r1 = tr + DR[d] * k + (DR[d] < 0 ? 2 : -1);
                    c0 = tc; c1 = tc + 1;
                }
                else
                {
                    c0 = c1 = tc + DC[d] * k + (DC[d] < 0 ? 2 : -1);
                    r0 = tr; r1 = tr + 1;
                }
                bool blocked = false, brick = false;
                for(NavGrid::Tile t : {nav.tile(r0, c0), nav.tile(r1, c1)})
                {
                    if(t == NavGrid::TILE_BLOCKED || (t == NavGrid::TILE_STONE && !abilities.break_stone)) blocked = true;
                    if(t == NavGrid::TILE_BRICK || t == NavGrid::TILE_STONE) brick = true;
                }
                if(brick) bricks++;
                if(blocked || bricks > 2) break;
            }

            int r = tr + DR[d] * k, c = tc + DC[d] * k;
            if(!nav.validCell(r, c)) break;
            if(nav.cellCost(r, c, abilities) < NavGrid::UNREACHABLE) goals.push_back(nav.cellIndex(r, c));
        }
    }
    return goals;
}

bool Duel::clearShot(Tank* shooter, Direction d, const SDL_Rect& target, int range) const
{
    const NavGrid& nav = m_nav[shooter->team];
    bool heavy = abilitiesOf(shooter).break_stone;
    SDL_Rect me = shooter->dest_rect;
    SDL_Point c = centerOf(me);
    SDL_Point tc = centerOf(target);
    const int half_bullet = 4; // o projétil tem 8x8 e sai do centro do tanque

    bool vertical = (d == D_UP || d == D_DOWN);
    // Alinhado: o projétil passa por dentro do alvo
    if(vertical ? std::abs(tc.x - c.x) >= target.w / 2 + half_bullet
                : std::abs(tc.y - c.y) >= target.h / 2 + half_bullet) return false;

    // À frente: intervalo [from, to) do eixo entre a frente do tanque e o alvo
    int from, to;
    switch(d)
    {
    case D_UP:    from = target.y + target.h; to = me.y; break;
    case D_DOWN:  from = me.y + me.h;         to = target.y; break;
    case D_LEFT:  from = target.x + target.w; to = me.x; break;
    default:      from = me.x + me.w;         to = target.x; break;
    }
    if(to < from - 2) return false;          // alvo atrás (ou ao lado, sobreposto)
    if(to - from > range * tile()) return false;

    // Blocos atravessados pelo projétil
    int lane = vertical ? c.x : c.y;
    int lane0 = (lane - half_bullet) / tile(), lane1 = (lane + half_bullet - 1) / tile();
    int first = std::max(from, 0) / tile(), last = (to - 1) / tile();
    int bricks = 0;
    for(int i = first; i <= last; i++)
    {
        bool brick = false;
        for(int j = lane0; j <= lane1; j++)
        {
            NavGrid::Tile t = vertical ? nav.tile(i, j) : nav.tile(j, i);
            if(t == NavGrid::TILE_BLOCKED || (t == NavGrid::TILE_STONE && !heavy)) return false;
            if(t == NavGrid::TILE_BRICK || t == NavGrid::TILE_STONE) brick = true;
        }
        if(brick && ++bricks > 2) return false;
    }
    return true;
}

bool Duel::worthFiring(Tank* shooter, Direction d, int range)
{
    int enemy_team = 1 - shooter->team;
    for(Tank* t : allTanks())
    {
        if(t->team != enemy_team || t->to_erase || !t->testFlag(TSF_LIFE)) continue;
        // Escudo: o tiro não faz nada (mas ainda vale anular os projéteis dele)
        if(!t->testFlag(TSF_SHIELD) && clearShot(shooter, d, t->collision_rect, range)) return true;

        // Projétil inimigo vindo na direção do tanque: atira para anular
        for(Bullet* b : t->bullets)
            if(!b->to_erase && !b->collide && (b->direction + 2) % 4 == d &&
               clearShot(shooter, d, b->collision_rect, 6))
                return true;
    }

    Eagle* base = baseOf(enemy_team);
    return base != nullptr && base->type == ST_EAGLE && clearShot(shooter, d, base->collision_rect, BASE_FIRE_RANGE);
}

bool Duel::firesAtOwnBase(Tank* tank, Direction d) const
{
    // Percorre a faixa do projétil (8 px no centro do tanque) até a borda do mapa ou uma
    // pedra: se passa pelos tijolos da muralha ou pela águia da própria equipe, não atira.
    // Tijolos de outros lugares no meio não contam: tiros seguidos acabam atravessando
    int team = tank->team;
    if(team < 0) return false;
    SDL_Point c = centerOf(tank->dest_rect);
    bool vertical = (d == D_UP || d == D_DOWN);
    int lane = vertical ? c.x : c.y;
    int lane0 = (lane - 4) / tile(), lane1 = (lane + 3) / tile();
    int start = (vertical ? c.y : c.x) / tile();
    int step = (d == D_UP || d == D_LEFT) ? -1 : 1;
    int limit = vertical ? m_level_rows_count : m_level_columns_count;
    for(int i = start; i >= 0 && i < limit; i += step)
        for(int j = lane0; j <= lane1; j++)
        {
            int row = vertical ? i : j, column = vertical ? j : i;
            if(row < 0 || column < 0 || row >= m_level_rows_count || column >= m_level_columns_count) continue;
            int base_row = DuelLayout::baseRow(team);
            if(row >= base_row && row < base_row + 2 && column >= DuelLayout::BASE_COLUMN && column < DuelLayout::BASE_COLUMN + 2)
                return true;
            // Pedra (inclusive a frente da própria base) segura o tiro sem dano; tijolo da
            // própria muralha, não
            Object* o = m_level.at(row).at(column);
            if(o != nullptr && o->type == ST_STONE_WALL) return false;
            if(o != nullptr && isBaseWall(team, row, column)) return true;
        }
    return false;
}

bool Duel::breakableAhead(Tank* tank, Direction d) const
{
    const NavGrid& nav = m_nav[tank->team];
    bool heavy = abilitiesOf(tank).break_stone;
    SDL_Rect r = tank->collision_rect;

    // Blocos encostados na frente do tanque
    int r0, r1, c0, c1;
    switch(d)
    {
    case D_UP:    r0 = r1 = (r.y - 3) / tile(); c0 = r.x / tile(); c1 = (r.x + r.w - 1) / tile(); break;
    case D_DOWN:  r0 = r1 = (r.y + r.h + 2) / tile(); c0 = r.x / tile(); c1 = (r.x + r.w - 1) / tile(); break;
    case D_LEFT:  c0 = c1 = (r.x - 3) / tile(); r0 = r.y / tile(); r1 = (r.y + r.h - 1) / tile(); break;
    default:      c0 = c1 = (r.x + r.w + 2) / tile(); r0 = r.y / tile(); r1 = (r.y + r.h - 1) / tile(); break;
    }
    if(r.x - 3 < 0 && d == D_LEFT) return false;
    if(r.y - 3 < 0 && d == D_UP) return false;

    for(int row = r0; row <= r1; row++)
        for(int column = c0; column <= c1; column++)
        {
            NavGrid::Tile t = nav.tile(row, column);
            if(t == NavGrid::TILE_BRICK || (t == NavGrid::TILE_STONE && heavy)) return true;
        }
    return false;
}

void Duel::planAI(Tank* tank, AIState& state)
{
    int team = tank->team, enemy_team = 1 - team;
    Eagle* own_base = baseOf(team);
    Eagle* enemy_base = baseOf(enemy_team);
    const NavGrid& nav = m_nav[team];
    NavGrid::Abilities abilities = abilitiesOf(tank);
    Bot::Role role = roleOf(tank);

    int my_row = static_cast<int>(std::lround(tank->pos_y / tile()));
    int my_column = static_cast<int>(std::lround(tank->pos_x / tile()));
    if(!nav.validCell(my_row, my_column)) return;
    int me = nav.cellIndex(my_row, my_column);

    // Tenta um destino: aceita se houver caminho a partir da posição do tanque
    auto tryGoals = [&](const std::vector<int>& goals, const SDL_Rect* face, int max_cost) {
        if(goals.empty()) return false;
        std::vector<int> field = nav.distanceField(goals, abilities);
        if(field[me] >= NavGrid::UNREACHABLE || field[me] > max_cost) return false;
        state.field.swap(field);
        state.face = face != nullptr ? *face : SDL_Rect{0, 0, 0, 0};
        return true;
    };

    // Inimigo mais próximo de um ponto de referência
    auto nearestEnemy = [&](SDL_Point reference, int* distance) {
        Tank* nearest = nullptr;
        int best = INT_MAX;
        for(Tank* t : allTanks())
        {
            if(t->team != enemy_team || t->to_erase || !t->testFlag(TSF_LIFE)) continue;
            int d = manhattan(centerOf(t->dest_rect), reference);
            if(d < best) { best = d; nearest = t; }
        }
        *distance = best;
        return nearest;
    };
    auto chase = [&](Tank* target) {
        return target != nullptr &&
               tryGoals(fireGoals(target->dest_rect, team, abilities, TANK_FIRE_RANGE), &target->dest_rect, NavGrid::UNREACHABLE - 1);
    };

    // Jogador do computador: busca os bônus que pode pegar, se o caminho não for longo
    Player* player = dynamic_cast<Player*>(tank);
    if(player != nullptr)
        for(Bonus* bonus : m_bonuses)
        {
            if(bonus->to_erase || (bonus->owner_team >= 0 && bonus->owner_team != team)) continue;
            int br = bonus->pos_y / tile(), bc = bonus->pos_x / tile();
            std::vector<int> goals;
            for(int r = br - 1; r <= br + 1; r++)
                for(int c = bc - 1; c <= bc + 1; c++)
                    if(nav.validCell(r, c)) goals.push_back(nav.cellIndex(r, c));
            if(tryGoals(goals, nullptr, BONUS_RANGE)) return;
        }

    int distance;
    if(role == Bot::ROLE_ATTACK)
    {
        // Histerese: começa a caçar um inimigo a CHASE_RANGE e continua até CHASE_KEEP. Com um
        // limite só, um inimigo andando em volta dele fazia a IA trocar de destino a cada
        // replanejamento e ir e voltar no mesmo lugar
        Tank* nearest = nearestEnemy(centerOf(tank->dest_rect), &distance);
        Tank* previous = nullptr;
        for(Tank* t : allTanks())
            if(t == state.chasing && t->team == enemy_team && !t->to_erase && t->testFlag(TSF_LIFE)) previous = t;
        state.chasing = nullptr;
        if(previous != nullptr && manhattan(centerOf(previous->dest_rect), centerOf(tank->dest_rect)) < CHASE_KEEP * tile() &&
           chase(previous))
        {
            state.chasing = previous;
            return;
        }
        if(distance < CHASE_RANGE * tile() && chase(nearest))
        {
            state.chasing = nearest;
            return;
        }
        // Base inimiga: posições alinhadas com ela (atira através da muralha de tijolos)
        if(enemy_base != nullptr && enemy_base->type == ST_EAGLE &&
           tryGoals(fireGoals(enemy_base->collision_rect, team, abilities, BASE_FIRE_RANGE), &enemy_base->collision_rect, NavGrid::UNREACHABLE - 1))
            return;
        // Base inalcançável (muralha de pedra): caça quem estiver em campo
        if(chase(nearest)) return;
    }
    else
    {
        SDL_Point base_center = own_base != nullptr ? centerOf(own_base->collision_rect) : centerOf(tank->dest_rect);
        Tank* invader = nearestEnemy(base_center, &distance);
        // Mesma histerese do atacante: sem ela, o guarda ia e voltava entre o posto e o invasor
        bool was_chasing = false;
        for(Tank* t : allTanks())
            if(t == state.chasing && t == invader) was_chasing = true;
        state.chasing = nullptr;
        if(distance < (was_chasing ? INVADER_KEEP : INVADER_RANGE) * tile() && chase(invader))
        {
            state.chasing = invader;
            return;
        }

        // Guarda a base olhando para o campo inimigo. Um defensor fica no centro do pátio, à
        // mesma distância dos dois flancos (por onde a base é atacada); os outros, duas linhas
        // à frente dele, fora do pátio. O pátio é a passagem para contornar a águia e, em alguns
        // mapas, a saída de quem nasce ao lado da base: com vários guardas lá dentro, eles se
        // trancavam e trancavam quem nascia. Cada defensor escolhe, em ordem, o posto livre mais
        // perto de onde está
        if(own_base != nullptr)
        {
            DuelLayout::Tile center = DuelLayout::yardCenter(team);
            int outside_row = center.row + (team == 0 ? -2 : 2);
            std::vector<DuelLayout::Tile> posts = {center, {outside_row, center.column - 2}, {outside_row, center.column + 2}};
            DuelLayout::Tile spot = center;
            for(Tank* t : allTanks())
            {
                if(t->team != team || t->to_erase || !isAI(t) || roleOf(t) != Bot::ROLE_DEFEND) continue;
                if(posts.empty()) break;
                SDL_Point at = {static_cast<int>(std::lround(t->pos_x / tile())), static_cast<int>(std::lround(t->pos_y / tile()))};
                size_t best = 0;
                for(size_t k = 1; k < posts.size(); k++)
                    if(manhattan({posts[k].column, posts[k].row}, at) < manhattan({posts[best].column, posts[best].row}, at)) best = k;
                if(t == tank)
                {
                    spot = posts[best];
                    break;
                }
                posts.erase(posts.begin() + best);
            }

            // O guarda não estaciona na saída de um ponto de nascimento (as 4 linhas à frente
            // dele): ali ele trancava os aliados que nasciam logo atrás
            auto blocksSpawn = [&](int r, int c) {
                for(SDL_Point s : DuelLayout::spawnOrder(team))
                {
                    int sr = s.y / tile(), sc = s.x / tile();
                    int r0 = (team == 0) ? sr - 4 : sr, r1 = (team == 0) ? sr + 1 : sr + 5;
                    if(r + 1 >= r0 && r <= r1 && c + 1 >= sc && c <= sc + 1) return true;
                }
                return false;
            };
            for(int radius = 0; radius <= 4; radius++)
            {
                std::vector<int> goals;
                for(int r = spot.row - radius; r <= spot.row + radius; r++)
                    for(int c = spot.column - radius; c <= spot.column + radius; c++)
                        if(nav.validCell(r, c) && nav.cellCost(r, c, abilities) < NavGrid::UNREACHABLE && !blocksSpawn(r, c))
                            goals.push_back(nav.cellIndex(r, c));
                int look_row = (team == 0 ? 0 : m_level_rows_count - 2);
                SDL_Rect look = {spot.column * tile(), look_row * tile(), 2 * tile(), 2 * tile()};
                if(tryGoals(goals, &look, NavGrid::UNREACHABLE - 1)) return;
            }
        }
        if(chase(invader)) return;
    }

    // Nada alcançável: vai para o inimigo mais próximo por qualquer caminho, ou fica parado
    Tank* anyone = nearestEnemy(centerOf(tank->dest_rect), &distance);
    if(anyone != nullptr)
    {
        int r = static_cast<int>(std::lround(anyone->pos_y / tile())), c = static_cast<int>(std::lround(anyone->pos_x / tile()));
        std::vector<int> goals;
        for(int dr = -2; dr <= 2; dr++)
            for(int dc = -2; dc <= 2; dc++)
                if(nav.validCell(r + dr, c + dc)) goals.push_back(nav.cellIndex(r + dr, c + dc));
        if(tryGoals(goals, &anyone->dest_rect, NavGrid::UNREACHABLE - 1)) return;
    }
    state.field.clear();
}

TankCommand Duel::steerAI(Tank* tank, AIState& state, Uint32 dt)
{
    TankCommand command;
    command.direction = tank->direction;
    const NavGrid& nav = m_nav[tank->team];

    // Preso (parede que o caminho não previu, outro tanque na frente): anda um pouco
    // numa direção qualquer, atirando, e depois recalcula o caminho
    double moved = std::fabs(tank->pos_x - state.last_x) + std::fabs(tank->pos_y - state.last_y);
    state.last_x = tank->pos_x;
    state.last_y = tank->pos_y;
    bool frozen = tank->testFlag(TSF_FROZEN);
    int kind = dynamic_cast<Bot*>(tank) != nullptr ? 0 : 1;
    if(state.wanted_move && !frozen && moved < 0.01)
    {
        state.still_time += dt;
        m_stats.ai_stuck_time[kind] += dt;
    }
    else
        state.still_time = 0;
    if(state.still_time > STUCK_TIME && state.unstick_time == 0)
    {
        Direction options[3] = {static_cast<Direction>((tank->direction + 1) % 4),
                                static_cast<Direction>((tank->direction + 3) % 4),
                                static_cast<Direction>((tank->direction + 2) % 4)};
        state.unstick_dir = options[rand() % 10 < 8 ? rand() % 2 : 2];
        state.unstick_time = 250 + rand() % 350;
        state.still_time = 0;
        state.replan = 0;
        m_stats.ai_unstick[kind]++;
    }
    if(state.unstick_time > 0)
    {
        state.unstick_time = state.unstick_time > dt ? state.unstick_time - dt : 0;
        command.direction = state.unstick_dir;
        command.move = true;
        command.fire = breakableAhead(tank, state.unstick_dir) || worthFiring(tank, state.unstick_dir, SHOT_RANGE);
        state.wanted_move = true;
        return command;
    }

    // Mira: com um alvo na frente, atira e segura essa direção enquanto ele estiver na linha
    // (no máximo AIM_MAX). Sem isso, a IA virava para o alvo, o caminho a fazia virar de volta
    // no quadro seguinte, e ela virava de novo: várias curvas por segundo, um zigue-zague
    // sem sentido. Inimigo alinhado ao lado, perto: vira para ele (e passa a segurar a mira)
    Direction facing = tank->direction;
    state.aim_cooldown = state.aim_cooldown > dt ? state.aim_cooldown - dt : 0;
    bool holding = false;
    if(state.aim_time < AIM_MAX && worthFiring(tank, facing, SHOT_RANGE))
    {
        command.fire = true;
        holding = true;
        state.aim_time += dt;
    }
    else
    {
        if(state.aim_time > 0) state.aim_cooldown = AIM_COOLDOWN;
        state.aim_time = 0;
        if(state.aim_cooldown == 0)
            for(int k = 1; k < 4; k++)
            {
                Direction d = static_cast<Direction>((facing + k) % 4);
                if(worthFiring(tank, d, TURN_RANGE))
                {
                    command.direction = d;
                    command.fire = true;
                    state.aim_time = dt;
                    state.wanted_move = false;
                    return command;
                }
            }
    }

    // Segue o campo de distâncias: célula atual (posição arredondada) e vizinha mais próxima
    if(state.field.empty())
    {
        state.wanted_move = false;
        return command;
    }
    int row = static_cast<int>(std::lround(tank->pos_y / tile()));
    int column = static_cast<int>(std::lround(tank->pos_x / tile()));
    if(!nav.validCell(row, column))
    {
        state.wanted_move = false;
        return command;
    }
    int here = state.field[nav.cellIndex(row, column)];

    if(here == 0)
    {
        // Chegou à célula (posição arredondada): termina de se alinhar à grade antes de parar.
        // Parado meio tile fora do lugar, o guarda tapava a saída do nascimento ao lado e o
        // atacante no pátio podia ficar com o tiro na quina de pedra em vez da águia
        // Tolerância: a folga da caixa de colisão (2 px) ou um passo do tanque, o que for maior
        // (com tolerância menor que o passo, o tanque rápido passava do ponto e voltava)
        double off_x = tank->pos_x - column * tile(), off_y = tank->pos_y - row * tile();
        double tolerance = std::max(2.0, tank->default_speed * dt);
        // Alinhamento bloqueado (outro tanque encostado): fica onde está por um tempo, em vez
        // de virar para lá e para cá na fronteira entre duas células
        if(state.align_pause > 0) state.align_pause = state.align_pause > dt ? state.align_pause - dt : 0;
        else if(state.still_time > 0) state.align_pause = 600;
        if(state.align_pause == 0 && (std::fabs(off_x) > tolerance || std::fabs(off_y) > tolerance))
        {
            if(std::fabs(off_x) >= std::fabs(off_y)) command.direction = off_x > 0 ? D_LEFT : D_RIGHT;
            else command.direction = off_y > 0 ? D_UP : D_DOWN;
            command.move = true;
            state.wanted_move = true;
            return command;
        }
        // Alinhado: encara o alvo (o tiro sai quando ele estiver na linha)
        if(state.face.w > 0)
        {
            command.direction = directionTo(centerOf(tank->dest_rect), centerOf(state.face));
            if(!command.fire) command.fire = worthFiring(tank, command.direction, SHOT_RANGE);
        }
        state.wanted_move = false;
        return command;
    }

    // Empate entre vizinhas: mantém a direção atual (menos curvas); senão sorteia,
    // para dois tanques no mesmo caminho não andarem sempre colados
    int best = here, best_dir = -1, ties = 0;
    for(int d = 0; d < 4; d++)
    {
        int r = row + DR[d], c = column + DC[d];
        if(!nav.validCell(r, c)) continue;
        int value = state.field[nav.cellIndex(r, c)];
        if(value < best)
        {
            best = value;
            best_dir = d;
            ties = 1;
        }
        else if(value == best && best_dir >= 0 && best_dir != facing &&
                (d == facing || rand() % ++ties == 0))
            best_dir = d;
    }
    if(best_dir < 0)
    {
        state.replan = 0; // sem saída a partir daqui (o mapa mudou): recalcula
        state.wanted_move = false;
        return command;
    }

    // Antes de virar, termina de se alinhar à grade no outro eixo (senão bate na quina)
    Direction wanted = static_cast<Direction>(best_dir);
    bool vertical = (wanted == D_UP || wanted == D_DOWN);
    double offset = vertical ? tank->pos_x - column * tile() : tank->pos_y - row * tile();
    if(std::fabs(offset) >= 4.0)
        wanted = vertical ? (offset > 0 ? D_LEFT : D_RIGHT) : (offset > 0 ? D_UP : D_DOWN);

    // Segurando a mira: só anda se o caminho segue para a frente (na direção do alvo);
    // não vira as costas para quem está na linha de tiro
    if(holding && wanted != facing)
    {
        command.direction = facing;
        state.wanted_move = false;
        return command;
    }

    command.direction = wanted;
    command.move = true;
    // Tijolo no caminho: para e atira até abrir passagem
    if(breakableAhead(tank, wanted))
    {
        command.move = (tank->direction != wanted); // primeiro vira
        command.fire = true;
    }
    else if(wanted != facing && !command.fire)
        command.fire = worthFiring(tank, wanted, SHOT_RANGE);

    state.wanted_move = command.move;
    return command;
}

void Duel::updateAI(Uint32 dt)
{
    buildNavGrids();

    for(Tank* tank : allTanks())
    {
        if(!isAI(tank) || tank->to_erase) continue;
        AIState& state = m_ai[tank];

        TankCommand command;
        command.direction = tank->direction;
        if(tank->testFlag(TSF_LIFE))
        {
            bool is_bot = dynamic_cast<Bot*>(tank) != nullptr;
            m_stats.ai_alive_time[is_bot ? 0 : 1] += dt;
            if(is_bot)
            {
                // Mapa de calor: onde os bots passam o tempo (cantos travados aparecem aqui)
                m_stats.heat_columns = m_level_columns_count;
                m_stats.bot_heat.resize(m_level_rows_count * m_level_columns_count, 0.0);
                SDL_Point c = centerOf(tank->dest_rect);
                int r = c.y / tile(), col = c.x / tile();
                if(r >= 0 && r < m_level_rows_count && col >= 0 && col < m_level_columns_count)
                    m_stats.bot_heat[r * m_level_columns_count + col] += dt;
            }
            if(state.replan <= dt)
            {
                planAI(tank, state);
                state.replan = REPLAN_TIME + rand() % 100;
            }
            else
                state.replan -= dt;
            command = steerAI(tank, state, dt);
        }
        else
            state = AIState(); // morto ou nascendo: começa do zero ao entrar em campo

        // Nunca atira na direção da própria base (o tiro do jogador do computador a destruiria)
        if(command.fire && (firesAtOwnBase(tank, command.direction) || firesAtOwnBase(tank, tank->direction)))
            command.fire = false;

        // Sem meia-volta logo depois de começar a andar numa direção (a não ser preso): perto
        // da fronteira entre duas células, o arredondamento da posição fazia o caminho apontar
        // ora para um lado, ora para o outro, e o tanque ia e voltava sem sair do lugar
        if(command.move && state.unstick_time == 0)
        {
            bool reverse = command.direction == static_cast<Direction>((state.move_dir + 2) % 4);
            if(reverse && state.move_time < MIN_BEFORE_REVERSE && state.still_time == 0)
                command.direction = state.move_dir;
            if(command.direction == state.move_dir) state.move_time += dt;
            else
            {
                state.move_dir = command.direction;
                state.move_time = 0;
            }
        }

        if(Bot* bot = dynamic_cast<Bot*>(tank)) bot->command = command;
        else if(Player* player = dynamic_cast<Player*>(tank))
        {
            // Tempo de reação: o jogador do computador não atira no quadro exato em que
            // o alvo se alinha (o bot já tem uma recarga mais lenta e sorteada)
            if(command.fire && rand() % 3 != 0) command.fire = false;
            player->cpu_command = command;
        }
    }
}
