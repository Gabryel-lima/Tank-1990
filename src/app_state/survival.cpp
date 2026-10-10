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

Survival::Survival(int players, int map, bool shared_coins)
    : Game(NoCampaign{})
{
    m_player_count = std::max(1, std::min(4, players));
    m_shared_coins = shared_coins;
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
    for(auto& effect : m_breach_effects) delete effect.first;
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
    tanks.insert(tanks.end(), m_allies.begin(), m_allies.end());
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
    tanks.insert(tanks.end(), m_allies.begin(), m_allies.end());
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

std::vector<SDL_Point> Survival::bonusSpots() const
{
    // Células = posições de tanque alinhadas à grade (canto 2x2). Livre: os 4 tiles sem bloco
    // que pare um tanque (gelo e arbusto não param; tijolo, pedra e água param)
    const int t = AppConfig::tile_rect.w;
    const int rows = m_level_rows_count - 1, columns = m_level_columns_count - 1;
    if(rows <= 0 || columns <= 0) return {};
    std::vector<SDL_Rect> busy;
    if(m_eagle != nullptr) busy.push_back(m_eagle->collision_rect);
    for(const Turret* turret : m_turrets) if(!turret->to_erase) busy.push_back(turret->dest_rect);
    auto open = [&](int r, int c) {
        for(int i = r; i < r + 2; i++)
            for(int j = c; j < c + 2; j++)
            {
                const Object* o = m_level.at(i).at(j);
                if(o != nullptr && o->type != ST_ICE) return false;
            }
        SDL_Rect area = {c * t, r * t, 2 * t, 2 * t};
        for(const SDL_Rect& b : busy)
            if(SDL_HasIntersection(&b, &area)) return false;
        return true;
    };

    // Aonde se chega andando: a partir dos nascimentos dos jogadores, um tile por passo
    std::vector<char> seen(rows * columns, 0);
    std::vector<SDL_Point> queue;
    for(SDL_Point p : AppConfig::player_starting_point)
    {
        int r = p.y / t, c = p.x / t;
        if(r >= 0 && c >= 0 && r < rows && c < columns && open(r, c) && !seen[r * columns + c])
        {
            seen[r * columns + c] = 1;
            queue.push_back({c, r});
        }
    }
    for(size_t k = 0; k < queue.size(); k++)
    {
        const int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};
        for(int d = 0; d < 4; d++)
        {
            int r = queue[k].y + dr[d], c = queue[k].x + dc[d];
            if(r < 0 || c < 0 || r >= rows || c >= columns || seen[r * columns + c] || !open(r, c)) continue;
            seen[r * columns + c] = 1;
            queue.push_back({c, r});
        }
    }

    std::vector<SDL_Point> spots;
    for(SDL_Point cell : queue)
    {
        SDL_Rect area = {cell.x * t, cell.y * t, 2 * t, 2 * t};
        bool on_shop = false;
        for(SDL_Point pad : m_shop_pads)
        {
            SDL_Rect shop = {pad.x, pad.y, 2 * t, 2 * t};
            if(pad.x >= 0 && SDL_HasIntersection(&shop, &area)) on_shop = true;
        }
        if(!on_shop) spots.push_back({cell.x * t, cell.y * t});
    }
    return spots;
}

bool Survival::bonusSpot(SDL_Point* spot)
{
    // Só onde os tanques andam: nada de bônus dentro de tijolo, pedra ou água (o sorteio da
    // campanha usa qualquer pixel do mapa). Sem lugar nenhum (improvável), o da campanha
    std::vector<SDL_Point> spots = bonusSpots();
    if(spots.empty()) return false;
    *spot = spots.at(rand() % spots.size());
    return true;
}

// ======================== Loja ========================

std::vector<Survival::ShopItem> Survival::shopItems()
{
    std::vector<ShopItem> items;
    for(const auto& entry : AppConfig::survival_shop_items)
    {
        if(entry.first == "star") items.push_back({ST_BONUS_STAR, entry.second});
        else if(entry.first == "slot") items.push_back({ST_NONE, entry.second});
        else if(entry.first == "reinforce") items.push_back({ST_BONUS_TANK, entry.second});
        else
            for(int t = ST_BONUS_MINE; t <= ST_BONUS_TEAM_SHIELD; t++)
                if(entry.first == Powers::name(static_cast<SpriteType>(t)))
                    items.push_back({static_cast<SpriteType>(t), entry.second});
    }
    // Do mais barato ao mais caro (empate: a ordem da lista)
    std::stable_sort(items.begin(), items.end(), [](const ShopItem& a, const ShopItem& b) { return a.price < b.price; });
    return items;
}

