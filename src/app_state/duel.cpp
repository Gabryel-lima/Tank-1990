#include "duel.h"
#include "duel_layout.h"
#include "message_box.h"
#include "powers.h"
#include "menu.h"
#include "../engine/engine.h"
#include "../appconfig.h"
#include "../soundmanager.h"
#include "../controllers.h"

#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{
    bool intersects(SDL_Rect a, SDL_Rect b)
    {
        SDL_Rect r = intersectRect(&a, &b);
        return r.w > 0 && r.h > 0;
    }

    const SDL_Color BLACK = {0, 0, 0, 255};
    const SDL_Color WHITE = {255, 255, 255, 255};
    const SDL_Color LIGHT_GRAY = {200, 200, 200, 255};
    const SDL_Color GRAY = {150, 150, 150, 255};

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
    m_ai.clear();
    for(int t = 0; t < 2; t++)
    {
        m_fortified[t] = false;
        m_fortify_time[t] = 0;
        m_base_wall[t] = defaultWall(t);
    }

    // Mapa escolhido, ou um sorteado a cada rodada ("Random")
    int map_count = static_cast<int>(AppConfig::duel_maps.size());
    m_map = (m_config.map >= 0 && m_config.map < map_count) ? m_config.map : rand() % map_count;

    // Mapa fora da geometria do duelo (arquivo ausente, tamanho errado...): volta ao menu
    // em vez de rodar uma partida quebrada. Normalmente já foi recusado em DuelLayout::loadMapList
    std::string path = AppConfig::duel_levels_path + AppConfig::duel_maps.at(m_map).first;
    std::vector<std::string> problems = DuelLayout::validate(DuelLayout::readMap(path));
    if(!problems.empty())
    {
        std::cerr << "Mapa de duelo " << path << " inválido: " << problems.front() << "\n";
        m_finished = true;
        return;
    }

    // A águia da equipe A (embaixo) é criada pelo loadLevel, como na campanha
    loadLevel(path);

    // Base da equipe B, espelhada no topo do mapa
    int t = AppConfig::tile_rect.w;
    m_base_b = new Eagle(DuelLayout::BASE_COLUMN * t, DuelLayout::baseRow(1) * t);
    for(int row = DuelLayout::baseRow(1); row < DuelLayout::baseRow(1) + 2; row++)
        for(int column = DuelLayout::BASE_COLUMN; column < DuelLayout::BASE_COLUMN + 2; column++)
        {
            delete m_level.at(row).at(column);
            m_level.at(row).at(column) = nullptr;
        }
    // O jogo monta a muralha das duas bases (tijolo, ou pedra para a equipe menor):
    // o arquivo do mapa não precisa trazê-la
    for(int team = 0; team < 2; team++) setBaseWalls(team, defaultWall(team));
    computeStoneOwners();

    // Jogadores humanos: sprite amarelo na equipe A, verde na B (como P1 e P2 no original)
    Controllers::setPlayerCount(m_config.humans);
    m_spawns = assignSpawns();
    for(int i = 0; i < m_config.humans; i++)
        m_players.push_back(createPlayer(i, livesPerTank(m_config.human_team[i])));

    SoundManager::getInstance().playSound("level_starting");
}

Player* Duel::createPlayer(int index, int lives)
{
    int team = m_config.human_team[index];
    Player* p = new Player(index);
    // A cor é da equipe: companheiros têm a mesma cor (e o mesmo sprite)
    p->team = team;
    p->cpu = m_config.cpu[index];
    p->star_armor = AppConfig::duel_star_armor;
    p->type = static_cast<SpriteType>(ST_PLAYER_1 + team);
    p->setPlayerColor(teamColor(team));
    p->spawn_point = m_spawns.at(index);
    p->lives_count = lives + 1; // respawn() gasta uma ao entrar no mapa
    p->setReloadTime(static_cast<Uint32>(1000.0 / AppConfig::duel_max_shots_per_second));
    p->respawn();
    return p;
}

