#include "survival.h"
#include "message_box.h"
#include "survival_layout.h"
#include "duel_layout.h"
#include "powers.h"
#include "menu.h"
#include "../engine/engine.h"
#include "../appconfig.h"
#include "../soundmanager.h"
#include "../controllers.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace
{
    const SDL_Color BLACK = {0, 0, 0, 255};
    const SDL_Color WHITE = {255, 255, 255, 255};
    const SDL_Color LIGHT_GRAY = {200, 200, 200, 255};
    const SDL_Color GRAY = {150, 150, 150, 255};
    const SDL_Color RED = {255, 70, 70, 255};
    const SDL_Color GOLD = {255, 215, 0, 255};

    const Uint32 RESULTS_INPUT_DELAY = 1500;
    const Uint32 RESULTS_TIMEOUT = 30000;
    const int CAMPAIGN_STAGES = 35;

    bool intersects(SDL_Rect a, SDL_Rect b)
    {
        SDL_Rect r = intersectRect(&a, &b);
        return r.w > 0 && r.h > 0;
    }
}

Survival::Survival(int players, int map)
    : Game(NoCampaign{})
{
    m_player_count = std::max(1, std::min(4, players));
    int map_count = static_cast<int>(AppConfig::survival_maps.size());
    m_map = (map >= 0 && map < map_count) ? map : rand() % map_count;
    m_current_level = m_map + 1;
    m_destroyed = 0;
    m_wave = 0;
    m_life_reward = false;

    // Mapa fora da geometria (arquivo ausente, tamanho errado...): volta ao menu em vez de
    // rodar uma partida quebrada. Normalmente já foi recusado em SurvivalLayout::loadMapList
    std::string path = AppConfig::survival_levels_path + AppConfig::survival_maps.at(m_map).first;
    std::vector<std::string> problems = SurvivalLayout::validate(DuelLayout::readMap(path));
    if(!problems.empty())
    {
        std::cerr << "Mapa de sobrevivência " << path << " inválido: " << problems.front() << "\n";
        m_finished = true;
        m_phase = PHASE_RESULTS;
        m_phase_time = 0;
        m_wave = 1;
        m_level_start_screen = true; // sem águia nem mapa: o Game desenha só a tela cinza
        return;
    }
    loadLevel(path);
    rebuildBaseWalls(); // a muralha da águia é do jogo, não do arquivo

    // Cada jogador com a sua cor (a da campanha: amarelo, verde, azul e vermelho)
    Controllers::setPlayerCount(m_player_count);
    for(int i = 0; i < m_player_count; i++)
        m_players.push_back(new Player(i));

    SoundManager::getInstance().playSound("level_starting");
    startWave(1);
}

Survival::~Survival()
{
    for(Player* player : m_killed_players) delete player;
    m_killed_players.clear();
}

// ======================== Ondas ========================

int Survival::waveEnemies(int wave)
{
    int count = AppConfig::survival_first_wave_enemies + (wave - 1) * AppConfig::survival_wave_enemy_step;
    return std::min(count, AppConfig::survival_max_wave_enemies);
}

int Survival::difficulty() const
{
    // Escala das fases da campanha: onda 1 ~ fase 3, onda 17 em diante ~ fase 35
    return std::min(CAMPAIGN_STAGES, 2 * m_wave + 1);
}

void Survival::startWave(int wave)
{
    m_wave = wave;
    m_phase = PHASE_WAVE_INTRO;
    m_phase_time = 0;
    m_enemy_to_kill = waveEnemies(wave);
    m_enemy_redy_time = 0;
    m_level_end_time = 0;
}

void Survival::startBreak()
{
    // Recompensa por sobreviver a uma onda: o mapa inteiro regenera (a muralha da águia
    // também), quem tinha caído volta e, a cada N ondas, todos ganham uma vida
    m_phase = PHASE_BREAK;
    m_phase_time = 0;
    m_level_end_time = 0;
    m_revived.clear();
    regenerateMap();
    while(!m_killed_players.empty())
    {
        m_revived.push_back(m_killed_players.front()->playerIndex());
        reviveOne();
    }
    m_life_reward = (m_wave % AppConfig::survival_life_every_waves == 0);
    if(m_life_reward)
    {
        for(Player* player : m_players) player->addLife();
        SoundManager::getInstance().playSound("life");
    }
    placeShop();
}