int Survival::price(const ShopItem& item, const Player* player) const
{
    if(item.type == ST_BONUS_STAR) return item.price + player->stars() * AppConfig::survival_star_price_step;
    if(item.type == ST_NONE) return item.price + (player->power_slots - 1) * AppConfig::survival_slot_price_step;
    return item.price;
}

std::string Survival::cannotBuy(const ShopItem& item, const Player* player) const
{
    // Primeiro o que nenhuma moeda resolve; por último, quanto falta
    if(item.type == ST_BONUS_STAR && player->stars() >= 3) return "MAX LEVEL";
    if(item.type == ST_NONE && player->power_slots >= AppConfig::survival_max_slots) return "MAX SLOTS";
    if(item.type == ST_BONUS_TANK)
    {
        if(static_cast<int>(m_allies.size()) >= AppConfig::survival_reinforce_max) return "MAX ALLIES";
        if(allySpawn().x < 0) return "NO ROOM";
    }
    if(storesPower(item.type) && player->storedPowers() >= player->power_slots) return "SLOTS FULL";
    if(Powers::isExtra(item.type) && !storesPower(item.type))
    {
        std::string why = noEffect(item.type);
        if(!why.empty()) return why;
    }
    int missing = price(item, player) - coins(player);
    if(missing > 0) return "NEED " + Engine::intToString(missing) + " MORE";
    return "";
}

int Survival::coins(const Player* player) const
{
    if(m_shared_coins) return teamCoins();
    // Individuais: os pontos do jogador, as moedas iniciais dele e o que ele gastou
    int index = player->playerIndex();
    int points = static_cast<int>(player->score) / std::max(1, AppConfig::survival_points_per_coin);
    return std::max(0, points + AppConfig::survival_start_coins - m_coins_spent[index]);
}

int Survival::teamCoins() const
{
    // Os pontos de todos somados antes de virar moedas, mais as moedas iniciais de cada um,
    // menos o que todos gastaram
    int points = 0, spent = 0;
    for(const Player* p : m_players) points += p->score;
    for(const Player* p : m_killed_players) points += p->score;
    for(int s : m_coins_spent) spent += s;
    int per_coin = std::max(1, AppConfig::survival_points_per_coin);
    return std::max(0, points / per_coin + m_player_count * AppConfig::survival_start_coins - spent);
}

Uint32 Survival::timeToWave() const
{
    Uint32 intro = AppConfig::survival_wave_intro_time;
    if(m_phase == PHASE_BREAK)
        return (breakTime() > m_phase_time ? breakTime() - m_phase_time : 0) + intro;
    if(m_phase == PHASE_WAVE_INTRO)
        return intro > m_phase_time ? intro - m_phase_time : 0;
    return 0;
}

int Survival::countdown() const
{
    // Arredonda para cima: "1" até o último instante, "0" só quando a onda começa
    return static_cast<int>((timeToWave() + 999) / 1000);
}

Uint32 Survival::breakTime() const
{
    // O intervalo inteiro (survival_break_time) termina com o aviso da onda; sem loja, não há
    // o que esperar e o aviso vem logo
    bool any = false;
    for(int i = 0; i < m_player_count; i++) if(hasShop(i)) any = true;
    if(!AppConfig::survival_shop || !any) return 0;
    Uint32 total = AppConfig::survival_break_time, intro = AppConfig::survival_wave_intro_time;
    return total > intro ? total - intro : 0;
}

bool Survival::shopOpen() const
{
    if(!AppConfig::survival_shop || m_phase != PHASE_BREAK || shopItems().empty()) return false;
    for(int i = 0; i < m_player_count; i++) if(hasShop(i)) return true;
    return false;
}

bool Survival::hasShop(int index) const
{
    return index >= 0 && index < 4 && m_shop_pads[index].x >= 0;
}

void Survival::placeShop()
{
    // Ao lado da base, uma por jogador, do lado de onde ele nasce (P1 e P3 à esquerda, P2 e
    // P4 à direita), a mais perto da águia que não esteja ocupada por barricada, torreta, mina
    // ou outra loja (as barricadas ficam depois de regenerar)
    for(SDL_Point& pad : m_shop_pads) pad = {-1, -1};
    if(!AppConfig::survival_shop) return;
    std::string path = AppConfig::survival_levels_path + AppConfig::survival_maps.at(m_map).first;
    std::vector<SurvivalLayout::Tile> spots = SurvivalLayout::shopSpots(DuelLayout::readMap(path));
    const int t = AppConfig::tile_rect.w;
    for(int index = 0; index < m_player_count; index++)
        for(int pass = 0; pass < 2 && m_shop_pads[index].x < 0; pass++)
        {
            bool want_left = (pass == 0) == (index % 2 == 0);
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
                for(int other = 0; other < index; other++)
                {
                    SDL_Rect shop = {m_shop_pads[other].x, m_shop_pads[other].y, 2 * t, 2 * t};
                    if(hasShop(other) && intersects(shop, area)) free = false;
                }
                if(!free) continue;
                m_shop_pads[index] = {spot.column * t, spot.row * t};
                break;
            }
        }
}

