#include "duel.h"
#include "menu.h"
#include "../engine/engine.h"
#include "../appconfig.h"
#include "../soundmanager.h"
#include "../controllers.h"

#include <algorithm>
#include <climits>
#include <cstdlib>

namespace
{
    // Centro do retângulo de desenho de um objeto
    SDL_Point centerOf(const Object* o)
    {
        return {o->dest_rect.x + o->dest_rect.w / 2, o->dest_rect.y + o->dest_rect.h / 2};
    }

    int manhattan(SDL_Point a, SDL_Point b)
    {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }

    bool intersects(SDL_Rect a, SDL_Rect b)
    {
        SDL_Rect r = intersectRect(&a, &b);
        return r.w > 0 && r.h > 0;
    }

    // Cores dos textos no painel lateral (cinza): versões escuras das cores das equipes
    const SDL_Color PANEL_TEAM_COLOR[2] = {{170, 110, 0, 255}, {0, 120, 0, 255}};
    const SDL_Color BLACK = {0, 0, 0, 255};
    const SDL_Color WHITE = {255, 255, 255, 255};
    const char* TEAM_NAME[2] = {"TEAM A", "TEAM B"};

    // Tempos das telas entre rodadas (ms)
    const Uint32 INTRO_TIME = 1500;
    const Uint32 ROUND_END_TIME = 3000;
    const Uint32 MATCH_END_INPUT_DELAY = 1500;
    const Uint32 MATCH_END_TIMEOUT = 20000;
    const Uint32 LABEL_TIME = 3000;
}

int DuelConfig::humansInTeam(int team) const
{
    int n = 0;
    for(int i = 0; i < humans; i++)
        if(human_team[i] == team) n++;
    return n;
}

// ======================== Construção e rodadas ========================

Duel::Duel(const DuelConfig& config)
    : Game(NoCampaign{}), m_config(config)
{
    // Garante uma configuração válida: 2 a 4 jogadores, ninguém sozinho no mapa
    m_config.humans = std::max(2, std::min(4, m_config.humans));
    for(int i = 0; i < m_config.humans; i++)
        m_config.human_team[i] = (m_config.human_team[i] == 1 ? 1 : 0);
    if(m_config.humansInTeam(0) == 0 || m_config.humansInTeam(1) == 0)
        for(int i = 0; i < m_config.humans; i++) m_config.human_team[i] = i % 2;
    // Só há humanos nas equipes: o tamanho de cada uma é a quantidade de jogadores nela
    for(int t = 0; t < 2; t++)
        m_config.team_size[t] = m_config.humansInTeam(t);

    m_base_b = nullptr;
    m_round = 1;
    m_wins[0] = m_wins[1] = 0;
    m_round_winner = -1;
    for(int i = 0; i < 4; i++) m_kills[i] = 0;

    startRound();
}

Duel::~Duel()
{
    delete m_base_b;
    m_base_b = nullptr;
}

void Duel::clearRound()
{
    clearLevel();
    delete m_base_b;
    m_base_b = nullptr;
}

void Duel::startRound()
{
    clearRound();

    m_phase = PHASE_INTRO;
    m_phase_time = 0;
    m_pause = false;
    m_bonus_time = 0;
    for(int t = 0; t < 2; t++)
    {
        m_fortified[t] = false;
        m_fortify_time[t] = 0;
        m_base_wall[t] = defaultWall(t);
    }

    // Mapa escolhido, ou um sorteado a cada rodada ("Random")
    int map_count = static_cast<int>(AppConfig::duel_maps.size());
    m_map = (m_config.map >= 0 && m_config.map < map_count) ? m_config.map : rand() % map_count;

    // A águia da equipe A (embaixo) é criada pelo loadLevel, como na campanha
    loadLevel(AppConfig::duel_levels_path + AppConfig::duel_maps.at(m_map).first);
    if(m_level_rows_count < 4 || m_level_columns_count < 15)
    {
        // Mapa não encontrado: volta ao menu em vez de rodar sem cenário
        m_finished = true;
        return;
    }

    // Base da equipe B, espelhada no topo do mapa
    m_base_b = new Eagle(12 * AppConfig::tile_rect.w, 0);
    for(int row = 0; row < 2; row++)
        for(int column = 12; column < 14; column++)
        {
            delete m_level.at(row).at(column);
            m_level.at(row).at(column) = nullptr;
        }
    for(int team = 0; team < 2; team++)
        if(defaultWall(team) != ST_BRICK_WALL) setBaseWalls(team, defaultWall(team));

    // Jogadores humanos: sprite amarelo na equipe A, verde na B (como P1 e P2 no original)
    Controllers::setPlayerCount(m_config.humans);
    int slot[2] = {0, 0};
    for(int i = 0; i < m_config.humans; i++)
    {
        int team = m_config.human_team[i];
        Player* p = new Player(i);
        // A cor é da equipe: companheiros têm a mesma cor (e o mesmo sprite)
        p->team = team;
        p->type = static_cast<SpriteType>(ST_PLAYER_1 + team);
        p->setPlayerColor(teamColor(team));
        p->spawn_point = AppConfig::duel_spawn_points.at(team).at(slot[team]++);
        p->lives_count = livesPerTank(team) + 1; // respawn() gasta uma ao entrar no mapa
        p->respawn();
        m_players.push_back(p);
    }

    SoundManager::getInstance().playSound("level_starting");
}