std::vector<SDL_Point> Survival::baseWallTiles() const
{
    // {coluna, linha}: as laterais (3 de altura) e a frente da muralha da águia
    int rows = m_level_rows_count;
    std::vector<SDL_Point> tiles;
    for(int i = 1; i <= 3; i++)
    {
        tiles.push_back({11, rows - i});
        tiles.push_back({14, rows - i});
    }
    tiles.push_back({12, rows - 3});
    tiles.push_back({13, rows - 3});
    return tiles;
}

void Survival::regenerateMap()
{
    // A águia e a muralha dela ficam de fora: a muralha é do jogo, não do arquivo, e com a
    // pá ativa é de pedra até o tempo acabar
    std::vector<SDL_Point> skip = baseWallTiles();
    int rows = m_level_rows_count;
    for(int column = 12; column <= 13; column++)
        for(int row = rows - 2; row < rows; row++)
            skip.push_back({column, row});
    std::string path = AppConfig::survival_levels_path + AppConfig::survival_maps.at(m_map).first;
    restoreTerrain(DuelLayout::readMap(path), skip);
    rebuildBaseWalls();
}

void Survival::rebuildBaseWalls()
{
    // Pá ativa: a pedra fica até o tempo dela acabar (Game::update devolve os tijolos)
    if(m_protect_eagle) return;

    int t = AppConfig::tile_rect.w;
    std::vector<Tank*> tanks(m_players.begin(), m_players.end());
    tanks.insert(tanks.end(), m_enemies.begin(), m_enemies.end());
    for(SDL_Point tile : baseWallTiles())
    {
        int row = tile.y, column = tile.x;
        if(row < 0 || column >= m_level_columns_count) continue;
        SDL_Rect area = {column * t, row * t, t, t};
        bool occupied = false;
        for(Tank* tank : tanks)
            if(!tank->to_erase && intersects(tank->collision_rect, area)) occupied = true;
        if(occupied) continue; // não prende ninguém dentro da parede

        delete m_level.at(row).at(column);
        m_level.at(row).at(column) = new Brick(column * t, row * t);
    }
}

// ======================== Regras ========================

int Survival::enemyLimit() const
{
    if(m_phase != PHASE_PLAY) return 0; // no aviso da onda, ninguém surge
    if(m_truce_time > 0) return 0;      // trégua: os que estão em campo continuam, novos não
    // Mais inimigos ao mesmo tempo com mais jogadores e a cada poucas ondas
    int limit = AppConfig::survival_first_on_map + (m_player_count - 1) + (m_wave - 1) / AppConfig::survival_on_map_every_waves;
    return std::min(limit, AppConfig::survival_max_on_map);
}

Uint32 Survival::enemySpawnDelay() const
{
    int delay = static_cast<int>(AppConfig::survival_first_spawn_delay) - 100 * (m_wave - 1);
    return static_cast<Uint32>(std::max(delay, static_cast<int>(AppConfig::survival_min_spawn_delay)));
}

void Survival::generateEnemy()
{
    // Como na campanha, nos três pontos do topo, em rodízio; com muitos inimigos ao mesmo
    // tempo, um ponto ocupado é pulado (o inimigo novo não nasce dentro de outro tanque)
    int points = static_cast<int>(AppConfig::enemy_starting_point.size());
    std::vector<Tank*> tanks(m_players.begin(), m_players.end());
    tanks.insert(tanks.end(), m_enemies.begin(), m_enemies.end());
    for(int k = 0; k < points; k++)
    {
        SDL_Point point = AppConfig::enemy_starting_point.at(m_enemy_respown_position);
        m_enemy_respown_position = (m_enemy_respown_position + 1) % points;

        SDL_Rect area = {point.x, point.y, 2 * AppConfig::tile_rect.w, 2 * AppConfig::tile_rect.h};
        bool occupied = false;
        for(Tank* tank : tanks)
            if(!tank->to_erase && (intersects(tank->collision_rect, area) || intersects(tank->dest_rect, area))) occupied = true;
        if(occupied) continue;

        m_enemies.push_back(createEnemy(point, difficulty()));
        return;
    }
}

// ======================== Poderes ========================

SpriteType Survival::randomBonusType()
{
    if(!AppConfig::survival_shop || shopItems().empty()) return Powers::draw(Powers::survivalTable());
    // Com a loja, os poderes novos só são comprados: caem os 8 originais, como na campanha
    static std::vector<Powers::Weight> classic;
    if(classic.empty())
        for(const Powers::Weight& w : Powers::survivalTable())
            if(!Powers::isExtra(w.type)) classic.push_back(w);
    return Powers::draw(classic);
}

// ======================== Loja ========================