bool Survival::onShop(const Player* player) const
{
    int index = player->playerIndex();
    if(!hasShop(index)) return false;
    SDL_Point pad = m_shop_pads[index];
    return std::abs(player->pos_x - pad.x) <= 8 && std::abs(player->pos_y - pad.y) <= 8;
}

void Survival::updateShop()
{
    std::vector<ShopItem> items = shopItems();
    bool open = shopOpen();
    for(Player* player : m_players)
    {
        int index = player->playerIndex();
        bool shopping = open && player->testFlag(TSF_LIFE) && !player->to_erase && onShop(player);
        bool power = player->takePowerPress();
        bool fire = player->takeFirePress();
        int step = player->takeShopStep();
        // L2 / R2 escolhem qual poder guardado o botão usa (dando a volta), também na loja
        int slot_step = player->takeSlotStep();
        int units = player->storedPowers();
        if(units > 0)
            player->power_selected = ((player->power_selected + slot_step) % units + units) % units;
        else
            player->power_selected = 0;
        player->shop_mode = shopping;
        if(shopping)
        {
            // LB / RB escolhem o item (dando a volta, em silêncio), tiro compra
            int n = static_cast<int>(items.size());
            if(step != 0) m_shop_item[index] = ((m_shop_item[index] + step) % n + n) % n;
            if(fire && !buy(player, items.at(m_shop_item[index] % n)))
                SoundManager::getInstance().playSound("steelhit");
            continue;
        }
        // Fora da loja: o botão de poder usa o poder guardado (sem espaço, continua guardado);
        // o próximo do estoque toma o lugar
        if(power && player->held_power != ST_NONE && player->testFlag(TSF_LIFE))
            useSelected(player);
    }
}

bool Survival::useSelected(Player* player)
{
    // usePower usa o held_power: o escolhido vai para lá só durante o uso. Usado, sai da
    // lista e os outros ficam na ordem em que estavam; sem efeito, nada muda
    std::vector<SpriteType> list = player->units();
    if(list.empty()) return false;
    int selected = std::max(0, std::min(player->power_selected, static_cast<int>(list.size()) - 1));
    player->held_power = list[selected];
    bool used = usePower(player);
    if(used) list.erase(list.begin() + selected);
    player->setUnits(list);
    int left = static_cast<int>(list.size());
    player->power_selected = left == 0 ? 0 : std::min(selected, left - 1);
    return used;
}

bool Survival::buy(Player* player, const ShopItem& item)
{
    if(!cannotBuy(item, player).empty()) return false;
    int cost = price(item, player);
    if(item.type == ST_BONUS_TANK && !callReinforcement(player)) return false;
    // O que vale na hora (reviver, reparo...) já vale aqui: cannotBuy recusou o que não
    // teria efeito
    if(Powers::isExtra(item.type) && !givePower(player, item.type)) return false;
    m_coins_spent[player->playerIndex()] += cost;
    SoundManager::getInstance().playSound("life");
    // Sem pontos: comprar não pode render moedas
    if(item.type == ST_BONUS_STAR) player->changeStarCountBy(1);
    else if(item.type == ST_NONE) player->power_slots++;
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

    // Com os espaços de poder cheios, só pega outro bônus depois de usar o que tem
    if(player->held_power != ST_NONE && player->storedPowers() >= player->power_slots) return;

    SpriteType type = bonus->type;
    if(!Powers::isExtra(type))
    {
        Game::checkCollisionPlayerWithBonus(player, bonus); // os 8 originais, como na campanha
        return;
    }
    // O que vale na hora e não faria nada agora (reparo com a muralha inteira) fica no mapa
    if(!storesPower(type) && !noEffect(type).empty()) return;
    SoundManager::getInstance().playSound("bonus");
    player->score += 300;
    bonus->to_erase = true;
    givePower(player, type);
}