void Duel::endRound(int winner)
{
    if(m_phase != PHASE_PLAY) return;

    m_round_winner = winner;
    m_phase_time = 0;
    if(winner >= 0) m_wins[winner]++;

    if(winner >= 0 && m_wins[winner] >= AppConfig::duel_rounds_to_win)
    {
        m_phase = PHASE_MATCH_END;
        SoundManager::getInstance().playSound("game_over");
    }
    else
        m_phase = PHASE_ROUND_END;
}

// ======================== Regras ========================

std::vector<Eagle*> Duel::bases()
{
    std::vector<Eagle*> v;
    if(m_eagle != nullptr) v.push_back(m_eagle);
    if(m_base_b != nullptr) v.push_back(m_base_b);
    return v;
}

Eagle* Duel::baseOf(int team)
{
    return team == 0 ? m_eagle : m_base_b;
}

void Duel::onBaseHit(Eagle* base, Bullet* bullet)
{
    int owner = (base == m_eagle ? 0 : 1);
    bullet->destroy();
    // Fogo amigo não destrói a própria base
    if(bullet->team == owner || m_phase != PHASE_PLAY) return;

    base->destroy();
    SoundManager::getInstance().playSound("fexplosion");
    endRound(1 - owner);
}

bool Duel::isBaseWall(int team, int row, int column) const
{
    // Equipe A: base nas duas últimas linhas; equipe B: nas duas primeiras
    int base_row = (team == 0 ? m_level_rows_count - 2 : 0);
    int front_row = (team == 0 ? base_row - 1 : base_row + 2);
    int top = std::min(base_row, front_row), bottom = std::max(base_row + 1, front_row);
    if((column == 11 || column == 14) && row >= top && row <= bottom) return true;
    return row == front_row && (column == 12 || column == 13);
}

bool Duel::bulletCanDamage(Bullet* bullet, int row, int column)
{
    // Uma equipe não derruba a muralha da própria base (evita que bots e
    // jogadores abram a defesa sem querer)
    return !(bullet->team >= 0 && isBaseWall(bullet->team, row, column));
}

int Duel::livesPerTank(int team) const
{
    // Equipes menores recebem mais vidas, mas só metade do que igualaria o total:
    // vidas = base * (1 + proporção) / 2, arredondado para cima (1 contra 3 -> 6 contra 3).
    // Junto com a muralha de pedra (defaultWall), foi o ajuste que mais aproximou as
    // vitórias em simulações só com bots; igualar o total de vidas com a muralha de pedra
    // deixava a equipe menor quase invencível.
    int largest = std::max(m_config.team_size[0], m_config.team_size[1]);
    int size = m_config.team_size[team];
    return (AppConfig::duel_tank_lives * (size + largest) + 2 * size - 1) / (2 * size);
}

SpriteType Duel::defaultWall(int team) const
{
    // Se a outra equipe tem o dobro de tanques ou mais, a menor defende a base com pedra:
    // vidas a mais não seguram vários atacantes na base, só a muralha segura.
    // Com diferença menor (3 contra 4, 2 contra 3), a pedra desequilibrava para o outro lado.
    return 2 * m_config.team_size[team] <= m_config.team_size[1 - team] ? ST_STONE_WALL : ST_BRICK_WALL;
}