void Duel::endRound(int winner)
{
    if(m_phase != PHASE_PLAY) return;

    bool by_base = false;
    for(Eagle* base : bases())
        if(base->type != ST_EAGLE) by_base = true;
    m_stats.round_winner.push_back(winner);
    m_stats.round_by_base.push_back(by_base);
    m_stats.round_time.push_back(m_phase_time);
    m_stats.round_map.push_back(m_map);

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
    if(m_phase != PHASE_PLAY) return;
    // Fogo amigo: o tiro de um jogador destrói a própria base (como no original), e a
    // rodada vai para o adversário. O do bot de reforço não (ele não erra de propósito)
    if(bullet->team == owner && !bullet->from_player) return;

    base->destroy();
    SoundManager::getInstance().playSound("fexplosion");
    endRound(1 - owner);
}

bool Duel::isBaseWall(int team, int row, int column) const
{
    return DuelLayout::isBaseWall(team, row, column);
}

bool Duel::isInBaseZone(int row, int column) const
{
    // Retângulo em volta de cada águia (DuelLayout): cobre a muralha e as defesas do mapa
    // logo à frente dela, como a pedra do Fortress e do River
    return DuelLayout::inBaseZone(row, column);
}

void Duel::computeStoneOwners()
{
    std::vector<std::string> grid(m_level_rows_count, std::string(m_level_columns_count, '.'));
    for(int r = 0; r < m_level_rows_count; r++)
        for(int c = 0; c < m_level_columns_count; c++)
        {
            Object* o = m_level.at(r).at(c);
            if(o != nullptr && o->type == ST_STONE_WALL) grid[r][c] = '@';
        }
    m_stone_owner = DuelLayout::stoneOwners(grid);
}

int Duel::stoneOwner(int row, int column) const
{
    for(int team = 0; team < 2; team++)
        if(DuelLayout::isBaseWall(team, row, column)) return team;
    if(row < 0 || row >= static_cast<int>(m_stone_owner.size()) || column < 0 ||
       column >= static_cast<int>(m_stone_owner[row].size())) return -1;
    return m_stone_owner[row][column];
}

bool Duel::powerAppliesAt(Bullet* bullet, int row, int column)
{
    // Perto das bases, o canhão (3 estrelas) não vale: o projétil age como um comum,
    // desgasta tijolo e para na pedra. Assim a pedra que protege a base (do mapa ou da pá)
    // não cai de longe e a base não cai em menos de um segundo. Longe das bases, o tanque
    // potente continua destruindo pedra do cenário
    if(!isInBaseZone(row, column)) return true;

    // Pedra comum dentro da zona (parede que cruza a borda): o canhão quebra, como fora
    Object* o = m_level.at(row).at(column);
    bool stone = (o != nullptr && o->type == ST_STONE_WALL);
    if(stone && stoneOwner(row, column) < 0) return true;

    // Tiro demolidor: derruba a pedra da base INIMIGA, mas só disparado de dentro da zona
    // dela. Abre um terceiro ângulo (a frente) para quem chega perto, sem tiro de longe,
    // de uma base para a outra
    int zone = stone ? stoneOwner(row, column) : DuelLayout::zoneTeam(row, column);
    int from = DuelLayout::zoneTeam(bullet->origin.y / AppConfig::tile_rect.h, bullet->origin.x / AppConfig::tile_rect.w);
    return bullet->demolisher && zone != bullet->team && from == zone;
}

bool Duel::breaksBlock(Bullet* bullet, int row, int column)
{
    if(Game::breaksBlock(bullet, row, column)) return true;
    Object* o = m_level.at(row).at(column);
    return bullet->from_player && o != nullptr && o->type == ST_STONE_WALL &&
           bullet->team >= 0 && stoneOwner(row, column) == bullet->team;
}

bool Duel::bulletCanDamage(Bullet* bullet, int row, int column)
{
    // O jogador derruba os tijolos da própria muralha (abrir uma passagem ou um ângulo de
    // tiro faz parte da estratégia); o bot de reforço não, para não abrir a defesa sem querer
    return !(bullet->team >= 0 && !bullet->from_player && isBaseWall(bullet->team, row, column));
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
    // Se a outra equipe tem AppConfig::duel_stone_wall_ratio vezes mais tanques, a menor
    // defende a base com pedra: vidas a mais não seguram vários atacantes na base, só a
    // muralha segura. Com diferença menor, a pedra desequilibra para o outro lado.
    return AppConfig::duel_stone_wall_ratio * m_config.team_size[team] <= m_config.team_size[1 - team] ? ST_STONE_WALL : ST_BRICK_WALL;
}

