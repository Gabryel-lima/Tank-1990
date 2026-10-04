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

    // Recompensa por sobreviver: a muralha volta inteira e, a cada N ondas, vida extra
    if(wave > 1) rebuildBaseWalls();
    m_life_reward = (wave > 1 && (wave - 1) % AppConfig::survival_life_every_waves == 0);
    if(m_life_reward) rewardLives();
}

void Survival::rewardLives()
{
    for(Player* player : m_players) player->addLife();

    // Quem tinha caído volta (uma vida, renascendo no seu ponto)
    for(Player* player : m_killed_players)
    {
        player->to_erase = false;
        player->lives_count = 2; // respawn() gasta uma ao entrar no mapa
        player->respawn();
        m_players.push_back(player);
    }
    m_killed_players.clear();
    SoundManager::getInstance().playSound("life");
}

void Survival::rebuildBaseWalls()
{
    // Pá ativa: a pedra fica até o tempo dela acabar (Game::update devolve os tijolos)
    if(m_protect_eagle) return;

    int rows = m_level_rows_count, t = AppConfig::tile_rect.w;
    std::vector<std::pair<int, int>> tiles;
    for(int i = 1; i <= 3; i++)
    {
        tiles.push_back({rows - i, 11});
        tiles.push_back({rows - i, 14});
    }
    tiles.push_back({rows - 3, 12});
    tiles.push_back({rows - 3, 13});

    std::vector<Tank*> tanks(m_players.begin(), m_players.end());
    tanks.insert(tanks.end(), m_enemies.begin(), m_enemies.end());
    for(auto& tile : tiles)
    {
        int row = tile.first, column = tile.second;
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
    return Powers::draw(Powers::survivalTable());
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
    if(Powers::storable(type))
    {
        SoundManager::getInstance().playSound("bonus");
        player->score += 300;
        bonus->to_erase = true;
        player->held_power = type;
        // Com os poderes de uso imediato (AppConfig::survival_store_powers = false), vale na
        // hora; se não couber ali (barricada sem espaço...), fica guardado
        if(!AppConfig::survival_store_powers && usePower(player)) player->held_power = ST_NONE;
        return;
    }
    if(!Powers::isExtra(type))
    {
        Game::checkCollisionPlayerWithBonus(player, bonus); // os 8 originais, como na campanha
        return;
    }

    SoundManager::getInstance().playSound("bonus");
    player->score += 300;
    bonus->to_erase = true;
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

    // Botão de poder: usa o poder guardado (sem espaço, continua guardado)
    for(Player* player : m_players)
        if(player->takePowerPress() && player->held_power != ST_NONE && player->testFlag(TSF_LIFE) && usePower(player))
            player->held_power = ST_NONE;

    if(m_phase == PHASE_WAVE_INTRO && m_phase_time > AppConfig::survival_wave_intro_time)
    {
        m_phase = PHASE_PLAY;
        m_phase_time = 0;
    }

    // O Game encerra a fase quando os inimigos acabam; aqui isso só encerra a onda
    if(m_finished && !m_game_over)
    {
        m_finished = false;
        startWave(m_wave + 1);
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

    if(ev->type == SDL_KEYDOWN)
    {
        SDL_Keycode key = ev->key.keysym.sym;
        if(key == SDLK_ESCAPE)
            m_finished = true;
        else if(results_ready)
        {
            bool fire = (key == SDLK_RETURN);
            for(auto& keys : AppConfig::keyboard_layouts)
                if(ev->key.keysym.scancode == keys.fire) fire = true;
            if(fire) m_finished = true;
        }
        else if(key == SDLK_RETURN && m_phase != PHASE_RESULTS && !m_game_over)
            m_pause = !m_pause;
    }
    else if(ev->type == SDL_CONTROLLERBUTTONDOWN)
    {
        if(ev->cbutton.button == SDL_CONTROLLER_BUTTON_BACK)
            m_finished = true;
        else if(results_ready && (ev->cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                                  ev->cbutton.button == SDL_CONTROLLER_BUTTON_START))
            m_finished = true;
        else if(ev->cbutton.button == SDL_CONTROLLER_BUTTON_START && m_phase != PHASE_RESULTS && !m_game_over)
            m_pause = !m_pause;
    }
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
        SDL_Rect slot = {x + 31, dst.y + 1, 14, 14};
        if(player->held_power != ST_NONE && !out)
        {
            SDL_Rect power = engine.getSpriteConfig()->getSpriteData(player->held_power)->rect;
            renderer->drawObject(&power, &slot);
        }
        else
            renderer->drawRect(&slot, BLACK, false);
        row++;
    }
}

void Survival::drawOverlay()
{
    Renderer* renderer = Engine::getEngine().getRenderer();

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
        if(m_life_reward)
        {
            lines.back().gap = 8;
            lines.push_back({"+1 LIFE", GOLD, 2, 0});
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