std::vector<Tank*> Duel::allTanks()
{
    std::vector<Tank*> v(m_players.begin(), m_players.end());
    v.insert(v.end(), m_enemies.begin(), m_enemies.end());
    return v;
}

// Vidas e eliminação contam só os jogadores: o bot de reforço é ajuda temporária,
// não segura a rodada sozinho depois que os jogadores da equipe caíram
int Duel::teamLives(int team)
{
    int lives = 0;
    for(auto player : m_players)
        if(player->team == team && !player->to_erase) lives += player->lives_count;
    return lives;
}

bool Duel::teamAlive(int team)
{
    for(auto player : m_players)
        if(player->team == team && !player->to_erase) return true;
    return false;
}

bool Duel::spawnAlly(Player* player)
{
    int team = player->team;

    // Limite de reforços em campo por equipe
    int allies = 0;
    bool has_defender = false;
    for(auto e : m_enemies)
    {
        Bot* bot = dynamic_cast<Bot*>(e);
        if(bot == nullptr || bot->team != team || bot->to_erase) continue;
        allies++;
        if(bot->role == Bot::ROLE_DEFEND) has_defender = true;
    }
    if(allies >= AppConfig::duel_max_allies) return false;

    // Nasce no primeiro ponto de renascimento livre da equipe
    for(SDL_Point spawn : AppConfig::duel_spawn_points.at(team))
    {
        SDL_Rect area = {spawn.x, spawn.y, 2 * AppConfig::tile_rect.w, 2 * AppConfig::tile_rect.h};
        bool occupied = false;
        for(Tank* t : allTanks())
            if(!t->to_erase && (intersects(t->dest_rect, area) || intersects(t->collision_rect, area))) occupied = true;
        if(occupied) continue;

        // O primeiro reforço guarda a base (o jogador costuma atacar); o segundo ataca.
        // Atacante usa o tiro do tanque D (mira no alvo); defensor, o do tanque A
        Bot::Role role = has_defender ? Bot::ROLE_ATTACK : Bot::ROLE_DEFEND;
        SpriteType type = (role == Bot::ROLE_ATTACK ? ST_TANK_D : ST_TANK_A);
        m_enemies.push_back(new Bot(spawn, type, team, 1, teamColor(team), role));
        return true;
    }
    return false;
}

void Duel::hitTank(Tank* target, Tank* shooter)
{
    target->destroy(); // Player e Bot ignoram o tiro se estiverem com escudo
    if(!target->testFlag(TSF_DESTROYED)) return;

    Player* shooter_player = dynamic_cast<Player*>(shooter);
    if(shooter_player != nullptr) m_kills[shooter_player->playerIndex()]++;
}

void Duel::checkBulletsAgainstTank(Tank* shooter, Tank* target)
{
    if(shooter->team == target->team) return; // sem fogo amigo
    if(target->to_erase || !target->testFlag(TSF_LIFE)) return;

    for(auto bullet : shooter->bullets)
    {
        if(bullet->to_erase || bullet->collide) continue;
        if(!intersects(bullet->collision_rect, target->collision_rect)) continue;

        bullet->destroy();
        hitTank(target, shooter);
        if(!target->testFlag(TSF_LIFE)) return;
    }
}