bool Survival::storesPower(SpriteType type)
{
    // Guardado só o que depende de onde e quando é usado (W1); o resto vale na hora, também
    // na compra (a trégua e o escudo, fora da onda, ficam armados para a próxima: applyNow)
    return Powers::storable(type);
}

bool Survival::givePower(Player* player, SpriteType type)
{
    if(!Powers::isExtra(type)) return false;
    if(!storesPower(type)) return applyNow(player, type);
    player->storePower(type);
    // Com os poderes de uso imediato (AppConfig::survival_store_powers = false), vale na
    // hora; se não couber ali (barricada sem espaço...), fica guardado
    if(!AppConfig::survival_store_powers && player->storedPowers() == 1 && usePower(player))
        player->consumeHeldPower();
    return true;
}

std::string Survival::noEffect(SpriteType type) const
{
    switch(type)
    {
    case ST_BONUS_REPAIR:
        // Com a pedra da pá, ou com a muralha inteira, não há o que refazer
        if(m_protect_eagle || baseWallIntact()) return "WALL OK";
        return "";
    case ST_BONUS_TRUCE:
        if(m_phase != PHASE_PLAY && m_truce_armed) return "READY";
        return "";
    case ST_BONUS_TEAM_SHIELD:
        if(m_phase != PHASE_PLAY && m_shield_armed) return "READY";
        return "";
    default:
        return "";
    }
}

bool Survival::applyNow(Player* player, SpriteType type)
{
    if(!noEffect(type).empty()) return false;
    switch(type)
    {
    case ST_BONUS_REVIVE:
        // Um companheiro que caiu volta; ninguém caído: vida extra para quem tem menos
        if(!reviveOne())
        {
            Player* weakest = player;
            for(Player* p : m_players)
                if(!p->to_erase && p->lives_count < weakest->lives_count) weakest = p;
            weakest->addLife();
            SoundManager::getInstance().playSound("life");
        }
        return true;
    case ST_BONUS_REPAIR:
        rebuildBaseWalls();
        SoundManager::getInstance().playSound("bonus");
        return true;
    case ST_BONUS_TRUCE:
        // Fora da onda, os inimigos já não surgem: começa com a próxima
        if(m_phase == PHASE_PLAY) m_truce_time = AppConfig::power_truce_time;
        else m_truce_armed = true;
        SoundManager::getInstance().playSound("bonus");
        return true;
    case ST_BONUS_TEAM_SHIELD:
        // No intervalo o escudo acabaria sem uso: liga quando a próxima onda começa
        if(m_phase == PHASE_PLAY) shieldTeam();
        else m_shield_armed = true;
        SoundManager::getInstance().playSound("bonus");
        return true;
    default:
        return false;
    }
}

void Survival::shieldTeam()
{
    for(Player* p : m_players)
        if(!p->to_erase && p->testFlag(TSF_LIFE)) p->setFlag(TSF_SHIELD); // como o capacete da campanha
}