std::vector<std::pair<SpriteType, int>> Survival::shopItems()
{
    std::vector<std::pair<SpriteType, int>> items;
    for(const auto& entry : AppConfig::survival_shop_items)
        for(int t = ST_BONUS_MINE; t <= ST_BONUS_TEAM_SHIELD; t++)
            if(entry.first == Powers::name(static_cast<SpriteType>(t)))
                items.push_back({static_cast<SpriteType>(t), entry.second});
    // Do mais barato ao mais caro (empate: a ordem da lista)
    std::stable_sort(items.begin(), items.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
    return items;
}

int Survival::coins() const
{
    int points = 0;
    for(const Player* p : m_players) points += p->score;
    for(const Player* p : m_killed_players) points += p->score;
    return std::max(0, points / std::max(1, AppConfig::survival_points_per_coin) - m_coins_spent);
}

Uint32 Survival::breakTime() const
{
    // O intervalo inteiro (survival_break_time) termina com o aviso da onda; sem loja, não há
    // o que esperar e o aviso vem logo
    if(!AppConfig::survival_shop || m_shop_pad.x < 0) return 0;
    Uint32 total = AppConfig::survival_break_time, intro = AppConfig::survival_wave_intro_time;
    return total > intro ? total - intro : 0;
}

bool Survival::shopOpen() const
{
    return AppConfig::survival_shop && m_phase == PHASE_BREAK && m_shop_pad.x >= 0 && !shopItems().empty();
}

void Survival::placeShop()
{
    // Ao lado da base, de um lado sorteado; o lugar mais perto da águia desse lado que não
    // esteja ocupado por barricada, torreta ou mina (as barricadas ficam depois de regenerar)
    m_shop_pad = {-1, -1};
    m_shop_user = -1;
    if(!AppConfig::survival_shop) return;
    std::string path = AppConfig::survival_levels_path + AppConfig::survival_maps.at(m_map).first;
    std::vector<SurvivalLayout::Tile> spots = SurvivalLayout::shopSpots(DuelLayout::readMap(path));
    const int t = AppConfig::tile_rect.w;
    int first_side = rand() % 2; // 0: esquerda, 1: direita
    for(int pass = 0; pass < 2 && m_shop_pad.x < 0; pass++)
    {
        bool want_left = (pass == 0) == (first_side == 0);
        for(const SurvivalLayout::Tile& spot : spots)
        {
            bool left = spot.column + 1 < 11;
            if(left != want_left) continue;
            bool free = true;
            for(int r = spot.row; r < spot.row + 2; r++)
                for(int c = spot.column; c < spot.column + 2; c++)
                    if(m_level.at(r).at(c) != nullptr && m_level.at(r).at(c)->type != ST_ICE) free = false;
            SDL_Rect area = {spot.column * t, spot.row * t, 2 * t, 2 * t};
            for(Turret* turret : m_turrets) if(!turret->to_erase && intersects(turret->collision_rect, area)) free = false;
            for(Mine* mine : m_mines) if(!mine->to_erase && intersects(mine->collision_rect, area)) free = false;
            if(!free) continue;
            m_shop_pad = {spot.column * t, spot.row * t};
            break;
        }
    }
}

bool Survival::onShop(const Player* player) const
{
    return std::abs(player->pos_x - m_shop_pad.x) <= 8 && std::abs(player->pos_y - m_shop_pad.y) <= 8;
}

void Survival::updateShop()
{
    std::vector<std::pair<SpriteType, int>> items = shopItems();
    bool open = shopOpen();

    // Quem está em cima é o único na loja: o tanque dele ocupa o lugar inteiro e barra os
    // outros (os tanques dos jogadores não se atravessam)
    if(m_shop_user >= 0)
    {
        bool still = false;
        for(Player* p : m_players)
            if(p->playerIndex() == m_shop_user && open && p->testFlag(TSF_LIFE) && !p->to_erase && onShop(p)) still = true;
        if(!still) m_shop_user = -1;
    }
    if(open && m_shop_user < 0)
        for(Player* p : m_players)
            if(p->testFlag(TSF_LIFE) && !p->to_erase && onShop(p)) { m_shop_user = p->playerIndex(); break; }

    for(Player* player : m_players)
    {
        bool shopping = (player->playerIndex() == m_shop_user);
        bool power = player->takePowerPress();
        bool fire = player->takeFirePress();
        int step = player->takeShopStep();
        player->shop_mode = shopping;
        if(shopping)
        {
            // LB / RB escolhem o item (dando a volta), tiro compra
            int n = static_cast<int>(items.size());
            if(step != 0)
            {
                m_shop_item = ((m_shop_item + step) % n + n) % n;
                SoundManager::getInstance().playSound("bonus");
            }
            if(fire)
            {
                const auto& item = items.at(m_shop_item % n);
                buy(player, item.first, item.second);
            }
            continue;
        }
        // Fora da loja: o botão de poder usa o poder guardado (sem espaço, continua guardado)
        if(power && player->held_power != ST_NONE && player->testFlag(TSF_LIFE) && usePower(player))
            player->held_power = ST_NONE;
    }
}

bool Survival::buy(Player* player, SpriteType type, int price)
{
    if(coins() < price) return false;
    if(Powers::storable(type) && player->held_power != ST_NONE) return false;
    m_coins_spent += price;
    SoundManager::getInstance().playSound("life");
    givePower(player, type); // sem pontos: comprar não pode render moedas
    return true;
}

bool Survival::reservedTile(int row, int column)
{
    // Nada de barricada ou torreta em cima de onde os inimigos surgem ou os jogadores nascem
    std::vector<SDL_Point> points(AppConfig::enemy_starting_point.begin(), AppConfig::enemy_starting_point.end());
    points.insert(points.end(), AppConfig::player_starting_point.begin(), AppConfig::player_starting_point.end());
    for(SDL_Point p : points)
    {
        int r = p.y / AppConfig::tile_rect.h, c = p.x / AppConfig::tile_rect.w;
        if(row >= r && row < r + 2 && column >= c && column < c + 2) return true;
    }
    return false;
}

void Survival::checkCollisionPlayerWithBonus(Player* player, Bonus* bonus)
{
    if(player->to_erase || bonus->to_erase) return;
    SDL_Rect hit = intersectRect(&player->collision_rect, &bonus->collision_rect);
    if(hit.w <= 0 || hit.h <= 0) return;

    // Quem guarda um poder só pega outro bônus depois de usar o que tem
    if(player->held_power != ST_NONE) return;

    SpriteType type = bonus->type;
    if(!Powers::isExtra(type))
    {
        Game::checkCollisionPlayerWithBonus(player, bonus); // os 8 originais, como na campanha
        return;
    }
    SoundManager::getInstance().playSound("bonus");
    player->score += 300;
    bonus->to_erase = true;
    givePower(player, type);
}

void Survival::givePower(Player* player, SpriteType type)
{
    if(Powers::storable(type))
    {
        player->held_power = type;
        // Com os poderes de uso imediato (AppConfig::survival_store_powers = false), vale na
        // hora; se não couber ali (barricada sem espaço...), fica guardado
        if(!AppConfig::survival_store_powers && usePower(player)) player->held_power = ST_NONE;
        return;
    }
    switch(type)
    {
    case ST_BONUS_REVIVE:
    {
        // Um companheiro que caiu volta; ninguém caído: vida extra para quem tem menos
        if(reviveOne()) break;
        Player* weakest = player;
        for(Player* p : m_players)
            if(!p->to_erase && p->lives_count < weakest->lives_count) weakest = p;
        weakest->addLife();
        break;
    }
    case ST_BONUS_REPAIR:
        rebuildBaseWalls();
        break;
    case ST_BONUS_TRUCE:
        m_truce_time = AppConfig::power_truce_time;
        break;
    case ST_BONUS_TEAM_SHIELD:
        for(Player* p : m_players)
            if(!p->to_erase) p->setFlag(TSF_SHIELD); // como o capacete da campanha
        break;
    default:
        break;
    }
}

bool Survival::reviveOne()
{
    if(m_killed_players.empty()) return false;
    Player* player = m_killed_players.front();
    m_killed_players.erase(m_killed_players.begin());
    player->to_erase = false;
    player->shop_mode = false;
    player->lives_count = 2; // respawn() gasta uma ao entrar no mapa
    player->respawn();
    m_players.push_back(player);
    SoundManager::getInstance().playSound("life");
    return true;
}

bool Survival::usePower(Player* player)
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
        // O próprio ponto de nascimento, ao lado da águia; ocupado, o de outro jogador
        std::vector<SDL_Point> points;
        int index = player->playerIndex();
        if(index >= 0 && index < static_cast<int>(AppConfig::player_starting_point.size()))
            points.push_back(AppConfig::player_starting_point[index]);
        points.insert(points.end(), AppConfig::player_starting_point.begin(), AppConfig::player_starting_point.end());
        return recall(player, points);
    }
    default:
        return false;
    }
}