void Duel::updateBotTargets()
{
    std::vector<Tank*> tanks = allTanks();
    for(auto e : m_enemies)
    {
        Bot* bot = dynamic_cast<Bot*>(e);
        if(bot == nullptr || !bot->testFlag(TSF_LIFE)) continue;

        int enemy_team = 1 - bot->team;
        Eagle* own_base = baseOf(bot->team);
        Eagle* enemy_base = baseOf(enemy_team);
        if(own_base == nullptr || enemy_base == nullptr) continue;

        // Atacante procura inimigos perto de si; defensor, perto da própria base
        SDL_Point reference = (bot->role == Bot::ROLE_DEFEND ? centerOf(own_base) : centerOf(bot));
        Tank* nearest = nullptr;
        int best = INT_MAX;
        for(Tank* t : tanks)
        {
            if(t->team != enemy_team || t->to_erase || !t->testFlag(TSF_LIFE)) continue;
            int d = manhattan(centerOf(t), reference);
            if(d < best) { best = d; nearest = t; }
        }

        if(bot->role == Bot::ROLE_ATTACK)
        {
            // Bot não quebra pedra (não coleta a arma): com a base inimiga de pedra, caça os tanques
            bool base_blocked = (m_base_wall[enemy_team] == ST_STONE_WALL);
            bool chase = nearest != nullptr && (base_blocked || best < 10 * AppConfig::tile_rect.w);
            bot->target_position = chase ? centerOf(nearest) : centerOf(enemy_base);
        }
        else
        {
            // Sem invasores, fica de guarda na frente da própria base
            SDL_Point guard = centerOf(own_base);
            guard.y += (bot->team == 0 ? -6 : 6) * AppConfig::tile_rect.h;
            bot->target_position = (nearest != nullptr && best < 13 * AppConfig::tile_rect.w) ? centerOf(nearest) : guard;
        }
    }
}

// ======================== Bônus ========================

SDL_Color Duel::teamColor(int team)
{
    // Paleta de 4 cores, uma por equipe (amarelo, verde, azul, vermelho: as dos
    // jogadores da campanha). Com duas equipes, o duelo usa as duas primeiras; num
    // modo cada um por si, cada jogador seria uma equipe com a sua cor.
    return Player::getPlayerColor(team);
}

bool Duel::isTeamBonus(SpriteType type)
{
    // Bônus que surgem para uma equipe específica. Para tornar outro poder exclusivo,
    // basta incluí-lo aqui: sorteio da equipe, lado do mapa, cor e coleta já são genéricos.
    return type == ST_BONUS_TANK;
}

int Duel::trailingTeam()
{
    // Equipe bem atrás em vidas (proporcionalmente ao total dela), ou -1
    double ratio[2];
    for(int team = 0; team < 2; team++)
        ratio[team] = static_cast<double>(teamLives(team)) / (m_config.team_size[team] * livesPerTank(team));
    if(ratio[0] + 0.25 <= ratio[1]) return 0;
    if(ratio[1] + 0.25 <= ratio[0]) return 1;
    return -1;
}

std::vector<SDL_Point> Duel::halfSpots(int team) const
{
    // Pontos simétricos dentro da metade do mapa da equipe (A embaixo, B em cima)
    int t = AppConfig::tile_rect.w;
    if(team == 0) return {{4 * t, 16 * t}, {20 * t, 16 * t}, {12 * t, 18 * t}};
    return {{4 * t, 8 * t}, {20 * t, 8 * t}, {12 * t, 6 * t}};
}

int Duel::chooseBonusTeam()
{
    // Sorteia a equipe (50/50, ou a que está atrás em vidas), não o jogador:
    // a equipe menor de um 3 contra 1 recebe tantos bônus exclusivos quanto a maior
    int trailing = trailingTeam();
    int first = trailing >= 0 ? trailing : rand() % 2;
    for(int team : {first, 1 - first})
        if(teamAlive(team)) return team;
    return -1;
}