std::vector<Tank*> Duel::allTanks()
{
    std::vector<Tank*> v(m_players.begin(), m_players.end());
    v.insert(v.end(), m_enemies.begin(), m_enemies.end());
    v.insert(v.end(), m_turrets.begin(), m_turrets.end());
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

std::vector<SDL_Point> Duel::spawnOrder(int team) const
{
    return DuelLayout::spawnOrder(team);
}

std::vector<SDL_Point> Duel::assignSpawns()
{
    // As equipes escolhem intercaladas (1º de A, 1º de B, 2º de A...), cada uma na sua
    // ordem de preferência, pulando colunas já tomadas. Com até 4 jogadores e 4 colunas,
    // ninguém divide coluna (nem com o adversário do outro lado, nem com o companheiro):
    // como o tiro só anda em linha reta, ninguém nasce na mira de ninguém.
    std::vector<SDL_Point> result(m_config.humans, SDL_Point{0, 0});
    std::vector<int> members[2];
    for(int i = 0; i < m_config.humans; i++) members[m_config.human_team[i]].push_back(i);

    m_player_columns.clear();
    for(size_t k = 0; k < 4; k++)
        for(int team = 0; team < 2; team++)
        {
            if(k >= members[team].size()) continue;
            for(SDL_Point spawn : spawnOrder(team))
            {
                if(std::find(m_player_columns.begin(), m_player_columns.end(), spawn.x) != m_player_columns.end()) continue;
                result[members[team][k]] = spawn;
                m_player_columns.push_back(spawn.x);
                break;
            }
        }
    return result;
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

    // Nasce no primeiro ponto livre da equipe, preferindo colunas que nenhum jogador usa
    // (fora da linha de tiro de quem nasce do outro lado); sem alternativa, usa as demais
    std::vector<SDL_Point> order;
    for(int pass = 0; pass < 2; pass++)
        for(SDL_Point spawn : spawnOrder(team))
        {
            bool column_used = std::find(m_player_columns.begin(), m_player_columns.end(), spawn.x) != m_player_columns.end();
            if(column_used == (pass == 1)) order.push_back(spawn);
        }
    for(SDL_Point spawn : order)
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

    Player* shooter_player = shooter != nullptr ? dynamic_cast<Player*>(shooter) : nullptr;
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

// ======================== Bônus ========================

SDL_Color Duel::stoneTint(int team)
{
    // Meio caminho entre a cor da equipe e o branco: a pedra continua com cara de pedra
    // (o verde puro a confundia com o arbusto)
    SDL_Color c = teamColor(team);
    return {static_cast<Uint8>((c.r + 255) / 2), static_cast<Uint8>((c.g + 255) / 2), static_cast<Uint8>((c.b + 255) / 2), 255};
}

SDL_Color Duel::teamColor(int team)
{
    // Paleta de 4 cores, uma por equipe (amarelo, verde, azul, vermelho: as dos
    // jogadores da campanha). Com duas equipes, o duelo usa as duas primeiras; num
    // modo cada um por si, cada jogador seria uma equipe com a sua cor.
    return Player::getPlayerColor(team);
}

double Duel::teamLead(int team)
{
    // Vidas restantes de cada equipe, proporcionais ao total com que ela começou
    double ratio[2];
    for(int t = 0; t < 2; t++)
        ratio[t] = static_cast<double>(teamLives(t)) / (m_config.team_size[t] * livesPerTank(t));
    return ratio[team] - ratio[1 - team];
}

int Duel::trailingTeam()
{
    // Equipe bem atrás em vidas (proporcionalmente ao total dela), ou -1
    double lead = teamLead(0);
    if(lead <= -0.25) return 0;
    if(lead >= 0.25) return 1;
    return -1;
}

std::vector<SDL_Point> Duel::halfSpots(int team) const
{
    return DuelLayout::halfBonusSpots(team);
}

int Duel::chooseBonusTeam()
{
    // Sorteia a equipe (50/50, ou a que está atrás em vidas), não o jogador:
    // a equipe menor de um 3 contra 1 recebe tantos bônus coloridos quanto a maior
    int trailing = trailingTeam();
    int first = trailing >= 0 ? trailing : rand() % 2;
    for(int team : {first, 1 - first})
        if(teamAlive(team)) return team;
    return -1;
}

void Duel::spawnBonus()
{
    // Pesos de cada bônus em Powers::duelTable: os que decidem a rodada sozinhos são raros
    SpriteType type = Powers::draw(Powers::duelTable());

    // Bônus da equipe (colorido): só jogadores daquela cor coletam, e ele surge com mais
    // frequência na metade do adversário (é preciso invadir para buscar) do que na própria
    double chance = static_cast<double>(rand()) / RAND_MAX;
    int owner_team = chance < AppConfig::duel_neutral_bonus_chance ? -1 : chooseBonusTeam();
    if(owner_team >= 0)
    {
        double side_roll = static_cast<double>(rand()) / RAND_MAX;
        int side = (side_roll < AppConfig::duel_team_bonus_enemy_side_chance) ? 1 - owner_team : owner_team;
        std::vector<SDL_Point> spots = openSpots(halfSpots(side));
        if(spots.empty()) return; // todos cobertos: tenta de novo no próximo intervalo
        SDL_Point p = spots.at(rand() % spots.size());
        Bonus* bonus = new Bonus(p.x, p.y, type);
        bonus->owner_team = owner_team;
        bonus->color = teamColor(owner_team); // o mesmo ícone, tingido com a cor da equipe
        m_bonuses.push_back(bonus);
        return;
    }

    // Bônus cinza (o ícone original, sem tinta): qualquer jogador coleta. Surge em pontos
    // simétricos no meio do mapa (mesma distância das duas bases) ou, se uma equipe
    // estiver bem atrás em vidas, na metade dela
    int trailing = trailingTeam();
    std::vector<SDL_Point> spots;
    if(trailing >= 0)
    {
        spots = halfSpots(trailing);
        spots.pop_back(); // só os pontos das laterais
    }
    else
        spots = DuelLayout::midBonusSpots();
    spots = openSpots(spots);
    if(spots.empty()) return;

    SDL_Point p = spots.at(rand() % spots.size());
    m_bonuses.push_back(new Bonus(p.x, p.y, type));
}

std::vector<SDL_Point> Duel::openSpots(const std::vector<SDL_Point>& spots) const
{
    // O mapa pode cobrir um ponto de bônus (o validador só avisa): o bônus não surge dentro
    // de um bloco; se o bloco cair, o ponto volta a valer
    std::vector<SDL_Point> open;
    int t = AppConfig::tile_rect.w;
    for(SDL_Point p : spots)
    {
        bool free = true;
        for(int r = p.y / t; r < p.y / t + 2; r++)
            for(int c = p.x / t; c < p.x / t + 2; c++)
            {
                Object* o = (r >= 0 && c >= 0 && r < m_level_rows_count && c < m_level_columns_count) ? m_level.at(r).at(c) : nullptr;
                if(o != nullptr && o->type != ST_ICE) free = false;
            }
        if(free) open.push_back(p);
    }
    return open;
}

void Duel::applyBonus(Player* player, Bonus* bonus)
{
    SoundManager::getInstance().playSound("bonus");
    int team = player->team;
    int enemy_team = 1 - team;
    int middle = AppConfig::map_rect.h / 2, center_y = bonus->dest_rect.y + bonus->dest_rect.h / 2;
    int half = center_y == middle ? -1 : (center_y > middle ? 0 : 1); // A embaixo, B em cima
    int side = half < 0 ? -1 : (half == team ? 0 : 1);
    m_stats.pickups.push_back({m_round, team, bonus->type, bonus->owner_team < 0, side, teamLead(team)});
    bonus->to_erase = true;

    // Poder de lugar e momento: fica guardado até o jogador usar (botão de poder)
    if(Powers::storable(bonus->type))
    {
        player->held_power = bonus->type;
        return;
    }

    switch(bonus->type)
    {
    case ST_BONUS_GRENADE:
        // Explode os inimigos em campo. Diferente de um tiro, escudo (o do capacete e os
        // 5 s depois de renascer) e barco não seguram a explosão: antes seguravam, e a
        // granada pega logo depois de uma morte quase nunca matava ninguém
        for(Tank* t : allTanks())
            if(t->team == enemy_team && !t->to_erase && t->testFlag(TSF_LIFE))
            {
                t->clearFlag(TSF_SHIELD);
                t->clearFlag(TSF_BOAT);
                hitTank(t, player);
            }
        break;
    case ST_BONUS_HELMET:
        player->shield(AppConfig::duel_helmet_time);
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
        // Segundo estágio: o canhão pego por quem já evoluiu o tanque (estrelas) vira tiro
        // demolidor
        {
            bool evolved = player->stars() >= AppConfig::duel_demolisher_stars;
            player->changeStarCountBy(3);
            if(evolved) player->setDemolisher(true);
        }
        break;
    case ST_BONUS_BOAT:
        player->setFlag(TSF_BOAT);
        break;
    case ST_BONUS_REVIVE:
        revive(player);
        break;
    case ST_BONUS_REPAIR:
        // A muralha volta inteira, no material atual (pedra se a pá estiver valendo)
        setBaseWalls(team, m_base_wall[team]);
        break;
    case ST_BONUS_TEAM_SHIELD:
        for(auto p : m_players)
            if(p->team == team && !p->to_erase) p->shield(AppConfig::duel_helmet_time);
        break;
    default:
        break;
    }
}

void Duel::revive(Player* player)
{
    int team = player->team;
    // Um companheiro que já caiu volta com uma vida
    for(int i = 0; i < m_config.humans; i++)
    {
        if(m_config.human_team[i] != team) continue;
        bool present = false;
        for(auto p : m_players)
            if(p->playerIndex() == i) present = true;
        if(present) continue;
        m_players.push_back(createPlayer(i, 1));
        SoundManager::getInstance().playSound("life");
        return;
    }
    // Ninguém caiu: vida extra para quem da equipe tem menos
    Player* weakest = player;
    for(auto p : m_players)
        if(p->team == team && !p->to_erase && p->lives_count < weakest->lives_count) weakest = p;
    weakest->addLife();
}

bool Duel::usePower(Player* player)
{
    switch(player->held_power)
    {
    case ST_BONUS_MINE:
        placeMine(player);
        return true;
    case ST_BONUS_BARRICADE:
        return placeBarricade(player);
    case ST_BONUS_TURRET:
        return placeTurret(player);
    case ST_BONUS_TURBO:
        player->boost(AppConfig::power_turbo_time);
        SoundManager::getInstance().playSound("bonus");
        return true;
    case ST_BONUS_RECALL:
    {
        // O próprio ponto de nascimento, ao lado da base; ocupado, outro da equipe
        std::vector<SDL_Point> points = {player->spawn_point};
        for(SDL_Point p : spawnOrder(player->team)) points.push_back(p);
        return recall(player, points);
    }
    default:
        return false;
    }
}

bool Duel::reservedTile(int row, int column)
{
    // Nada de barricada ou torreta em cima de um ponto de nascimento (prenderia quem nasce)
    for(int team = 0; team < 2; team++)
        for(SDL_Point s : spawnOrder(team))
        {
            int r = s.y / AppConfig::tile_rect.h, c = s.x / AppConfig::tile_rect.w;
            if(row >= r && row < r + 2 && column >= c && column < c + 2) return true;
        }
    return false;
}

void Duel::updatePowers(Uint32 dt)
{
    // Botão de poder: usa o poder guardado (se não couber, por exemplo a barricada sem
    // espaço na frente, o poder continua guardado)
    for(auto player : m_players)
        if(player->takePowerPress() && player->held_power != ST_NONE && player->testFlag(TSF_LIFE) && usePower(player))
            player->held_power = ST_NONE;

    // Torretas: atiram em quem estiver na linha, nunca na direção da própria base
    for(auto turret : m_turrets)
        turret->think([&](Direction d) {
            return worthFiring(turret, d, AppConfig::power_turret_range) && !firesAtOwnBase(turret, d);
        });

    // Minas: o tanque da outra equipe que passar por cima leva o "tiro" da mina (escudo e
    // barco seguram, como num tiro: assim uma mina no ponto de nascimento não mata quem nasce)
    std::vector<Tank*> tanks = allTanks();
    auto overlap = [](SDL_Rect a, SDL_Rect b, int min) {
        SDL_Rect r = intersectRect(&a, &b);
        return r.w >= min && r.h >= min;
    };
    for(auto mine : m_mines)
    {
        if(mine->to_erase) continue;
        for(Tank* t : tanks)
        {
            if(t->team == mine->team || t->to_erase || !t->testFlag(TSF_LIFE) || dynamic_cast<Turret*>(t) != nullptr) continue;
            if(!overlap(mine->collision_rect, t->collision_rect, 6)) continue;
            Player* owner = nullptr;
            for(auto p : m_players)
                if(p->playerIndex() == mine->owner) owner = p;
            hitTank(t, owner != nullptr ? static_cast<Tank*>(owner) : nullptr);
            mine->detonate();
            break;
        }
        // Qualquer tiro acerta a mina e ela some (dá para limpar o caminho)
        for(Tank* t : tanks)
            for(auto bullet : t->bullets)
                if(!mine->to_erase && !bullet->to_erase && !bullet->collide && overlap(mine->collision_rect, bullet->collision_rect, 1))
                {
                    bullet->destroy();
                    mine->detonate();
                }
    }
    for(auto mine : m_mines) mine->update(dt);
    for(auto turret : m_turrets) turret->update(dt);
}

void Duel::setBaseWalls(int team, SpriteType wall)
{
    // A frente da águia é sempre de pedra; laterais e cantos são do material pedido: tijolo,
    // ou pedra com a pá / para a equipe menor
    std::vector<Tank*> tanks = allTanks();
    for(const DuelLayout::Tile& wall_tile : DuelLayout::baseWallTiles(team))
    {
        int row = wall_tile.row, column = wall_tile.column;

        // Não cria parede em cima de um tanque (ele ficaria preso)
        SDL_Rect tile = {column * AppConfig::tile_rect.w, row * AppConfig::tile_rect.h, AppConfig::tile_rect.w, AppConfig::tile_rect.h};
        bool occupied = false;
        for(Tank* t : tanks)
            if(!t->to_erase && intersects(t->collision_rect, tile)) occupied = true;
        if(occupied) continue;

        delete m_level.at(row).at(column);
        if(wall == ST_STONE_WALL || DuelLayout::isBaseFront(team, row, column))
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
    // (os adversários passam por cima); quem guarda um poder só pega outro depois de usá-lo
    for(auto player : m_players)
        for(auto bonus : m_bonuses)
            if(!player->to_erase && !bonus->to_erase && player->testFlag(TSF_LIFE) && player->held_power == ST_NONE &&
               (bonus->owner_team < 0 || bonus->owner_team == player->team) &&
               intersects(player->collision_rect, bonus->collision_rect))
                applyBonus(player, bonus);

    // Bônus de uma equipe sem jogadores em campo não serve para ninguém
    for(auto bonus : m_bonuses)
        if(bonus->owner_team >= 0 && !teamAlive(bonus->owner_team)) bonus->to_erase = true;

    // Tanques contra o cenário
    for(Tank* t : tanks) checkCollisionTankWithLevel(t, dt);
    for(Tank* t : tanks) tryCornerSlide(t, dt);

    updateAI(dt);
    updatePowers(dt);

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
    // A memória da IA de um tanque sai junto com ele (o endereço pode ser reaproveitado)
    for(Tank* t : tanks)
        if(t->to_erase) m_ai.erase(t);
    erase(m_enemies);
    erase(m_players);
    erase(m_bonuses);
    erase(m_bushes);
    erase(m_turrets);
    erase(m_mines);

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
    // Quem tem o tiro demolidor em cada equipe: a pedra da base adversária pisca junto
    // com o brilho dele, avisando que ela não segura esse tanque
    Player* demolisher[2] = {nullptr, nullptr};
    for(auto player : m_players)
        if(!player->to_erase && player->demolisher() && demolisher[player->team] == nullptr)
            demolisher[player->team] = player;
    for(size_t r = 0; r < m_level.size(); r++)
        for(size_t c = 0; c < m_level[r].size(); c++)
        {
            Object* item = m_level[r][c];
            if(item == nullptr) continue;
            int team = item->type == ST_STONE_WALL ? stoneOwner(static_cast<int>(r), static_cast<int>(c)) : -1;
            if(team < 0) { item->draw(); continue; }
            // Pedra com dono (muralha da águia e paredes inteiras na zona): na cor da equipe
            renderer->drawObjectWithColor(&item->src_rect, &item->dest_rect, stoneTint(team));
            Player* threat = demolisher[1 - team];
            if(threat != nullptr && Player::demolisherGlint(threat->effectTime()))
                renderer->drawWhite(&item->src_rect, &item->dest_rect, Player::DEMOLISHER_GLINT_ALPHA);
        }
    for(auto mine : m_mines) mine->draw();
    for(auto player : m_players) player->draw();
    for(auto enemy : m_enemies) enemy->draw();
    for(auto turret : m_turrets) turret->draw();
    for(auto bush : m_bushes) bush->draw();
    for(auto bonus : m_bonuses) bonus->draw();
    for(Eagle* base : bases()) base->draw();

    // Bônus da equipe: letra da equipe acima do ícone, na cor dela (piscando junto com o bônus)
    for(auto bonus : m_bonuses)
    {
        if(bonus->owner_team < 0 || !bonus->visible() || bonus->to_erase) continue;
        std::string letter = bonus->owner_team == 0 ? "A" : "B";
        SDL_Point size = renderer->textSize(letter, 3);
        renderer->drawTextOutlined({bonus->dest_rect.x + (bonus->dest_rect.w - size.x) / 2, bonus->dest_rect.y - size.y - 1},
                         letter, bonus->color, 3);
    }

    // Número de cada jogador acima do tanque: no começo da rodada e ao renascer
    bool round_start = (m_phase == PHASE_INTRO || (m_phase == PHASE_PLAY && m_phase_time < LABEL_TIME));
    for(auto player : m_players)
    {
        if(!(round_start || player->testFlag(TSF_CREATE)) || player->testFlag(TSF_DESTROYED)) continue;
        std::string label = "P" + Engine::intToString(player->playerIndex() + 1);
        SDL_Point size = renderer->textSize(label, 3);
        SDL_Point p = {player->dest_rect.x + (player->dest_rect.w - size.x) / 2, player->dest_rect.y - size.y - 1};
        if(p.y < 0) p.y = player->dest_rect.y + player->dest_rect.h + 1;
        renderer->drawTextOutlined(p, label, player->color, 3);
    }

    //=========== Painel lateral: equipe B em cima, rodada no meio, equipe A embaixo ===========
    // Cada equipe num bloco da cor dela com texto preto (contraste de ~15:1; o texto
    // colorido direto sobre o cinza do painel ficava em ~1,2:1, quase invisível)
    SDL_Rect flag_src = engine.getSpriteConfig()->getSpriteData(ST_FLAG)->rect;
    const int block_x = AppConfig::status_rect.x + 2, block_w = AppConfig::status_rect.w - 4;
    for(int team = 0; team < 2; team++)
    {
        std::vector<Player*> members;
        for(auto player : m_players)
            if(player->team == team && !player->to_erase) members.push_back(player);
        std::sort(members.begin(), members.end(), [](Player* a, Player* b) { return a->playerIndex() < b->playerIndex(); });
        // Cada jogador: "P1  3" e, embaixo, o espaço do poder guardado (até 3 por equipe)
        const int MEMBER_HEIGHT = 31;
        int height = 4 + 16 + static_cast<int>(members.size()) * MEMBER_HEIGHT + 1;
        int y = (team == 1 ? 6 : AppConfig::map_rect.h - 6 - height);
        SDL_Rect block = {block_x, y, block_w, height};
        renderer->drawRect(&block, teamColor(team), true);

        SDL_Point p = {block_x + 4, y + 4};
        renderer->drawText(&p, team == 0 ? "A" : "B", BLACK, 2);
        // Uma bandeira por rodada vencida
        for(int w = 0; w < m_wins[team]; w++)
        {
            SDL_Rect dst = {block_x + 18 + w * 12, y + 4, 12, 12};
            renderer->drawObject(&flag_src, &dst);
        }
        // Jogadores da equipe, as vidas de cada um ("P1  3") e o poder guardado: o ícone do
        // poder, ou uma moldura vazia; e, ao lado, o canhão de quem tem o tiro demolidor
        for(size_t row = 0; row < members.size(); row++)
        {
            int row_y = y + 21 + static_cast<int>(row) * MEMBER_HEIGHT;
            p = {block_x + 4, row_y};
            renderer->drawText(&p, "P" + Engine::intToString(members[row]->playerIndex() + 1), BLACK, 3);
            p = {block_x + 30, row_y};
            renderer->drawText(&p, Engine::intToString(members[row]->lives_count), BLACK, 3);
            drawPowerSlot(members[row], {block_x + 4, row_y + 12, 16, 16});
            // Ao lado, o canhão de quem tem o tiro demolidor (a pedra da base adversária não o
            // segura de perto)
            if(members[row]->demolisher())
            {
                SDL_Rect gun = engine.getSpriteConfig()->getSpriteData(ST_BONUS_GUN)->rect;
                SDL_Rect mark = {block_x + 24, row_y + 12, 16, 16};
                renderer->drawObject(&gun, &mark);
            }
        }
    }
    // Rodada atual no meio do painel, em branco sobre preto
    SDL_Rect round_box = {block_x, AppConfig::map_rect.h / 2 - 20, block_w, 40};
    renderer->drawRect(&round_box, BLACK, true);
    renderer->drawTextCentered(round_box.x + round_box.w / 2, round_box.y + 5, "RND", GRAY, 3);
    renderer->drawTextCentered(round_box.x + round_box.w / 2, round_box.y + 19, Engine::intToString(m_round), WHITE, 2);

    //=========== Mensagens no centro: sempre numa caixa, centralizada no mapa ===========
    std::string score = Engine::intToString(m_wins[0]) + " - " + Engine::intToString(m_wins[1]);
    if(m_demo) {} // demonstração no fundo do menu: sem caixas por cima das opções
    else if(m_phase == PHASE_INTRO)
    {
        drawMessageBox(renderer, {
            {"ROUND " + Engine::intToString(m_round), WHITE, 1, 8},
            {AppConfig::duel_maps.at(m_map).second, LIGHT_GRAY, 2, 8},
            {"FIRST TO " + Engine::intToString(AppConfig::duel_rounds_to_win) + " WINS", GRAY, 3, 0},
        }, GRAY);
    }
    else if(m_phase == PHASE_PLAY && m_pause)
        drawPauseBox();
    else if(m_phase == PHASE_ROUND_END)
    {
        if(m_round_winner < 0)
            drawMessageBox(renderer, {
                {"DRAW", WHITE, 1, 8},
                {"ROUND REPLAYED", GRAY, 3, 0},
            }, GRAY);
        else
            drawMessageBox(renderer, {
                {TEAM_NAME[m_round_winner], teamColor(m_round_winner), 1, 8},
                {"WINS THE ROUND", WHITE, 2, 8},
                {score, LIGHT_GRAY, 2, 0},
            }, teamColor(m_round_winner));
    }
    else if(m_phase == PHASE_MATCH_END)
    {
        std::vector<MessageLine> lines = {
            {TEAM_NAME[m_round_winner], teamColor(m_round_winner), 1, 8},
            {"WINS THE DUEL", WHITE, 2, 8},
            {score, LIGHT_GRAY, 2, 14},
        };
        // Tabela de eliminações: todas as linhas com o mesmo número de caracteres
        // (a fonte é monoespaçada), então as colunas ficam alinhadas
        for(int i = 0; i < m_config.humans; i++)
        {
            int team = m_config.human_team[i];
            std::string kills = Engine::intToString(m_kills[i]);
            if(kills.size() < 2) kills = " " + kills;
            std::string line = "P" + Engine::intToString(i + 1) + "  " + (team == 0 ? "A" : "B") + "  " + kills +
                               (m_kills[i] == 1 ? " KILL " : " KILLS");
            lines.push_back({line, teamColor(team), 3, 4});
        }
        lines.back().gap = 14;
        // "PRESS FIRE" ocupa o espaço desde o início, para a caixa não mudar de tamanho
        lines.push_back({"PRESS FIRE", WHITE, 3, 0, m_phase_time > MATCH_END_INPUT_DELAY});
        drawMessageBox(renderer, lines, teamColor(m_round_winner));
    }

    renderer->flush();
}

void Duel::eventProcess(SDL_Event* ev)
{
    bool match_over = (m_phase == PHASE_MATCH_END && m_phase_time > MATCH_END_INPUT_DELAY);
    extraModeInput(ev, match_over, m_phase == PHASE_PLAY);
}

AppState* Duel::nextState()
{
    // Volta para a escolha de mapa, com o último selecionado: revanche com um botão
    return new Menu(Menu::SCREEN_DUEL_MAP);
}