bool Survival::turretShot(Turret* turret, Direction d)
{
    const int t = AppConfig::tile_rect.w;
    SDL_Point c = {turret->dest_rect.x + turret->dest_rect.w / 2, turret->dest_rect.y + turret->dest_rect.h / 2};
    bool vertical = (d == D_UP || d == D_DOWN);

    // Inimigo mais perto alinhado com o cano (o tiro tem 8 px de largura)
    Enemy* target = nullptr;
    int best = AppConfig::power_turret_range * t + 1;
    for(Enemy* e : m_enemies)
    {
        if(e->to_erase || !e->testFlag(TSF_LIFE)) continue;
        SDL_Rect r = e->collision_rect;
        bool aligned = vertical ? (r.x < c.x + 4 && r.x + r.w > c.x - 4) : (r.y < c.y + 4 && r.y + r.h > c.y - 4);
        if(!aligned) continue;
        int distance;
        switch(d)
        {
        case D_UP: distance = c.y - (r.y + r.h); break;
        case D_DOWN: distance = r.y - c.y; break;
        case D_LEFT: distance = c.x - (r.x + r.w); break;
        default: distance = r.x - c.x; break;
        }
        if(distance >= 0 && distance < best) { best = distance; target = e; }
    }
    if(target == nullptr) return false;

    // Caminho do tiro até o alvo: sem pedra, sem a águia e sem a muralha dela
    int lane0 = ((vertical ? c.x : c.y) - 4) / t, lane1 = ((vertical ? c.x : c.y) + 3) / t;
    int from = (vertical ? c.y : c.x) / t;
    int to = vertical ? (d == D_UP ? target->collision_rect.y + target->collision_rect.h : target->collision_rect.y) / t
                      : (d == D_LEFT ? target->collision_rect.x + target->collision_rect.w : target->collision_rect.x) / t;
    int step = (to >= from) ? 1 : -1;
    std::vector<SurvivalLayout::Tile> wall = SurvivalLayout::baseWallTiles();
    for(int i = from; i != to + step; i += step)
        for(int j = lane0; j <= lane1; j++)
        {
            int row = vertical ? i : j, column = vertical ? j : i;
            if(row < 0 || column < 0 || row >= m_level_rows_count || column >= m_level_columns_count) continue;
            Object* o = m_level.at(row).at(column);
            if(o != nullptr && o->type == ST_STONE_WALL) return false;
            for(const SurvivalLayout::Tile& w : wall)
                if(w.row == row && w.column == column) return false;
            if(row >= m_level_rows_count - 2 && (column == 12 || column == 13)) return false; // águia
        }
    return true;
}