void Duel::spawnBonus()
{
    // Pesos de cada bônus: os que decidem a rodada sozinhos (granada e arma) são raros
    static const struct { SpriteType type; int weight; } table[] = {
        {ST_BONUS_STAR, 20}, {ST_BONUS_HELMET, 16}, {ST_BONUS_SHOVEL, 14}, {ST_BONUS_CLOCK, 12},
        {ST_BONUS_TANK, 12}, {ST_BONUS_BOAT, 10}, {ST_BONUS_GRENADE, 8}, {ST_BONUS_GUN, 8},
    };
    int total = 0;
    for(auto& entry : table) total += entry.weight;
    int roll = rand() % total;
    SpriteType type = ST_BONUS_STAR;
    for(auto& entry : table)
    {
        if(roll < entry.weight) { type = entry.type; break; }
        roll -= entry.weight;
    }

    // Bônus da equipe: só jogadores daquela cor coletam, e ele surge com mais frequência
    // na metade do adversário (é preciso invadir para buscar) do que na própria
    int owner_team = isTeamBonus(type) ? chooseBonusTeam() : -1;
    if(owner_team >= 0)
    {
        double roll = static_cast<double>(rand()) / RAND_MAX;
        int side = (roll < AppConfig::duel_team_bonus_enemy_side_chance) ? 1 - owner_team : owner_team;
        std::vector<SDL_Point> spots = halfSpots(side);
        SDL_Point p = spots.at(rand() % spots.size());
        Bonus* bonus = new Bonus(p.x, p.y, type);
        bonus->owner_team = owner_team;
        bonus->color = teamColor(owner_team); // o mesmo ícone, tingido com a cor da equipe
        m_bonuses.push_back(bonus);
        return;
    }
    if(isTeamBonus(type)) type = ST_BONUS_STAR; // nenhuma equipe em jogo (não deve ocorrer)

    // Bônus comuns, em pontos simétricos: no meio do mapa (mesma distância das duas
    // bases) ou, se uma equipe estiver bem atrás em vidas, na metade dela
    int t = AppConfig::tile_rect.w;
    int trailing = trailingTeam();
    std::vector<SDL_Point> spots;
    if(trailing >= 0)
    {
        spots = halfSpots(trailing);
        spots.pop_back(); // só os pontos das laterais
    }
    else
        spots = {{12 * t, 12 * t}, {4 * t, 12 * t}, {20 * t, 12 * t}};

    SDL_Point p = spots.at(rand() % spots.size());
    m_bonuses.push_back(new Bonus(p.x, p.y, type));
}

void Duel::applyBonus(Player* player, Bonus* bonus)
{
    SoundManager::getInstance().playSound("bonus");
    int team = player->team;
    int enemy_team = 1 - team;

    switch(bonus->type)
    {
    case ST_BONUS_GRENADE:
        // Destrói os inimigos em campo (escudo, barco e 3 estrelas protegem como num tiro)
        for(Tank* t : allTanks())
            if(t->team == enemy_team && !t->to_erase && t->testFlag(TSF_LIFE))
                hitTank(t, player);
        break;
    case ST_BONUS_HELMET:
        player->setFlag(TSF_SHIELD);
        break;
    case ST_BONUS_CLOCK:
        // Imobiliza a equipe inimiga por pouco tempo; humanos ainda podem girar e atirar
        for(Tank* t : allTanks())
            if(t->team == enemy_team && !t->to_erase) t->freeze(AppConfig::duel_freeze_time);
        break;
    case ST_BONUS_SHOVEL:
        m_fortified[team] = true;
        m_fortify_time[team] = 0;
        m_base_wall[team] = ST_STONE_WALL;
        setBaseWalls(team, ST_STONE_WALL);
        break;
    case ST_BONUS_TANK:
        // Reforço: um bot aliado da cor da equipe. Sem vaga (limite atingido ou
        // pontos de renascimento ocupados), vira uma vida extra
        if(!spawnAlly(player)) player->addLife();
        break;
    case ST_BONUS_STAR:
        player->changeStarCountBy(1);
        break;
    case ST_BONUS_GUN:
        player->changeStarCountBy(3);
        break;
    case ST_BONUS_BOAT:
        player->setFlag(TSF_BOAT);
        break;
    default:
        break;
    }
    bonus->to_erase = true;
}

void Duel::setBaseWalls(int team, SpriteType wall)
{
    std::vector<Tank*> tanks = allTanks();
    for(int row = 0; row < m_level_rows_count; row++)
        for(int column = 11; column <= 14; column++)
        {
            if(!isBaseWall(team, row, column)) continue;

            // Não cria parede em cima de um tanque (ele ficaria preso)
            SDL_Rect tile = {column * AppConfig::tile_rect.w, row * AppConfig::tile_rect.h, AppConfig::tile_rect.w, AppConfig::tile_rect.h};
            bool occupied = false;
            for(Tank* t : tanks)
                if(!t->to_erase && intersects(t->collision_rect, tile)) occupied = true;
            if(occupied) continue;

            delete m_level.at(row).at(column);
            if(wall == ST_STONE_WALL)
                m_level.at(row).at(column) = new Object(tile.x, tile.y, ST_STONE_WALL);
            else
                m_level.at(row).at(column) = new Brick(tile.x, tile.y);
        }
}