bool Survival::baseWallIntact() const
{
    for(SDL_Point tile : baseWallTiles())
    {
        const Object* o = m_level.at(tile.y).at(tile.x);
        if(o == nullptr) return false;
        // Tijolo rachado tem o retângulo de colisão menor que o tile
        if(o->type == ST_BRICK_WALL && (o->collision_rect.w < AppConfig::tile_rect.w || o->collision_rect.h < AppConfig::tile_rect.h))
            return false;
    }
    return true;
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

void Survival::onBaseHit(Eagle* base, Bullet* bullet)
{
    // O tiro que passou do alvo e seguiu até a águia: só os inimigos e os jogadores a ferem
    auto fired_by = [bullet](const Tank* tank) {
        return std::find(tank->bullets.begin(), tank->bullets.end(), bullet) != tank->bullets.end();
    };
    for(const Turret* turret : m_turrets)
        if(fired_by(turret)) { bullet->destroy(); return; }
    for(const Bot* ally : m_allies)
        if(fired_by(ally)) { bullet->destroy(); return; }
    Game::onBaseHit(base, bullet);
}

// ======================== Tropa de reforço ========================

SDL_Point Survival::allySpawn() const
{
    // Os pontos dos jogadores, ao lado da base: o primeiro sem tanque nem torreta em cima
    const int t = AppConfig::tile_rect.w;
    std::vector<const Tank*> tanks(m_players.begin(), m_players.end());
    tanks.insert(tanks.end(), m_enemies.begin(), m_enemies.end());
    tanks.insert(tanks.end(), m_allies.begin(), m_allies.end());
    tanks.insert(tanks.end(), m_turrets.begin(), m_turrets.end());
    for(SDL_Point p : AppConfig::player_starting_point)
    {
        SDL_Rect area = {p.x, p.y, 2 * t, 2 * t};
        bool free = true;
        for(const Tank* tank : tanks)
            if(!tank->to_erase && (intersects(tank->collision_rect, area) || intersects(tank->dest_rect, area))) free = false;
        if(free) return p;
    }
    return {-1, -1};
}

bool Survival::callReinforcement(Player* player)
{
    if(static_cast<int>(m_allies.size()) >= AppConfig::survival_reinforce_max) return false;
    SDL_Point spawn = allySpawn();
    if(spawn.x < 0) return false;
    // O tanque A dos inimigos (o mais simples), na cor de quem chamou (C1)
    m_allies.push_back(new Bot(spawn, ST_TANK_A, -1, AppConfig::survival_reinforce_lives, player->color, Bot::ROLE_ATTACK));
    return true;
}

void Survival::steerAllies(Uint32 dt)
{
    for(Bot* ally : m_allies)
        if(!ally->to_erase && ally->testFlag(TSF_LIFE)) hunt(ally, ally->command, dt);
}

// ======================== Rompimento da pedra ========================

std::vector<SDL_Point> Survival::stoneAhead(SDL_Rect r, Direction direction, int* line) const
{
    // A fileira logo à frente do tanque, na largura dele ({coluna, linha})
    const int t = AppConfig::tile_rect.w;
    bool vertical = (direction == D_UP || direction == D_DOWN);
    int first, last;
    // A primeira fileira inteira fora do tanque (o retângulo de colisão fica 2 px para dentro
    // do sprite, então ele quase nunca começa na grade)
    switch(direction)
    {
    case D_UP: *line = r.y / t - 1; break;
    case D_DOWN: *line = (r.y + r.h + t - 1) / t; break;
    case D_LEFT: *line = r.x / t - 1; break;
    default: *line = (r.x + r.w + t - 1) / t; break;
    }
    if(vertical) { first = r.x / t; last = (r.x + r.w - 1) / t; }
    else { first = r.y / t; last = (r.y + r.h - 1) / t; }

    // Só se o tanque está encostado na fileira (a até 2 px dela): quem empurra
    int gap;
    switch(direction)
    {
    case D_UP: gap = r.y - (*line + 1) * t; break;
    case D_DOWN: gap = *line * t - (r.y + r.h); break;
    case D_LEFT: gap = r.x - (*line + 1) * t; break;
    default: gap = *line * t - (r.x + r.w); break;
    }
    std::vector<SDL_Point> stones;
    if(gap > 2 || *line < 0 || r.x < 0 || r.y < 0) return stones;
    std::vector<SurvivalLayout::Tile> wall = SurvivalLayout::baseWallTiles();
    for(int k = first; k <= last; k++)
    {
        int row = vertical ? *line : k, column = vertical ? k : *line;
        if(row < 0 || column < 0 || row >= m_level_rows_count || column >= m_level_columns_count) continue;
        for(const SurvivalLayout::Tile& w : wall)
            if(w.row == row && w.column == column) return {}; // a muralha da águia nunca rompe
        const Object* o = m_level.at(row).at(column);
        if(o != nullptr && o->type == ST_STONE_WALL) stones.push_back({column, row});
    }
    return stones;
}

void Survival::breachWalls(Uint32 dt)
{
    // Uma batida é o começo de um empurrão: o inimigo que continua empurrando a mesma pedra
    // não sorteia de novo a cada quadro
    std::vector<Enemy*> pushing;
    for(Enemy* enemy : m_enemies)
    {
        if(enemy->to_erase || !enemy->testFlag(TSF_LIFE) || enemy->testFlag(TSF_FROZEN) || enemy->speed <= 0) continue;
        int line;
        if(stoneAhead(enemy->collision_rect, enemy->direction, &line).empty()) continue;
        pushing.push_back(enemy);
        if(std::find(m_pushing_stone.begin(), m_pushing_stone.end(), enemy) != m_pushing_stone.end()) continue;
        if(rand() < AppConfig::survival_wall_breach_chance * (static_cast<double>(RAND_MAX) + 1.0)) breach(enemy);
    }
    m_pushing_stone = pushing;
}

void Survival::updateBreachEffects(Uint32 dt)
{
    for(auto& effect : m_breach_effects)
    {
        effect.first->update(dt);
        effect.second += dt;
    }
    // A explosão do tiro: 5 quadros de 40 ms
    m_breach_effects.erase(std::remove_if(m_breach_effects.begin(), m_breach_effects.end(), [](std::pair<Object*, Uint32>& e) {
        if(e.second < 220) return false;
        delete e.first;
        return true;
    }), m_breach_effects.end());
}

void Survival::breach(Enemy* enemy)
{
    // A parede inteira na frente dele (até 4 fileiras de pedra), para ele passar de verdade;
    // a pedra volta na regeneração do mapa, entre as ondas
    const int t = AppConfig::tile_rect.w;
    SDL_Rect probe = enemy->collision_rect; // anda uma fileira por vez, para olhar as seguintes
    bool vertical = (enemy->direction == D_UP || enemy->direction == D_DOWN);
    int sign = (enemy->direction == D_UP || enemy->direction == D_LEFT) ? -1 : 1;
    for(int k = 0; k < 4; k++)
    {
        int line;
        std::vector<SDL_Point> stones = stoneAhead(probe, enemy->direction, &line);
        if(stones.empty()) break;
        for(SDL_Point p : stones)
        {
            delete m_level.at(p.y).at(p.x);
            m_level.at(p.y).at(p.x) = nullptr;
            m_breach_effects.push_back({new Object(p.x * t - 8, p.y * t - 8, ST_DESTROY_BULLET), 0});
        }
        (vertical ? probe.y : probe.x) += sign * t;
    }
    SoundManager::getInstance().playSound("steelhit");
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

    // Torretas e aliados: miram antes do Game atualizar e mover tudo; o inimigo que bate na
    // pedra pode rompê-la (antes das colisões, para ele já passar neste quadro)
    if(!m_pause)
    {
        for(Turret* turret : m_turrets)
            turret->think([&](Direction d) { return clearShot(turret, d); });
        steerAllies(dt);
        if(m_phase == PHASE_PLAY && AppConfig::survival_wall_breach_chance > 0) breachWalls(dt);
        updateBreachEffects(dt);
    }

    int before = m_enemy_to_kill;
    Game::update(dt);
    if(m_pause) return;
    m_phase_time += dt;
    m_destroyed += std::max(0, before - m_enemy_to_kill);
    m_truce_time = m_truce_time > dt ? m_truce_time - dt : 0;

    // Loja e botão de poder (fora da loja, usa o poder guardado; sem espaço, continua guardado)
    updateShop();

    if(m_phase == PHASE_WAVE_INTRO && m_phase_time >= AppConfig::survival_wave_intro_time)
    {
        m_phase = PHASE_PLAY;
        m_phase_time = 0;
        // Trégua e escudo comprados no intervalo começam agora, com a onda
        if(m_truce_armed) m_truce_time = AppConfig::power_truce_time;
        if(m_shield_armed) shieldTeam();
        m_truce_armed = m_shield_armed = false;
        // A contagem chegou a 0: o primeiro inimigo surge já no próximo quadro (o relógio de
        // surgimento corre durante o aviso; isto garante o mesmo com um aviso mais curto)
        m_enemy_redy_time = std::max(m_enemy_redy_time, enemySpawnDelay());
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
    // Com mais de um espaço de poder (comprados na loja), os espaços vão numa linha embaixo
    // do tanque, um por espaço; a lista sobe o que precisar para não chegar nas moedas
    auto extraLine = [&](const Player* player) {
        bool out = std::find(m_killed_players.begin(), m_killed_players.end(), player) != m_killed_players.end();
        return !out && player->power_slots > 1;
    };
    int total = 0;
    for(Player* player : all) total += 18 + (extraLine(player) ? 16 : 0);
    int y = std::min(250, 326 - total);
    for(Player* player : all)
    {
        SDL_Rect dst = {x + 5, y, 16, 16};
        SDL_Rect icon = engine.getSpriteConfig()->getSpriteData(player->type)->rect;
        renderer->drawObjectWithColor(&icon, &dst, player->color);
        SDL_Point p = {dst.x + dst.w + 2, dst.y + 3};
        bool out = std::find(m_killed_players.begin(), m_killed_players.end(), player) != m_killed_players.end();
        renderer->drawText(&p, Engine::intToString(out ? 0 : player->lives_count), BLACK, 3);
        // Poder guardado: o ícone, ou uma moldura vazia
        if(extraLine(player))
        {
            drawPowerSlots(player, {x + 2, dst.y + 17}, 14, 1);
            y += 16;
        }
        else
            drawPowerSlot(out ? nullptr : player, {x + 31, dst.y + 1, 14, 14});
        y += 18;
    }

    // Moedas para a loja: texto preto sobre bloco dourado (V5). Da equipe: o número na fonte
    // média (na pequena ele se confundia com as vidas logo acima); de cada um: uma linha por
    // jogador, com o ícone na cor dele
    if(AppConfig::survival_shop && m_shared_coins)
    {
        SDL_Rect box = {x + 2, 330, w - 4, 40};
        renderer->drawRect(&box, GOLD, true);
        renderer->drawTextCentered(box.x + box.w / 2, box.y + 4, "$", BLACK, 2);
        int value = std::min(teamCoins(), 99999);
        renderer->drawTextCentered(box.x + box.w / 2, box.y + 22, Engine::intToString(value), BLACK, value < 1000 ? 2 : 3);
    }
    else if(AppConfig::survival_shop)
    {
        const int line_h = 12;
        SDL_Rect box = {x + 2, 330, w - 4, 16 + line_h * static_cast<int>(all.size())};
        renderer->drawRect(&box, GOLD, true);
        renderer->drawTextCentered(box.x + box.w / 2, box.y + 3, "$", BLACK, 3);
        int y = box.y + 14;
        for(Player* player : all)
        {
            SDL_Rect dst = {box.x + 2, y, 10, 10};
            SDL_Rect icon = engine.getSpriteConfig()->getSpriteData(player->type)->rect;
            renderer->drawObjectWithColor(&icon, &dst, player->color);
            SDL_Point p = {box.x + 14, y + 1};
            renderer->drawText(&p, Engine::intToString(std::min(coins(player), 9999)), BLACK, 3);
            y += line_h;
        }
    }
}

void Survival::drawFloor()
{
    // Lugar de cada loja no intervalo: quadrado com "$" no chão, na cor do jogador dono (o
    // tanque em cima o cobre)
    if(!shopOpen()) return;
    Renderer* renderer = Engine::getEngine().getRenderer();
    const int size = 2 * AppConfig::tile_rect.w;
    for(int index = 0; index < m_player_count; index++)
    {
        if(!hasShop(index)) continue;
        SDL_Color color = Player::getPlayerColor(index);
        SDL_Color dark = {static_cast<Uint8>(color.r * 3 / 8), static_cast<Uint8>(color.g * 3 / 8), static_cast<Uint8>(color.b * 3 / 8), 255};
        SDL_Rect pad = {m_shop_pads[index].x, m_shop_pads[index].y, size, size};
        renderer->drawRect(&pad, dark, true);
        for(int k = 0; k < 2; k++)
        {
            SDL_Rect border = {pad.x + k, pad.y + k, pad.w - 2 * k, pad.h - 2 * k};
            renderer->drawRect(&border, color, false);
        }
        SDL_Point text = renderer->textSize("$", 2);
        SDL_Point p = {pad.x + (pad.w - text.x) / 2, pad.y + (pad.h - text.y) / 2};
        renderer->drawText(&p, "$", color, 2);
    }
}

SDL_Rect Survival::shopBoxRect(SDL_Point pad, int width, int height) const
{
    const int tank = 2 * AppConfig::tile_rect.w;
    bool left = pad.x + tank / 2 < AppConfig::map_rect.w / 2;
    SDL_Rect box = {left ? pad.x - 3 - width : pad.x + tank + 3, pad.y + tank - height, width, height};
    // Dentro do mapa com a borda (1 px em volta da caixa): sem isso, encostada na beirada, a
    // borda saía da tela ou entrava no painel
    const int edge = 1;
    box.x = std::max(edge, std::min(box.x, AppConfig::map_rect.w - box.w - edge));
    box.y = std::max(edge, std::min(box.y, AppConfig::map_rect.h - box.h - edge));
    return box;
}

void Survival::drawShop(int index)
{
    // Caixa pequena ao lado da loja, do lado de fora (loja à esquerda da base: caixa à
    // esquerda), para não tapar a águia, e só com o dono em cima: o ícone e o nome do item e,
    // embaixo, o preço (dourado se dá para comprar) ou, em vermelho, por que não dá. Sem o
    // dono em cima, nada: o chão na cor dele já diz de quem é. Pouca coisa para não virar
    // bagunça com 4 lojas abertas; as moedas ficam no painel
    if(!shopOpen() || !hasShop(index)) return;
    Renderer* renderer = Engine::getEngine().getRenderer();
    std::vector<ShopItem> items = shopItems();
    const Player* user = nullptr;
    for(const Player* p : m_players) if(p->playerIndex() == index && !p->to_erase && p->shop_mode) user = p;
    if(user == nullptr) return;
    SDL_Color color = user->color;

    struct Line { std::string text; SDL_Color color; };
    std::vector<Line> lines;
    const bool has_icon = true;
    ShopItem item = items.at(m_shop_item[index] % items.size());
    lines.push_back({shopName(item.type), WHITE});
    std::string why = cannotBuy(item, user);
    int cost = price(item, user);
    if(why.empty()) lines.push_back({"$" + Engine::intToString(cost), GOLD});
    else if(why.compare(0, 4, "NEED") == 0) lines.push_back({"$" + Engine::intToString(cost), RED});
    else lines.push_back({why, RED});

    // Primeira linha com o item: "◀ ícone ▶  NOME"
    const int pad_x = 5, pad_y = 5, gap = 4, icon_size = 16, arrow = 5;
    const int icon_w = has_icon ? arrow + 3 + icon_size + 3 + arrow + 6 : 0;
    int width = 0, height = pad_y;
    for(size_t i = 0; i < lines.size(); i++)
    {
        SDL_Point size = renderer->textSize(lines[i].text, 3);
        int line_w = size.x + (i == 0 ? icon_w : 0);
        width = std::max(width, line_w);
        height += std::max(size.y, i == 0 && has_icon ? icon_size : 0) + gap;
    }
    width += 2 * pad_x;
    height += pad_y - gap;

    SDL_Rect box = shopBoxRect(m_shop_pads[index], width, height);
    SDL_Rect border = {box.x - 1, box.y - 1, box.w + 2, box.h + 2};
    renderer->drawRect(&border, color, true);
    renderer->drawRect(&box, BLACK, true);

    int y = box.y + pad_y;
    for(size_t i = 0; i < lines.size(); i++)
    {
        SDL_Point size = renderer->textSize(lines[i].text, 3);
        int x = box.x + pad_x, line_h = size.y;
        if(i == 0 && has_icon)
        {
            line_h = icon_size;
            // Setas de LB (esquerda) e RB (direita) em volta do ícone: a ponta é a coluna de
            // 1 px (k = 0), do lado de fora
            for(int k = 0; k < arrow; k++)
            {
                SDL_Rect l = {x + k, y + icon_size / 2 - k, 1, 2 * k + 1};
                renderer->drawRect(&l, LIGHT_GRAY, true);
            }
            SDL_Rect dst = {x + arrow + 3, y, icon_size, icon_size};
            if(item.type == ST_NONE)
            {
                // Espaço de poder: a moldura vazia do painel, com um "+"
                renderer->drawRect(&dst, LIGHT_GRAY, false);
                SDL_Rect h = {dst.x + 4, dst.y + 7, 8, 2}, v = {dst.x + 7, dst.y + 4, 2, 8};
                renderer->drawRect(&h, GOLD, true);
                renderer->drawRect(&v, GOLD, true);
            }
            else
            {
                SDL_Rect src = Engine::getEngine().getSpriteConfig()->getSpriteData(item.type)->rect;
                renderer->drawObject(&src, &dst);
            }
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
    case ST_BONUS_STAR: return "STAR";
    case ST_BONUS_TANK: return "REINFORCE";
    case ST_NONE: return "+1 SLOT";
    default: return "?";
    }
}

void Survival::drawOverlay()
{
    Renderer* renderer = Engine::getEngine().getRenderer();

    // Pedra rompida: a explosão do tiro em cada bloco que caiu
    for(const auto& effect : m_breach_effects) effect.first->draw();

    // As caixas de quem está comprando por cima das outras
    if(!m_pause)
        for(int pass = 0; pass < 2; pass++)
            for(const Player* p : m_players)
                if(p->shop_mode == (pass == 1)) drawShop(p->playerIndex());

    // Intervalo: onda vencida, recompensas e contagem até a próxima, no alto do mapa (a loja
    // e a base ficam embaixo)
    if(m_phase == PHASE_BREAK && !m_pause)
    {
        std::vector<MessageLine> lines = {{"WAVE " + Engine::intToString(m_wave) + " CLEAR", GOLD, 2, 6}};
        for(int idx : m_revived)
            lines.push_back({"P" + Engine::intToString(idx + 1) + " IS BACK", Player::getPlayerColor(idx), 3, 4});
        if(m_life_reward) lines.push_back({"+1 LIFE", GOLD, 3, 4});
        lines.back().gap = 6;
        lines.push_back({"NEXT WAVE IN " + Engine::intToString(countdown()), WHITE, 3, 0});
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
            {Engine::intToString(m_enemy_to_kill) + " ENEMIES", LIGHT_GRAY, 2, 8},
            // A mesma contagem do intervalo, até o 0 (quando os inimigos começam a surgir)
            {"START IN " + Engine::intToString(countdown()), WHITE, 3, 0},
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