// ======================== Laço do jogo ========================

void Survival::update(Uint32 dt)
{
    if(dt > 40) return;

    if(m_phase == PHASE_RESULTS)
    {
        m_phase_time += dt;
        if(m_eagle != nullptr) m_eagle->update(dt); // explosão da águia
        if(m_phase_time > RESULTS_TIMEOUT) m_finished = true;
        return;
    }

    // Torretas: miram antes do Game atualizar e mover tudo
    if(!m_pause)
        for(Turret* turret : m_turrets)
            turret->think([&](Direction d) { return turretShot(turret, d); });

    int before = m_enemy_to_kill;
    Game::update(dt);
    if(m_pause) return;
    m_phase_time += dt;
    m_destroyed += std::max(0, before - m_enemy_to_kill);
    m_truce_time = m_truce_time > dt ? m_truce_time - dt : 0;

    // Loja e botão de poder (fora da loja, usa o poder guardado; sem espaço, continua guardado)
    updateShop();

    if(m_phase == PHASE_WAVE_INTRO && m_phase_time > AppConfig::survival_wave_intro_time)
    {
        m_phase = PHASE_PLAY;
        m_phase_time = 0;
    }
    // Fim do intervalo: a loja some e vem o aviso da próxima onda
    if(m_phase == PHASE_BREAK && m_phase_time >= breakTime())
        startWave(m_wave + 1);

    // O Game encerra a fase quando os inimigos acabam; aqui isso só encerra a onda. No
    // intervalo continua sem inimigos, e o Game repete o aviso: não é outra onda vencida
    if(m_finished && !m_game_over)
    {
        m_finished = false;
        if(m_phase == PHASE_PLAY) startBreak();
    }
    // Fim de jogo (águia destruída ou todos sem vidas), depois do "GAME OVER" do Game
    else if(m_finished && m_game_over)
    {
        m_finished = false;
        m_phase = PHASE_RESULTS;
        m_phase_time = 0;
        SoundManager::getInstance().playSound("game_over");
    }
}