void Duel::updateFortify(Uint32 dt)
{
    for(int team = 0; team < 2; team++)
    {
        if(!m_fortified[team]) continue;
        m_fortify_time[team] += dt;

        SpriteType wanted = ST_STONE_WALL;
        if(m_fortify_time[team] > AppConfig::protect_eagle_time)
        {
            m_fortified[team] = false;
            wanted = defaultWall(team);
        }
        // Pisca no último quarto do tempo, avisando que a proteção vai acabar
        else if(m_fortify_time[team] > AppConfig::protect_eagle_time / 4 * 3 &&
                m_fortify_time[team] / AppConfig::bonus_blink_time % 2)
            wanted = defaultWall(team);

        if(wanted != m_base_wall[team])
        {
            m_base_wall[team] = wanted;
            setBaseWalls(team, wanted);
        }
    }
}

// ======================== Laço do jogo ========================

void Duel::update(Uint32 dt)
{
    if(dt > 40) return;
    m_phase_time += dt;

    switch(m_phase)
    {
    case PHASE_INTRO:
        if(m_phase_time > INTRO_TIME)
        {
            m_phase = PHASE_PLAY;
            m_phase_time = 0;
        }
        return;
    case PHASE_ROUND_END:
    case PHASE_MATCH_END:
        // Só anima as bases (explosão da águia); o resto fica congelado
        for(Eagle* base : bases()) base->update(dt);
        if(m_phase == PHASE_ROUND_END && m_phase_time > ROUND_END_TIME)
        {
            m_round++;
            startRound();
        }
        else if(m_phase == PHASE_MATCH_END && m_phase_time > MATCH_END_TIMEOUT)
            m_finished = true;
        return;
    case PHASE_PLAY:
        break;
    }

    if(m_pause) return;

    std::vector<Tank*> tanks = allTanks();

    // Tanques entre si
    for(size_t i = 0; i < tanks.size(); i++)
        for(size_t j = i + 1; j < tanks.size(); j++)
            checkCollisionTwoTanks(tanks[i], tanks[j], dt);

    // Projéteis contra o cenário e as bases
    for(Tank* t : tanks)
        for(auto bullet : t->bullets)
        {
            checkCollisionBulletWithLevel(bullet);
            checkCollisionBulletWithBush(bullet);
        }

    // Projéteis contra tanques e projéteis de outra equipe
    for(Tank* shooter : tanks)
        for(Tank* target : tanks)
            if(shooter != target) checkBulletsAgainstTank(shooter, target);
    for(size_t i = 0; i < tanks.size(); i++)
        for(size_t j = i + 1; j < tanks.size(); j++)
            if(tanks[i]->team != tanks[j]->team)
                for(auto b1 : tanks[i]->bullets)
                    for(auto b2 : tanks[j]->bullets)
                        checkCollisionTwoBullets(b1, b2);

    // Bônus: só jogadores humanos coletam; o da equipe, só jogadores daquela cor
    // (os adversários passam por cima)
    for(auto player : m_players)
        for(auto bonus : m_bonuses)
            if(!player->to_erase && !bonus->to_erase && player->testFlag(TSF_LIFE) &&
               (bonus->owner_team < 0 || bonus->owner_team == player->team) &&
               intersects(player->collision_rect, bonus->collision_rect))
                applyBonus(player, bonus);

    // Bônus de uma equipe sem jogadores em campo não serve para ninguém
    for(auto bonus : m_bonuses)
        if(bonus->owner_team >= 0 && !teamAlive(bonus->owner_team)) bonus->to_erase = true;

    // Tanques contra o cenário
    for(Tank* t : tanks) checkCollisionTankWithLevel(t, dt);
    for(auto player : m_players) tryCornerSlide(player, dt);

    updateBotTargets();

    for(auto enemy : m_enemies) enemy->update(dt);
    for(auto player : m_players) player->update(dt);
    for(auto bonus : m_bonuses) bonus->update(dt);
    for(Eagle* base : bases()) base->update(dt);
    for(auto row : m_level)
        for(auto item : row)
            if(item != nullptr) item->update(dt);
    for(auto bush : m_bushes) bush->update(dt);

    auto erase = [](auto& v) {
        v.erase(std::remove_if(v.begin(), v.end(), [](auto* o){ if(o->to_erase) { delete o; return true; } return false; }), v.end());
    };
    erase(m_enemies);
    erase(m_players);
    erase(m_bonuses);
    erase(m_bushes);

    updateFortify(dt);

    // O próximo bônus só começa a contar quando o mapa fica sem nenhum
    if(!m_bonuses.empty()) m_bonus_time = 0;
    else if((m_bonus_time += dt) > AppConfig::duel_bonus_interval)
    {
        m_bonus_time = 0;
        spawnBonus();
    }

    // Equipe sem tanques perde a rodada (as duas ao mesmo tempo: empate, rodada repetida)
    if(m_phase == PHASE_PLAY)
    {
        bool alive_a = teamAlive(0), alive_b = teamAlive(1);
        if(!alive_a || !alive_b)
            endRound(alive_a ? 0 : (alive_b ? 1 : -1));
    }
}

void Duel::draw()
{
    Engine& engine = Engine::getEngine();
    Renderer* renderer = engine.getRenderer();
    renderer->clear();

    renderer->drawRect(&AppConfig::map_rect, {0, 0, 0, 255}, true);
    for(auto row : m_level)
        for(auto item : row)
            if(item != nullptr) item->draw();
    for(auto player : m_players) player->draw();
    for(auto enemy : m_enemies) enemy->draw();
    for(auto bush : m_bushes) bush->draw();
    for(auto bonus : m_bonuses) bonus->draw();
    for(Eagle* base : bases()) base->draw();

    // Bônus da equipe: letra da equipe acima do ícone, na cor dela (piscando junto com o bônus)
    for(auto bonus : m_bonuses)
    {
        if(bonus->owner_team < 0 || !bonus->visible() || bonus->to_erase) continue;
        SDL_Point p = {bonus->dest_rect.x + 12, bonus->dest_rect.y - 11};
        renderer->drawText(&p, bonus->owner_team == 0 ? "A" : "B", bonus->color, 3);
    }

    // Número de cada jogador acima do tanque: no começo da rodada e ao renascer
    bool round_start = (m_phase == PHASE_INTRO || (m_phase == PHASE_PLAY && m_phase_time < LABEL_TIME));
    for(auto player : m_players)
    {
        if(!(round_start || player->testFlag(TSF_CREATE)) || player->testFlag(TSF_DESTROYED)) continue;
        SDL_Point p = {player->dest_rect.x + 6, player->dest_rect.y - 11};
        if(p.y < 0) p.y = player->dest_rect.y + player->dest_rect.h + 1;
        renderer->drawText(&p, "P" + Engine::intToString(player->playerIndex() + 1), player->color, 3);
    }

    //=========== Painel lateral: equipe B em cima, rodada no meio, equipe A embaixo ===========
    SDL_Rect flag_src = engine.getSpriteConfig()->getSpriteData(ST_FLAG)->rect;
    for(int team = 0; team < 2; team++)
    {
        int y = (team == 1 ? 8 : 330);
        int x = AppConfig::status_rect.x + 6;
        SDL_Point p = {x, y};
        renderer->drawText(&p, team == 0 ? "A" : "B", PANEL_TEAM_COLOR[team], 2);

        // Uma bandeira por rodada vencida
        for(int w = 0; w < m_wins[team]; w++)
        {
            SDL_Rect dst = {x + 14 + w * 13, y + 1, 12, 12};
            renderer->drawObject(&flag_src, &dst);
        }

        // Jogadores da equipe e as vidas de cada um ("P1 3")
        int row = 0;
        for(auto player : m_players)
        {
            if(player->team != team || player->to_erase) continue;
            p = {x, y + 26 + row * 16};
            renderer->drawText(&p, "P" + Engine::intToString(player->playerIndex() + 1), PANEL_TEAM_COLOR[team], 3);
            p = {x + 26, y + 26 + row * 16};
            renderer->drawText(&p, Engine::intToString(player->lives_count), BLACK, 3);
            row++;
        }

    }
    SDL_Point p = {AppConfig::status_rect.x + 6, 190};
    renderer->drawText(&p, "RND", BLACK, 3);
    p = {AppConfig::status_rect.x + 10, 204};
    renderer->drawText(&p, Engine::intToString(m_round), BLACK, 2);

    //=========== Mensagens no centro ===========
    if(m_phase == PHASE_INTRO)
    {
        renderer->drawText(nullptr, "ROUND " + Engine::intToString(m_round), WHITE, 1);
        p = {-1, 160};
        renderer->drawText(&p, AppConfig::duel_maps.at(m_map).second, WHITE, 2);
        p = {-1, 240};
        renderer->drawText(&p, "FIRST TO " + Engine::intToString(AppConfig::duel_rounds_to_win) + " WINS", WHITE, 2);
    }
    else if(m_phase == PHASE_PLAY && m_pause)
        renderer->drawText(nullptr, std::string("PAUSE"), {200, 0, 0, 255}, 1);
    else if(m_phase == PHASE_ROUND_END)
    {
        if(m_round_winner < 0)
            renderer->drawText(nullptr, std::string("DRAW"), WHITE, 1);
        else
        {
            p = {-1, 180};
            renderer->drawText(&p, TEAM_NAME[m_round_winner], teamColor(m_round_winner), 1);
            p = {-1, 220};
            renderer->drawText(&p, std::string("WINS THE ROUND"), WHITE, 2);
        }
    }
    else if(m_phase == PHASE_MATCH_END)
    {
        SDL_Rect box = {32, 104, AppConfig::map_rect.w - 64, 208};
        renderer->drawRect(&box, {0, 0, 0, 255}, true);
        renderer->drawRect(&box, teamColor(m_round_winner), false);

        p = {-1, 120};
        renderer->drawText(&p, TEAM_NAME[m_round_winner], teamColor(m_round_winner), 1);
        p = {-1, 156};
        renderer->drawText(&p, std::string("WINS THE DUEL"), WHITE, 2);
        p = {-1, 180};
        renderer->drawText(&p, Engine::intToString(m_wins[0]) + " - " + Engine::intToString(m_wins[1]), WHITE, 2);

        for(int i = 0; i < m_config.humans; i++)
        {
            int team = m_config.human_team[i];
            p = {-1, 212 + i * 16};
            renderer->drawText(&p, "P" + Engine::intToString(i + 1) + (team == 0 ? "  A  " : "  B  ") +
                               Engine::intToString(m_kills[i]) + " KILLS", teamColor(team), 3);
        }
        if(m_phase_time > MATCH_END_INPUT_DELAY)
        {
            p = {-1, 290};
            renderer->drawText(&p, std::string("PRESS FIRE"), WHITE, 3);
        }
    }

    renderer->flush();
}