void Survival::eventProcess(SDL_Event* ev)
{
    bool results_ready = (m_phase == PHASE_RESULTS && m_phase_time > RESULTS_INPUT_DELAY);
    extraModeInput(ev, results_ready, m_phase != PHASE_RESULTS && !m_game_over);
}

AppState* Survival::nextState()
{
    // Volta para a escolha de mapa, com o último selecionado: jogar de novo é um botão só
    return new Menu(Menu::SCREEN_SURVIVAL_MAP);
}

// ======================== Desenho ========================

void Survival::drawStatus()
{
    Engine& engine = Engine::getEngine();
    Renderer* renderer = engine.getRenderer();
    const int x = AppConfig::status_rect.x, w = AppConfig::status_rect.w;

    // Onda atual no topo do painel, em branco sobre preto (como o "RND" do duelo)
    SDL_Rect wave_box = {x + 2, 6, w - 4, 40};
    renderer->drawRect(&wave_box, BLACK, true);
    renderer->drawTextCentered(wave_box.x + wave_box.w / 2, wave_box.y + 5, "WAVE", GRAY, 3);
    renderer->drawTextCentered(wave_box.x + wave_box.w / 2, wave_box.y + 19, Engine::intToString(m_wave), WHITE, 2);

    // Inimigos que faltam na onda: um ícone por inimigo (como na campanha), até 20; o resto
    // vira número para não invadir as vidas dos jogadores
    SDL_Rect src = engine.getSpriteConfig()->getSpriteData(ST_LEFT_ENEMY)->rect;
    int icons = std::min(m_enemy_to_kill, 20);
    for(int i = 0; i < icons; i++)
    {
        SDL_Rect dst = {x + 8 + src.w * (i % 2), 54 + src.h * (i / 2), src.w, src.h};
        renderer->drawObject(&src, &dst);
    }
    if(m_enemy_to_kill > 20)
    {
        SDL_Point p = {x + 8, 54 + src.h * 10 + 2};
        renderer->drawText(&p, "+" + Engine::intToString(m_enemy_to_kill - 20), BLACK, 3);
    }

    // Vidas de cada jogador, com o ícone na cor dele (quem caiu aparece com 0)
    std::vector<Player*> all(m_players.begin(), m_players.end());
    all.insert(all.end(), m_killed_players.begin(), m_killed_players.end());
    std::sort(all.begin(), all.end(), [](Player* a, Player* b) { return a->playerIndex() < b->playerIndex(); });
    int row = 0;
    for(Player* player : all)
    {
        SDL_Rect dst = {x + 5, 250 + row * 18, 16, 16};
        SDL_Rect icon = engine.getSpriteConfig()->getSpriteData(player->type)->rect;
        renderer->drawObjectWithColor(&icon, &dst, player->color);
        SDL_Point p = {dst.x + dst.w + 2, dst.y + 3};
        bool out = std::find(m_killed_players.begin(), m_killed_players.end(), player) != m_killed_players.end();
        renderer->drawText(&p, Engine::intToString(out ? 0 : player->lives_count), BLACK, 3);
        // Poder guardado: o ícone, ou uma moldura vazia
        drawPowerSlot(out ? nullptr : player, {x + 31, dst.y + 1, 14, 14});
        row++;
    }

    // Moedas da equipe para a loja: texto preto sobre bloco dourado (V5), com o número na
    // fonte média (na pequena ele se confundia com as vidas logo acima)
    if(AppConfig::survival_shop)
    {
        SDL_Rect box = {x + 2, 330, w - 4, 40};
        renderer->drawRect(&box, GOLD, true);
        renderer->drawTextCentered(box.x + box.w / 2, box.y + 4, "$", BLACK, 2);
        int value = std::min(coins(), 99999);
        renderer->drawTextCentered(box.x + box.w / 2, box.y + 22, Engine::intToString(value), BLACK, value < 1000 ? 2 : 3);
    }
}

void Survival::drawFloor()
{
    // Lugar da loja no intervalo: quadrado dourado com "$" no chão (o tanque em cima o cobre)
    if(!shopOpen()) return;
    Renderer* renderer = Engine::getEngine().getRenderer();
    const int size = 2 * AppConfig::tile_rect.w;
    SDL_Rect pad = {m_shop_pad.x, m_shop_pad.y, size, size};
    const SDL_Color dark_gold = {90, 70, 0, 255};
    renderer->drawRect(&pad, dark_gold, true);
    for(int k = 0; k < 2; k++)
    {
        SDL_Rect border = {pad.x + k, pad.y + k, pad.w - 2 * k, pad.h - 2 * k};
        renderer->drawRect(&border, GOLD, false);
    }
    SDL_Point text = renderer->textSize("$", 2);
    SDL_Point p = {pad.x + (pad.w - text.x) / 2, pad.y + (pad.h - text.y) / 2};
    renderer->drawText(&p, "$", GOLD, 2);
}

void Survival::drawShop()
{
    // Caixa ao lado da loja, do lado de fora (loja à esquerda da base: caixa à esquerda), para
    // não tapar a águia. Sem ninguém em cima, só o nome e o saldo; com alguém, o item escolhido
    if(!shopOpen()) return;
    Renderer* renderer = Engine::getEngine().getRenderer();
    std::vector<std::pair<SpriteType, int>> items = shopItems();
    const Player* user = nullptr;
    for(const Player* p : m_players) if(p->playerIndex() == m_shop_user) user = p;

    struct Line { std::string text; SDL_Color color; };
    std::vector<Line> lines;
    SpriteType icon = ST_NONE;
    if(user != nullptr)
    {
        const auto& item = items.at(m_shop_item % items.size());
        icon = item.first;
        lines.push_back({"$" + Engine::intToString(item.second), coins() >= item.second ? GOLD : RED});
        lines.push_back({shopName(item.first), WHITE});
        lines.push_back({"COINS " + Engine::intToString(coins()), GOLD});
        if(Powers::storable(item.first) && user->held_power != ST_NONE)
            lines.push_back({"SLOT FULL", RED});
        else
            lines.push_back({"FIRE: BUY", LIGHT_GRAY});
        // Na loja, LB escolhe o item em vez de usar o poder guardado: para usar, é sair dela
        if(user->held_power != ST_NONE)
            lines.push_back({"USE OUTSIDE", LIGHT_GRAY});
    }
    else
    {
        lines.push_back({"SHOP", GOLD});
        lines.push_back({"COINS " + Engine::intToString(coins()), GOLD});
    }

    // Primeira linha com o item: "◀ ícone ▶  $8"
    const int pad_x = 5, pad_y = 5, gap = 4, icon_size = 16, arrow = 5;
    const int icon_w = icon != ST_NONE ? arrow + 3 + icon_size + 3 + arrow + 6 : 0;
    int width = 0, height = pad_y;
    for(size_t i = 0; i < lines.size(); i++)
    {
        SDL_Point size = renderer->textSize(lines[i].text, 3);
        int line_w = size.x + (i == 0 ? icon_w : 0);
        width = std::max(width, line_w);
        height += std::max(size.y, i == 0 && icon != ST_NONE ? icon_size : 0) + gap;
    }
    width += 2 * pad_x;
    height += pad_y - gap;

    const int tank = 2 * AppConfig::tile_rect.w;
    bool left = m_shop_pad.x + tank / 2 < AppConfig::map_rect.w / 2;
    SDL_Rect box = {left ? m_shop_pad.x - 3 - width : m_shop_pad.x + tank + 3, m_shop_pad.y + tank - height, width, height};
    box.x = std::max(0, std::min(box.x, AppConfig::map_rect.w - box.w));
    box.y = std::max(0, box.y);
    SDL_Color border_color = user != nullptr ? user->color : GOLD;
    SDL_Rect border = {box.x - 1, box.y - 1, box.w + 2, box.h + 2};
    renderer->drawRect(&border, border_color, true);
    renderer->drawRect(&box, BLACK, true);

    int y = box.y + pad_y;
    for(size_t i = 0; i < lines.size(); i++)
    {
        SDL_Point size = renderer->textSize(lines[i].text, 3);
        int x = box.x + pad_x, line_h = size.y;
        if(i == 0 && icon != ST_NONE)
        {
            line_h = icon_size;
            // Setas de LB (esquerda) e RB (direita) em volta do ícone: a ponta é a coluna de
            // 1 px (k = 0), do lado de fora
            for(int k = 0; k < arrow; k++)
            {
                SDL_Rect l = {x + k, y + icon_size / 2 - k, 1, 2 * k + 1};
                renderer->drawRect(&l, LIGHT_GRAY, true);
            }
            SDL_Rect src = Engine::getEngine().getSpriteConfig()->getSpriteData(icon)->rect;
            SDL_Rect dst = {x + arrow + 3, y, icon_size, icon_size};
            renderer->drawObject(&src, &dst);
            int rx = dst.x + icon_size + 3;
            for(int k = 0; k < arrow; k++)
            {
                SDL_Rect r = {rx + arrow - 1 - k, y + icon_size / 2 - k, 1, 2 * k + 1};
                renderer->drawRect(&r, LIGHT_GRAY, true);
            }
            x = rx + arrow + 6;
        }
        SDL_Point p = {x, y + (line_h - size.y) / 2};
        renderer->drawText(&p, lines[i].text, lines[i].color, 3);
        y += line_h + gap;
    }
}