void Duel::eventProcess(SDL_Event* ev)
{
    bool match_over = (m_phase == PHASE_MATCH_END && m_phase_time > MATCH_END_INPUT_DELAY);

    if(ev->type == SDL_KEYDOWN)
    {
        SDL_Scancode key = ev->key.keysym.scancode;
        if(ev->key.keysym.sym == SDLK_ESCAPE)
            m_finished = true;
        else if(match_over)
        {
            // Qualquer tecla de tiro ou Enter volta ao menu
            bool fire = (ev->key.keysym.sym == SDLK_RETURN);
            for(auto& keys : AppConfig::keyboard_layouts)
                if(key == keys.fire) fire = true;
            if(fire) m_finished = true;
        }
        else if(ev->key.keysym.sym == SDLK_RETURN && m_phase == PHASE_PLAY)
            m_pause = !m_pause;
    }
    else if(ev->type == SDL_CONTROLLERBUTTONDOWN)
    {
        if(ev->cbutton.button == SDL_CONTROLLER_BUTTON_BACK)
            m_finished = true;
        else if(match_over && (ev->cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                               ev->cbutton.button == SDL_CONTROLLER_BUTTON_START))
            m_finished = true;
        else if(ev->cbutton.button == SDL_CONTROLLER_BUTTON_START && m_phase == PHASE_PLAY)
            m_pause = !m_pause;
    }
}

AppState* Duel::nextState()
{
    // Volta para a escolha de mapa, com o último selecionado: revanche com um botão
    return new Menu(Menu::SCREEN_DUEL_MAP);
}