std::string Survival::shopName(SpriteType type)
{
    switch(type)
    {
    case ST_BONUS_MINE: return "MINE";
    case ST_BONUS_BARRICADE: return "BARRICADE";
    case ST_BONUS_TURRET: return "TURRET";
    case ST_BONUS_RECALL: return "RECALL";
    case ST_BONUS_TURBO: return "TURBO";
    case ST_BONUS_REVIVE: return "REVIVE";
    case ST_BONUS_REPAIR: return "REPAIR";
    case ST_BONUS_TRUCE: return "TRUCE";
    case ST_BONUS_TEAM_SHIELD: return "TEAM SHIELD";
    default: return "?";
    }
}

void Survival::drawOverlay()
{
    Renderer* renderer = Engine::getEngine().getRenderer();

    if(!m_pause) drawShop();

    // Intervalo: onda vencida, recompensas e contagem até a próxima, no alto do mapa (a loja
    // e a base ficam embaixo)
    if(m_phase == PHASE_BREAK && !m_pause)
    {
        Uint32 left = (breakTime() > m_phase_time ? breakTime() - m_phase_time : 0) + AppConfig::survival_wave_intro_time;
        std::vector<MessageLine> lines = {{"WAVE " + Engine::intToString(m_wave) + " CLEAR", GOLD, 2, 6}};
        for(int idx : m_revived)
            lines.push_back({"P" + Engine::intToString(idx + 1) + " IS BACK", Player::getPlayerColor(idx), 3, 4});
        if(m_life_reward) lines.push_back({"+1 LIFE", GOLD, 3, 4});
        lines.back().gap = 6;
        lines.push_back({"NEXT WAVE IN " + Engine::intToString(static_cast<int>((left + 999) / 1000)), WHITE, 3, 0});
        drawMessageBox(renderer, lines, GOLD, 12);
    }

    // Trégua: contagem no alto do mapa enquanto os inimigos não surgem
    if(m_truce_time > 0 && m_phase == PHASE_PLAY)
    {
        std::string text = "TRUCE " + Engine::intToString(static_cast<int>((m_truce_time + 999) / 1000));
        SDL_Point size = renderer->textSize(text, 2);
        renderer->drawTextOutlined({AppConfig::map_rect.x + (AppConfig::map_rect.w - size.x) / 2, 40}, text, WHITE, 2);
    }

    if(m_phase == PHASE_WAVE_INTRO && !m_pause)
    {
        std::vector<MessageLine> lines = {
            {"WAVE " + Engine::intToString(m_wave), WHITE, 1, 8},
            {Engine::intToString(m_enemy_to_kill) + " ENEMIES", LIGHT_GRAY, 2, 0},
        };
        if(m_wave == 1)
        {
            lines.insert(lines.begin(), {AppConfig::survival_maps.at(m_map).second, LIGHT_GRAY, 2, 10});
            lines.insert(lines.begin(), {"SURVIVAL", GOLD, 2, 6});
        }
        drawMessageBox(renderer, lines, GRAY);
    }
    else if(m_phase == PHASE_RESULTS)
    {
        std::vector<MessageLine> lines = {
            {"GAME OVER", RED, 1, 8},
            {"WAVE " + Engine::intToString(m_wave), WHITE, 2, 6},
            {Engine::intToString(m_destroyed) + " TANKS DESTROYED", LIGHT_GRAY, 3, 14},
        };
        // Pontos de cada jogador, na cor dele, com as colunas alinhadas (fonte monoespaçada)
        std::vector<Player*> all(m_players.begin(), m_players.end());
        all.insert(all.end(), m_killed_players.begin(), m_killed_players.end());
        std::sort(all.begin(), all.end(), [](Player* a, Player* b) { return a->playerIndex() < b->playerIndex(); });
        for(Player* player : all)
        {
            std::string score = Engine::intToString(player->score);
            if(score.size() < 7) score = std::string(7 - score.size(), ' ') + score;
            lines.push_back({"P" + Engine::intToString(player->playerIndex() + 1) + "  " + score, player->color, 3, 4});
        }
        lines.back().gap = 14;
        lines.push_back({"PRESS FIRE", WHITE, 3, 0, m_phase_time > RESULTS_INPUT_DELAY});
        drawMessageBox(renderer, lines, RED);
    }
}
