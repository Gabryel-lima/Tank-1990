#include "game.h"
#include "../engine/engine.h"
#include "../appconfig.h"
#include "../soundmanager.h"
#include "../controllers.h"
#include "menu.h"
#include "scores.h"
#include "message_box.h"

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <cstdlib>

// Construtor padrão do jogo
Game::Game()
{
    m_level_columns_count = 0;
    m_level_rows_count = 0;
    m_current_level = 0;
    m_eagle = nullptr;
    m_player_count = 1;
    m_enemy_redy_time = 0;
    m_pause = false;
    m_level_end_time = 0;
    m_protect_eagle = false;
    m_protect_eagle_time = 0;
    m_enemy_respown_position = 0;
    nextLevel();
}

// Construtor do jogo com quantidade de jogadores
Game::Game(int players_count)
{
    m_level_columns_count = 0;
    m_level_rows_count = 0;
    m_current_level = 0;
    m_eagle = nullptr;
    m_player_count = players_count;
    m_enemy_redy_time = 0;
    m_pause = false;
    m_level_end_time = 0;
    m_protect_eagle = false;
    m_protect_eagle_time = 0;
    m_enemy_respown_position = 0;
    nextLevel();
}

// Construtor do jogo com jogadores existentes e nível anterior
Game::Game(std::vector<Player *> players, int previous_level)
{
    m_level_columns_count = 0;
    m_level_rows_count = 0;
    m_current_level = previous_level;
    m_eagle = nullptr;
    m_players = players;
    m_player_count = m_players.size();
    m_enemy_redy_time = 0;
    for(auto player : m_players)
    {
        player->clearFlag(TSF_MENU);
        player->lives_count++;
        player->respawn();
    }
    m_pause = false;
    m_level_end_time = 0;
    m_protect_eagle = false;
    m_protect_eagle_time = 0;
    m_enemy_respown_position = 0;
    nextLevel();
}

// Construtor para modos derivados: não carrega fase nem cria jogadores
Game::Game(NoCampaign)
{
    m_level_columns_count = 0;
    m_level_rows_count = 0;
    m_current_level = 0;
    m_eagle = nullptr;
    m_player_count = 0;
    m_enemy_to_kill = 0;
    m_enemy_redy_time = 0;
    m_pause = false;
    m_level_end_time = 0;
    m_protect_eagle = false;
    m_protect_eagle_time = 0;
    m_enemy_respown_position = 0;
    m_level_start_screen = false;
    m_level_start_time = 0;
    m_game_over = false;
    m_game_over_hold_time = 0;
    m_game_over_position = 0;
    m_finished = false;
}

// Destrutor do jogo
Game::~Game()
{
    clearLevel();
}

// Desenha todos os elementos do jogo na tela
void Game::draw()
{
    Engine& engine = Engine::getEngine();
    Renderer* renderer = engine.getRenderer();
    renderer->clear();

    if(m_level_start_screen)
    {
        // Como no Battle City original: "STAGE N" em preto sobre a tela cinza
        std::string level_name = "STAGE " + Engine::intToString(m_current_level);
        renderer->drawText(nullptr, level_name, {0, 0, 0, 255}, 1);
    }
    else
    {
        renderer->drawRect(&AppConfig::map_rect, {0, 0, 0, 0}, true);
        for(auto row : m_level)
            for(auto item : row)
                if(item != nullptr) item->draw();

        drawFloor();
        for(auto mine : m_mines) mine->draw();
        for(auto player : m_players) player->draw();
        for(auto enemy : m_enemies) enemy->draw();
        for(auto turret : m_turrets) turret->draw();
        for(auto bush : m_bushes) bush->draw();
        for(auto bonus : m_bonuses) bonus->draw();
        m_eagle->draw();

        // "GAME OVER" sobe até o centro do mapa (centralizado no mapa, não na janela),
        // com contorno para não sumir sobre os tijolos
        if(m_game_over)
        {
            SDL_Point size = renderer->textSize(AppConfig::game_over_text, 1);
            SDL_Point pos = {AppConfig::map_rect.x + (AppConfig::map_rect.w - size.x) / 2, static_cast<int>(m_game_over_position)};
            renderer->drawTextOutlined(pos, AppConfig::game_over_text, {255, 10, 10, 255}, 1);
        }

        drawStatus();

        if(m_pause) drawPause();
        drawOverlay();
    }

    renderer->flush();
}

void Game::drawOverlay()
{
}

// Painel lateral da campanha
void Game::drawStatus()
{
    Engine& engine = Engine::getEngine();
    Renderer* renderer = engine.getRenderer();
    //===========Status do jogo===========
    SDL_Rect src = engine.getSpriteConfig()->getSpriteData(ST_LEFT_ENEMY)->rect;
    SDL_Rect dst;
    SDL_Point p_dst;
    // inimigos restantes para eliminar
    for(int i = 0; i < m_enemy_to_kill; i++)
    {
        dst = {AppConfig::status_rect.x + 8 + src.w * (i % 2), 5 + src.h * (i / 2), src.w, src.h};
        renderer->drawObject(&src, &dst);
    }
    // vidas dos jogadores: ícone fixo do tanque, na cor do jogador (o quadro atual do
    // sprite virava estrela durante o nascimento, e sem a cor P3/P4 pareciam P2/P1)
    int i = 0;
    for(auto player : m_players)
    {
        dst = {AppConfig::status_rect.x + 5, i * 18 + 180, 16, 16};
        p_dst = {dst.x + dst.w + 2, dst.y + 3};
        i++;
        SDL_Rect icon = engine.getSpriteConfig()->getSpriteData(player->type)->rect;
        renderer->drawObjectWithColor(&icon, &dst, player->color);
        renderer->drawText(&p_dst, Engine::intToString(player->lives_count), {0, 0, 0, 255}, 3);
    }
    // número do mapa/nível
    src = engine.getSpriteConfig()->getSpriteData(ST_STAGE_STATUS)->rect;
    dst = {AppConfig::status_rect.x + 8, static_cast<int>(185 + (m_players.size() + m_killed_players.size()) * 18), src.w, src.h};
    p_dst = {dst.x + 10, dst.y + 26};
    renderer->drawObject(&src, &dst);
    renderer->drawText(&p_dst, Engine::intToString(m_current_level), {0, 0, 0, 255}, 2);
}

// Atualiza o estado do jogo
void Game::update(Uint32 dt)
{
    if(dt > 40) return;

    if(m_level_start_screen)
    {
        if(m_level_start_time > AppConfig::level_start_time)
            m_level_start_screen = false;

        m_level_start_time += dt;
    }
    else
    {
        if(m_pause) return;

        std::vector<Player*>::iterator pl1, pl2;
        std::vector<Enemy*>::iterator en1, en2;

        // Verifica colisão entre tanques dos jogadores
        for(pl1 = m_players.begin(); pl1 != m_players.end(); pl1++)
            for(pl2 = pl1 + 1; pl2 != m_players.end(); pl2++)
                checkCollisionTwoTanks(*pl1, *pl2, dt);

        // Verifica colisão entre tanques dos inimigos
        for(en1 = m_enemies.begin(); en1 != m_enemies.end(); en1++)
             for(en2 = en1 + 1; en2 != m_enemies.end(); en2++)
                checkCollisionTwoTanks(*en1, *en2, dt);

        // Verifica colisão de balas dos inimigos com o cenário
        for(auto enemy : m_enemies)
            for(auto bullet : enemy->bullets)
                checkCollisionBulletWithLevel(bullet);
        // Verifica colisão de balas dos jogadores com o cenário e com arbustos
        for(auto player : m_players)
            for(auto bullet : player->bullets)
            {
                checkCollisionBulletWithLevel(bullet);
                checkCollisionBulletWithBush(bullet);
            }

        // Colisões entre jogadores e inimigos
        for(auto player : m_players)
            for(auto enemy : m_enemies)
            {
                // Colisão entre tanque do jogador e do inimigo
                checkCollisionTwoTanks(player, enemy, dt);
                // Colisão entre balas do jogador e inimigo
                checkCollisionPlayerBulletsWithEnemy(player, enemy);

                // Colisão entre balas do jogador e balas do inimigo
                for(auto bullet1 : player->bullets)
                     for(auto bullet2 : enemy->bullets)
                            checkCollisionTwoBullets(bullet1, bullet2);
            }

        // Colisão entre balas do inimigo e jogadores
        for(auto enemy : m_enemies)
            for(auto player : m_players)
                    checkCollisionEnemyBulletsWithPlayer(enemy, player);

        // Minas e torretas dos jogadores (modos extras; a campanha não tem)
        updateFriendlyPowers(dt);

        // Colisão entre jogadores e bônus
        for(auto player : m_players)
            for(auto bonus : m_bonuses)
                checkCollisionPlayerWithBonus(player, bonus);

        // Colisão entre tanques e o cenário
        for(auto enemy : m_enemies) checkCollisionTankWithLevel(enemy, dt);
        for(auto player : m_players) checkCollisionTankWithLevel(player, dt);
        // Jogadores travados numa quina deslizam para o corredor livre
        for(auto player : m_players) tryCornerSlide(player, dt);

        // Definir alvo dos inimigos (jogadores ou águia)
        int min_metric; // 2 * 26 * 16
        int metric;
        SDL_Point target = {-1, -1};
        for(auto enemy : m_enemies)
        {
            min_metric = 832;
            if(enemy->type == ST_TANK_A || enemy->type == ST_TANK_D)
                for(auto player : m_players)
                {
                    metric = std::abs(player->dest_rect.x - enemy->dest_rect.x) + std::abs(player->dest_rect.y - enemy->dest_rect.y);
                    if(metric < min_metric)
                    {
                        min_metric = metric;
                        target = {player->dest_rect.x + player->dest_rect.w / 2, player->dest_rect.y + player->dest_rect.h / 2};
                    }
                }
            metric = std::abs(m_eagle->dest_rect.x - enemy->dest_rect.x) + std::abs(m_eagle->dest_rect.y - enemy->dest_rect.y);
            if(metric < min_metric)
            {
                min_metric = metric;
                target = {m_eagle->dest_rect.x + m_eagle->dest_rect.w / 2, m_eagle->dest_rect.y + m_eagle->dest_rect.h / 2};
            }

            enemy->target_position = target;
        }

        // Atualiza todos os objetos do jogo
        for(auto enemy : m_enemies) enemy->update(dt);
        for(auto player : m_players) player->update(dt);
        for(auto bonus : m_bonuses) bonus->update(dt);
        m_eagle->update(dt);

        for(auto row : m_level)
            for(auto item : row)
                if(item != nullptr) item->update(dt);

        for(auto bush : m_bushes) bush->update(dt);

        // Remove elementos que devem ser apagados
        m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(), [](Enemy*e){if(e->to_erase) {delete e; return true;} return false;}), m_enemies.end());
        m_players.erase(std::remove_if(m_players.begin(), m_players.end(), [this](Player*p){if(p->to_erase) {m_killed_players.push_back(p); return true;} return false;}), m_players.end());
        m_bonuses.erase(std::remove_if(m_bonuses.begin(), m_bonuses.end(), [](Bonus*b){if(b->to_erase) {delete b; return true;} return false;}), m_bonuses.end());
        m_bushes.erase(std::remove_if(m_bushes.begin(), m_bushes.end(), [](Object*b){if(b->to_erase) {delete b; return true;} return false;}), m_bushes.end());

        // Adiciona novo inimigo se necessário
        m_enemy_redy_time += dt;
        int limit = std::min(enemyLimit(), m_enemy_to_kill);
        if(static_cast<int>(m_enemies.size()) < limit && m_enemy_redy_time > enemySpawnDelay())
        {
            m_enemy_redy_time = 0;
            generateEnemy();
        }

        // Verifica se o nível terminou (todos inimigos eliminados)
        if(m_enemies.empty() && m_enemy_to_kill <= 0)
        {
            m_level_end_time += dt;
            if(m_level_end_time > AppConfig::level_end_time)
                m_finished = true;
        }

        // Verifica se todos os jogadores morreram
        if(m_players.empty() && !m_game_over)
        {
            m_eagle->destroy();
            m_game_over_position = AppConfig::map_rect.h;
            m_game_over = true;
        }

        // Animação de game over: como no original, o texto sobe até o centro do mapa,
        // fica parado um instante e só então o jogo segue para a pontuação
        if(m_game_over)
        {
            double center = AppConfig::map_rect.y +
                (AppConfig::map_rect.h - Engine::getEngine().getRenderer()->textSize(AppConfig::game_over_text, 1).y) / 2.0;
            if(m_game_over_position > center)
                m_game_over_position = std::max(center, m_game_over_position - AppConfig::game_over_entry_speed * dt);
            else
            {
                m_game_over_hold_time += dt;
                if(m_game_over_hold_time > AppConfig::game_over_hold_time) m_finished = true;
            }
        }

        // Lógica de proteção temporária da águia
        if(m_protect_eagle)
        {
            m_protect_eagle_time += dt;
            if(m_protect_eagle_time > AppConfig::protect_eagle_time)
            {
                m_protect_eagle = false;
                m_protect_eagle_time = 0;
                // Restaura os tijolos ao redor da águia
                for(int i = 0; i < 3; i++)
                {
                    if(m_level.at(m_level_rows_count - i - 1).at(11) != nullptr)
                        delete m_level.at(m_level_rows_count - i - 1).at(11);
                    m_level.at(m_level_rows_count - i - 1).at(11) = new Brick(11 * AppConfig::tile_rect.w, (m_level_rows_count - i - 1) * AppConfig::tile_rect.h);

                    if(m_level.at(m_level_rows_count - i - 1).at(14) != nullptr)
                        delete m_level.at(m_level_rows_count - i - 1).at(14);
                    m_level.at(m_level_rows_count - i - 1).at(14) = new Brick(14 * AppConfig::tile_rect.w, (m_level_rows_count - i - 1)  * AppConfig::tile_rect.h);
                }
                for(int i = 12; i < 14; i++)
                {
                    if(m_level.at(m_level_rows_count - 3).at(i) != nullptr)
                        delete m_level.at(m_level_rows_count - 3).at(i);
                    m_level.at(m_level_rows_count - 3).at(i) = new Brick(i * AppConfig::tile_rect.w, (m_level_rows_count - 3) * AppConfig::tile_rect.h);
                }
            }

            // Pisca a proteção da águia nos últimos instantes
            if(m_protect_eagle && m_protect_eagle_time > AppConfig::protect_eagle_time / 4 * 3 && m_protect_eagle_time / AppConfig::bonus_blink_time % 2)
            {
                for(int i = 0; i < 3; i++)
                {
                    if(m_level.at(m_level_rows_count - i - 1).at(11) != nullptr)
                        delete m_level.at(m_level_rows_count - i - 1).at(11);
                    m_level.at(m_level_rows_count - i - 1).at(11) = new Brick(11 * AppConfig::tile_rect.w, (m_level_rows_count - i - 1) * AppConfig::tile_rect.h);

                    if(m_level.at(m_level_rows_count - i - 1).at(14) != nullptr)
                        delete m_level.at(m_level_rows_count - i - 1).at(14);
                    m_level.at(m_level_rows_count - i - 1).at(14) = new Brick(14 * AppConfig::tile_rect.w, (m_level_rows_count - i - 1)  * AppConfig::tile_rect.h);
                }
                for(int i = 12; i < 14; i++)
                {
                    if(m_level.at(m_level_rows_count - 3).at(i) != nullptr)
                        delete m_level.at(m_level_rows_count - 3).at(i);
                    m_level.at(m_level_rows_count - 3).at(i) = new Brick(i * AppConfig::tile_rect.w, (m_level_rows_count - 3) * AppConfig::tile_rect.h);
                }
            }
            // Proteção ativa: coloca paredes de pedra ao redor da águia
            else if(m_protect_eagle)
            {
                for(int i = 0; i < 3; i++)
                {
                    if(m_level.at(m_level_rows_count - i - 1).at(11) != nullptr)
                        delete m_level.at(m_level_rows_count - i - 1).at(11);
                    m_level.at(m_level_rows_count - i - 1).at(11) = new Object(11 * AppConfig::tile_rect.w, (m_level_rows_count - i - 1) * AppConfig::tile_rect.h, ST_STONE_WALL);

                    if(m_level.at(m_level_rows_count - i - 1).at(14) != nullptr)
                        delete m_level.at(m_level_rows_count - i - 1).at(14);
                    m_level.at(m_level_rows_count - i - 1).at(14) = new Object(14 * AppConfig::tile_rect.w, (m_level_rows_count - i - 1)  * AppConfig::tile_rect.h, ST_STONE_WALL);
                }
                for(int i = 12; i < 14; i++)
                {
                    if(m_level.at(m_level_rows_count - 3).at(i) != nullptr)
                        delete m_level.at(m_level_rows_count - 3).at(i);
                    m_level.at(m_level_rows_count - 3).at(i) = new Object(i * AppConfig::tile_rect.w, (m_level_rows_count - 3) * AppConfig::tile_rect.h, ST_STONE_WALL);
                }
            }
        }
    }
}

// Processa eventos do teclado
void Game::eventProcess(SDL_Event *ev)
{
    if(ev->type == SDL_KEYDOWN)
    {
        switch(ev->key.keysym.sym)
        {
        case SDLK_n:
            m_enemy_to_kill = 0;
            m_finished = true;
            break;
        case SDLK_b:
            m_enemy_to_kill = 0;
            m_current_level -= 2;
            m_finished = true;
            break;
        case SDLK_t:
            AppConfig::show_enemy_target = !AppConfig::show_enemy_target;
            break;
        case SDLK_RETURN:
            m_pause = !m_pause;
            break;
        case SDLK_ESCAPE:
            m_finished = true;
            break;
        }
    }
    else if(ev->type == SDL_CONTROLLERBUTTONDOWN)
    {
        // Start pausa (como Enter) e Back sai para o menu (como Esc)
        if(ev->cbutton.button == SDL_CONTROLLER_BUTTON_START)
            m_pause = !m_pause;
        else if(ev->cbutton.button == SDL_CONTROLLER_BUTTON_BACK)
            m_finished = true;
    }
}

/*
. = campo vazio
# = parede de tijolo
@ = parede de pedra
% = arbustos
~ = água
- = gelo
*/

// Carrega o nível a partir de um arquivo
void Game::loadLevel(std::string path)
{
    std::fstream level(path, std::ios::in);
    std::string line;
    int j = -1;

    if(level.is_open())
    {
        while(!level.eof())
        {
            std::getline(level, line);
            // Arquivos de fase salvos no Windows terminam em CRLF; sem remover
            // o '\r' o Linux cria uma coluna fantasma a mais no mapa.
            while(!line.empty() && (line.back() == '\r' || line.back() == '\n'))
                line.pop_back();
            if(line.empty()) continue; // ignora a linha em branco no fim do arquivo
            std::vector<Object*> row;
            j++;
            for(unsigned i = 0; i < line.size(); i++)
            {
                Object* obj;
                switch(line.at(i))
                {
                case '#' : obj = new Brick(i * AppConfig::tile_rect.w, j * AppConfig::tile_rect.h); break;
                case '@' : obj = new Object(i * AppConfig::tile_rect.w, j * AppConfig::tile_rect.h, ST_STONE_WALL); break;
                case '%' : m_bushes.push_back(new Object(i * AppConfig::tile_rect.w, j * AppConfig::tile_rect.h, ST_BUSH)); obj =  nullptr; break;
                case '~' : obj = new Object(i * AppConfig::tile_rect.w, j * AppConfig::tile_rect.h, ST_WATER); break;
                case '-' : obj = new Object(i * AppConfig::tile_rect.w, j * AppConfig::tile_rect.h, ST_ICE); break;
                default: obj = nullptr;
                }
                row.push_back(obj);
            }
            m_level.push_back(row);
        }
    }

    m_level_rows_count = m_level.size();
    if(m_level_rows_count)
        m_level_columns_count = m_level.at(0).size();
    else m_level_columns_count = 0;

    // Cria a águia (eagle) no mapa
    m_eagle = new Eagle(12 * AppConfig::tile_rect.w, (m_level_rows_count - 2) * AppConfig::tile_rect.h);

    // Limpa o espaço ao redor da águia
    for(int i = 12; i < 14 && i < m_level_columns_count; i++)
    {
        for(int j = m_level_rows_count - 2; j < m_level_rows_count; j++)
        {
            if(m_level.at(j).at(i) != nullptr)
            {
                delete m_level.at(j).at(i);
                m_level.at(j).at(i) = nullptr;
            }
        }
    }
}

// Retorna se o jogo terminou
bool Game::finished() const
{
    return m_finished || m_quit;
}

// Retorna o próximo estado do jogo (menu ou placar)
AppState* Game::nextState()
{
    if(m_game_over || m_enemy_to_kill <= 0)
    {
        // sound
        SoundManager::getInstance().playSound("game_over");

        m_players.erase(std::remove_if(m_players.begin(), m_players.end(), [this](Player*p){m_killed_players.push_back(p); return true;}), m_players.end());
        Scores* scores = new Scores(m_killed_players, m_current_level, m_game_over);
        return scores;
    }
    Menu* m = new Menu;
    return m;
}

// Limpa todos os elementos do nível atual
void Game::clearLevel()
{
    for(auto enemy : m_enemies) delete enemy;
    m_enemies.clear();

    for(auto player : m_players) delete player;
    m_players.clear();

    for(auto bonus : m_bonuses) delete bonus;
    m_bonuses.clear();

    for(auto mine : m_mines) delete mine;
    m_mines.clear();
    for(auto turret : m_turrets) delete turret;
    m_turrets.clear();

    for(auto row : m_level)
    {
        for(auto item : row) if(item != nullptr) delete item;
        row.clear();
    }
    m_level.clear();

    for(auto bush : m_bushes)  delete bush;
    m_bushes.clear();

    if(m_eagle != nullptr) delete m_eagle;
    m_eagle = nullptr;
}

// Verifica colisão do tanque com o cenário e limites do mapa
void Game::checkCollisionTankWithLevel(Tank* tank, Uint32 dt)
{
    if(tank->to_erase) return;

    int row_start, row_end;
    int column_start, column_end;

    SDL_Rect pr, *lr;
    Object* o;

    //========================colisão com elementos do mapa========================
    row_start = row_end = column_start = column_end = 0;
    switch(tank->direction)
    {
    case D_UP:
        row_end = tank->collision_rect.y / AppConfig::tile_rect.h;
        row_start = row_end - 1;
        column_start = tank->collision_rect.x / AppConfig::tile_rect.w - 1;
        column_end = (tank->collision_rect.x + tank->collision_rect.w) / AppConfig::tile_rect.w + 1;
        break;
    case D_RIGHT:
        column_start = (tank->collision_rect.x + tank->collision_rect.w) / AppConfig::tile_rect.w;
        column_end = column_start + 1;
        row_start = tank->collision_rect.y / AppConfig::tile_rect.h - 1;
        row_end = (tank->collision_rect.y + tank->collision_rect.h) / AppConfig::tile_rect.h + 1;
        break;
    case D_DOWN:
        row_start = (tank->collision_rect.y + tank->collision_rect.h)/ AppConfig::tile_rect.h;
        row_end = row_start + 1;
        column_start = tank->collision_rect.x / AppConfig::tile_rect.w - 1;
        column_end = (tank->collision_rect.x + tank->collision_rect.w) / AppConfig::tile_rect.w + 1;
        break;
    case D_LEFT:
        column_end = tank->collision_rect.x / AppConfig::tile_rect.w;
        column_start = column_end - 1;
        row_start = tank->collision_rect.y / AppConfig::tile_rect.h - 1;
        row_end = (tank->collision_rect.y + tank->collision_rect.h) / AppConfig::tile_rect.h + 1;
        break;
    }
    if(column_start < 0) column_start = 0;
    if(row_start < 0) row_start = 0;
    if(column_end >= m_level_columns_count) column_end = m_level_columns_count - 1;
    if(row_end >= m_level_rows_count) row_end = m_level_rows_count - 1;

    pr = tank->nextCollisionRect(dt);
    SDL_Rect intersect_rect;

    for(int i = row_start; i <= row_end; i++)
        for(int j = column_start; j <= column_end ;j++)
        {
            if(tank->stop) break;
            o = m_level.at(i).at(j);
            if(o == nullptr) continue;
            if(tank->testFlag(TSF_BOAT) && o->type == ST_WATER) continue;

            lr = &o->collision_rect;

            intersect_rect = intersectRect(lr, &pr);
            if(intersect_rect.w > 0 && intersect_rect.h > 0)
            {
                if(o->type == ST_ICE)
                {
                    if(intersect_rect.w > 10 && intersect_rect.h > 10)
                       tank->setFlag(TSF_ON_ICE);
                    continue;
                }
                else
                    tank->collide(intersect_rect);
                break;
            }
        }

    //========================colisão com limites do mapa========================
    SDL_Rect outside_map_rect;
    // retângulo à esquerda do mapa
    outside_map_rect.x = -AppConfig::tile_rect.w;
    outside_map_rect.y = -AppConfig::tile_rect.h;
    outside_map_rect.w = AppConfig::tile_rect.w;
    outside_map_rect.h = AppConfig::map_rect.h + 2 * AppConfig::tile_rect.h;
    intersect_rect = intersectRect(&outside_map_rect, &pr);
    if(intersect_rect.w > 0 && intersect_rect.h > 0)
        tank->collide(intersect_rect);

    // retângulo à direita do mapa
    outside_map_rect.x = AppConfig::map_rect.w;
    outside_map_rect.y = -AppConfig::tile_rect.h;
    outside_map_rect.w = AppConfig::tile_rect.w;
    outside_map_rect.h = AppConfig::map_rect.h + 2 * AppConfig::tile_rect.h;
    intersect_rect = intersectRect(&outside_map_rect, &pr);
    if(intersect_rect.w > 0 && intersect_rect.h > 0)
        tank->collide(intersect_rect);

    // retângulo acima do mapa
    outside_map_rect.x = 0;
    outside_map_rect.y = -AppConfig::tile_rect.h;
    outside_map_rect.w = AppConfig::map_rect.w;
    outside_map_rect.h = AppConfig::tile_rect.h;
    intersect_rect = intersectRect(&outside_map_rect, &pr);
    if(intersect_rect.w > 0 && intersect_rect.h > 0)
        tank->collide(intersect_rect);

    // retângulo abaixo do mapa
    outside_map_rect.x = 0;
    outside_map_rect.y = AppConfig::map_rect.h;
    outside_map_rect.w = AppConfig::map_rect.w;
    outside_map_rect.h = AppConfig::tile_rect.h;
    intersect_rect = intersectRect(&outside_map_rect, &pr);
    if(intersect_rect.w > 0 && intersect_rect.h > 0)
        tank->collide(intersect_rect);

   //========================colisão com as bases========================
    for(Eagle* base : bases())
    {
        intersect_rect = intersectRect(&base->collision_rect, &pr);
        if(intersect_rect.w > 0 && intersect_rect.h > 0)
            tank->collide(intersect_rect);
    }
}

// Verifica colisão entre dois tanques
void Game::checkCollisionTwoTanks(Tank* tank1, Tank* tank2, Uint32 dt)
{
    SDL_Rect cr1 = tank1->nextCollisionRect(dt);
    SDL_Rect cr2 = tank2->nextCollisionRect(dt);
    SDL_Rect intersect_rect = intersectRect(&cr1, &cr2);

    if(intersect_rect.w > 0 && intersect_rect.h > 0)
    {
        tank1->collide(intersect_rect);
        tank2->collide(intersect_rect);
    }
}

// Verifica se a área está livre para o tanque (mapa, cenário, águia e outros tanques)
bool Game::isAreaFreeForTank(SDL_Rect area, Tank* tank, Uint32 dt)
{
    if(area.x < 0 || area.y < 0 ||
       area.x + area.w > AppConfig::map_rect.w || area.y + area.h > AppConfig::map_rect.h)
        return false;

    SDL_Rect intersect_rect;
    int row_start = area.y / AppConfig::tile_rect.h;
    int row_end = std::min((area.y + area.h - 1) / AppConfig::tile_rect.h, m_level_rows_count - 1);
    int column_start = area.x / AppConfig::tile_rect.w;
    int column_end = std::min((area.x + area.w - 1) / AppConfig::tile_rect.w, m_level_columns_count - 1);

    for(int i = row_start; i <= row_end; i++)
        for(int j = column_start; j <= column_end; j++)
        {
            Object* o = m_level.at(i).at(j);
            if(o == nullptr || o->type == ST_ICE) continue;
            if(tank->testFlag(TSF_BOAT) && o->type == ST_WATER) continue;

            intersect_rect = intersectRect(&o->collision_rect, &area);
            if(intersect_rect.w > 0 && intersect_rect.h > 0) return false;
        }

    for(Eagle* base : bases())
    {
        intersect_rect = intersectRect(&base->collision_rect, &area);
        if(intersect_rect.w > 0 && intersect_rect.h > 0) return false;
    }

    // Outros tanques: posição atual e prevista para este frame
    auto blocked_by = [&](Tank* other) {
        if(other == tank || other->to_erase) return false;
        SDL_Rect next_rect = other->nextCollisionRect(dt);
        SDL_Rect r1 = intersectRect(&other->collision_rect, &area);
        SDL_Rect r2 = intersectRect(&next_rect, &area);
        return (r1.w > 0 && r1.h > 0) || (r2.w > 0 && r2.h > 0);
    };
    for(auto player : m_players) if(blocked_by(player)) return false;
    for(auto enemy : m_enemies) if(blocked_by(enemy)) return false;
    for(auto turret : m_turrets) if(blocked_by(turret)) return false;

    return true;
}

// Desliza o jogador para o lado quando ele bate na quina de um obstáculo
void Game::tryCornerSlide(Tank* tank, Uint32 dt)
{
    if(AppConfig::tank_corner_slide_max <= 0) return;
    if(tank->to_erase || !tank->stop || tank->speed == 0) return;
    if(!tank->testFlag(TSF_LIFE) || tank->testFlag(TSF_FROZEN)) return;

    bool vertical = (tank->direction == D_UP || tank->direction == D_DOWN);
    double &lateral = vertical ? tank->pos_x : tank->pos_y;
    int tile = vertical ? AppConfig::tile_rect.w : AppConfig::tile_rect.h;

    // Posições alinhadas à grade de cada lado; testa primeiro a mais próxima
    double before = std::floor(lateral / tile) * tile;
    if(lateral - before < 0.001) return; // já alinhado: o bloqueio é frontal, não uma quina
    double candidates[2] = {before, before + tile};
    if(candidates[1] - lateral < lateral - candidates[0]) std::swap(candidates[0], candidates[1]);

    SDL_Rect current = tank->collision_rect;
    SDL_Rect ahead = tank->nextCollisionRect(dt);
    int inset = vertical ? (tank->dest_rect.w - current.w) / 2 : (tank->dest_rect.h - current.h) / 2;
    int current_lateral = vertical ? current.x : current.y;
    int size = vertical ? current.w : current.h;

    for(double target : candidates)
    {
        double distance = target - lateral;
        if(std::fabs(distance) > AppConfig::tank_corner_slide_max) continue;

        int target_lateral = static_cast<int>(target) + inset;

        // A frente precisa estar livre na posição alinhada...
        SDL_Rect ahead_aligned = ahead;
        (vertical ? ahead_aligned.x : ahead_aligned.y) = target_lateral;
        if(!isAreaFreeForTank(ahead_aligned, tank, dt)) continue;

        // ...e o caminho lateral até lá também
        SDL_Rect sweep = current;
        (vertical ? sweep.x : sweep.y) = std::min(current_lateral, target_lateral);
        (vertical ? sweep.w : sweep.h) = std::abs(target_lateral - current_lateral) + size;
        if(!isAreaFreeForTank(sweep, tank, dt)) continue;

        // Desliza na mesma velocidade do tanque, sem passar do alinhamento
        double step = tank->speed * dt;
        if(std::fabs(distance) <= step) lateral = target;
        else lateral += (distance > 0 ? step : -step);
        return;
    }
}

// Verifica colisão da bala com o cenário e a águia
void Game::checkCollisionBulletWithLevel(Bullet* bullet)
{
    if(bullet == nullptr) return;
    if(bullet->collide) return;

    int row_start, row_end;
    int column_start, column_end;

    SDL_Rect* br, *lr;
    SDL_Rect intersect_rect;
    Object* o;

    //========================colisão com elementos do mapa========================
    row_start = row_end = column_start = column_end = 0;
    switch(bullet->direction)
    {
    case D_UP:
        row_start = row_end = bullet->collision_rect.y / AppConfig::tile_rect.h;
        column_start = bullet->collision_rect.x / AppConfig::tile_rect.w;
        column_end = (bullet->collision_rect.x + bullet->collision_rect.w) / AppConfig::tile_rect.w;
        break;
    case D_RIGHT:
        column_start = column_end = (bullet->collision_rect.x + bullet->collision_rect.w) / AppConfig::tile_rect.w;
        row_start = bullet->collision_rect.y / AppConfig::tile_rect.h;
        row_end = (bullet->collision_rect.y + bullet->collision_rect.h) / AppConfig::tile_rect.h;
        break;
    case D_DOWN:
        row_start = row_end = (bullet->collision_rect.y + bullet->collision_rect.h)/ AppConfig::tile_rect.h;
        column_start = bullet->collision_rect.x / AppConfig::tile_rect.w;
        column_end = (bullet->collision_rect.x + bullet->collision_rect.w) / AppConfig::tile_rect.w;
        break;
    case D_LEFT:
        column_start = column_end = bullet->collision_rect.x / AppConfig::tile_rect.w;
        row_start = bullet->collision_rect.y / AppConfig::tile_rect.h;
        row_end = (bullet->collision_rect.y + bullet->collision_rect.h) / AppConfig::tile_rect.h;
        break;
    }
    if(column_start < 0) column_start = 0;
    if(row_start < 0) row_start = 0;
    if(column_end >= m_level_columns_count) column_end = m_level_columns_count - 1;
    if(row_end >= m_level_rows_count) row_end = m_level_rows_count - 1;

    br = &bullet->collision_rect;

    for(int i = row_start; i <= row_end; i++)
        for(int j = column_start; j <= column_end; j++)
        {
            o = m_level.at(i).at(j);
            if(o == nullptr) continue;
            if(o->type == ST_ICE || o->type == ST_WATER) continue;

            lr = &o->collision_rect;
            intersect_rect = intersectRect(lr, br);

            if(intersect_rect.w > 0 && intersect_rect.h > 0)
            {
                if(!bulletCanDamage(bullet, i, j))
                {
                    // bloco protegido: o projétil some sem causar dano
                }
                else if(breaksBlock(bullet, i, j))
                {
                    delete o;
                    m_level.at(i).at(j) = nullptr;
                }
                else if(o->type == ST_BRICK_WALL)
                {
                    Brick* brick = dynamic_cast<Brick*>(o);
                    brick->bulletHit(bullet->direction);
                    if(brick->to_erase)
                    {
                        delete brick;
                        m_level.at(i).at(j) = nullptr;
                    }
                }
                bullet->destroy();
            }
        }

    //========================colisão com limites do mapa========================
    if(br->x < 0 || br->y < 0 || br->x + br->w > AppConfig::map_rect.w || br->y + br->h > AppConfig::map_rect.h)
    {
        bullet->destroy();
    }
    //========================colisão com as bases========================
    // Projétil que parou num bloco neste mesmo quadro não chega à base: se ele encosta ao
    // mesmo tempo na pedra da frente e na águia (metade em cada), a pedra protege
    if(bullet->collide) return;
    for(Eagle* base : bases())
    {
        if(base->type != ST_EAGLE) continue; // base já destruída
        intersect_rect = intersectRect(&base->collision_rect, br);
        if(intersect_rect.w > 0 && intersect_rect.h > 0)
            onBaseHit(base, bullet);
    }
}

// Bases do mapa: na campanha, só a águia
std::vector<Eagle*> Game::bases()
{
    return {m_eagle};
}

// Campanha: projétil na águia encerra o jogo
void Game::onBaseHit(Eagle* base, Bullet* bullet)
{
    if(m_game_over) return;
    bullet->destroy();
    base->destroy();
    m_game_over_position = AppConfig::map_rect.h;
    m_game_over = true;
}

// Campanha: todo bloco pode ser danificado
bool Game::bulletCanDamage(Bullet*, int, int)
{
    return true;
}

// Campanha: o projétil reforçado (3 estrelas) destrói qualquer bloco em qualquer lugar
bool Game::powerAppliesAt(Bullet*, int, int)
{
    return true;
}

void Game::drawPause()
{
    // "PAUSE" piscando no centro do mapa, como no original; contorno para não sumir
    // sobre os tijolos (o vermelho direto sobre tijolo vermelho desaparecia)
    if((SDL_GetTicks() / AppConfig::pause_blink_time) % 2 != 0) return;
    Renderer* renderer = Engine::getEngine().getRenderer();
    SDL_Point size = renderer->textSize("PAUSE", 1);
    SDL_Point pos = {AppConfig::map_rect.x + (AppConfig::map_rect.w - size.x) / 2,
                     AppConfig::map_rect.y + (AppConfig::map_rect.h - size.y) / 2};
    renderer->drawTextOutlined(pos, "PAUSE", {255, 70, 70, 255}, 1);
}

void Game::drawPauseBox()
{
    const SDL_Color pause_red = {255, 70, 70, 255};   // 6,2:1 sobre preto
    const SDL_Color hint = {150, 150, 150, 255};
    drawMessageBox(Engine::getEngine().getRenderer(), {
        {"PAUSE", pause_red, 1, 10},
        {"ENTER / START: PLAY", hint, 3, 6},
        {"ESC / SELECT: MENU", hint, 3, 0},
    }, pause_red);
}

void Game::extraModeInput(SDL_Event* ev, bool results_ready, bool can_pause)
{
    if(ev->type == SDL_KEYDOWN)
    {
        SDL_Keycode key = ev->key.keysym.sym;
        // Esc só sai com o jogo pausado (a caixa da pausa diz): sem querer, no meio da
        // partida, não perde nada
        if(key == SDLK_ESCAPE)
        {
            if(m_pause) m_quit = true;
        }
        else if(results_ready)
        {
            // Qualquer tecla de tiro ou Enter volta ao menu
            bool fire = (key == SDLK_RETURN);
            for(auto& keys : AppConfig::keyboard_layouts)
                if(ev->key.keysym.scancode == keys.fire) fire = true;
            if(fire) m_finished = true;
        }
        else if(key == SDLK_RETURN && can_pause)
            m_pause = !m_pause;
    }
    else if(ev->type == SDL_CONTROLLERBUTTONDOWN)
    {
        // Select, como o Esc: só na pausa
        if(ev->cbutton.button == SDL_CONTROLLER_BUTTON_BACK)
        {
            if(m_pause) m_quit = true;
        }
        else if(results_ready && (ev->cbutton.button == SDL_CONTROLLER_BUTTON_A ||
                                  ev->cbutton.button == SDL_CONTROLLER_BUTTON_START))
            m_finished = true;
        else if(ev->cbutton.button == SDL_CONTROLLER_BUTTON_START && can_pause)
            m_pause = !m_pause;
    }
}

void Game::drawPowerSlot(const Player* player, const SDL_Rect& slot)
{
    Engine& engine = Engine::getEngine();
    if(player != nullptr && player->held_power != ST_NONE)
    {
        SDL_Rect icon = engine.getSpriteConfig()->getSpriteData(player->held_power)->rect;
        engine.getRenderer()->drawObject(&icon, &slot);
    }
    else if(player != nullptr && player->boosted())
    {
        // Turbo em uso: o ícone fica no espaço enquanto dura, com a névoa no fim (V1).
        // Sem isso, o espaço esvaziava ao usar e nada mostrava que o turbo estava valendo
        SDL_Rect icon = engine.getSpriteConfig()->getSpriteData(ST_BONUS_TURBO)->rect;
        engine.getRenderer()->drawObject(&icon, &slot);
        engine.getRenderer()->drawWhite(&icon, &slot, Object::hazeAlpha(player->turboEnding(), SDL_GetTicks()));
    }
    else
        engine.getRenderer()->drawRect(&slot, {0, 0, 0, 255}, false);
}

bool Game::breaksBlock(Bullet* bullet, int row, int column)
{
    return bullet->increased_damage && powerAppliesAt(bullet, row, column);
}

// Verifica colisão da bala com arbustos (só se for bala forte)
void Game::checkCollisionBulletWithBush(Bullet *bullet)
{
    if(bullet == nullptr) return;
    if(bullet->collide) return;
    if(!bullet->increased_damage) return;

    SDL_Rect* br, *lr;
    SDL_Rect intersect_rect;
    br = &bullet->collision_rect;

    for(auto bush : m_bushes)
    {
        if(bush->to_erase) continue;
        lr = &bush->collision_rect;
        intersect_rect = intersectRect(lr, br);

        if(intersect_rect.w > 0 && intersect_rect.h > 0)
        {
            bullet->destroy();
            bush->to_erase = true;
        }
    }
}

// Verifica colisão das balas do jogador com o inimigo
void Game::checkCollisionPlayerBulletsWithEnemy(Tank *shooter, Enemy *enemy)
{
    if(shooter->to_erase || enemy->to_erase) return;
    Player* player = dynamic_cast<Player*>(shooter); // os pontos vão para o jogador (torreta não pontua)
    if(enemy->testFlag(TSF_DESTROYED)) return;
    SDL_Rect intersect_rect;

    for(auto bullet : shooter->bullets)
    {
        if(!bullet->to_erase && !bullet->collide)
        {
            intersect_rect = intersectRect(&bullet->collision_rect, &enemy->collision_rect);
            if(intersect_rect.w > 0 && intersect_rect.h > 0)
            {
                // Como no original: o tanque vermelho solta o bônus no primeiro acerto e deixa
                // de carregá-lo (antes, um tanque blindado soltava um bônus a cada tiro)
                if(enemy->testFlag(TSF_BONUS))
                {
                    generateBonus();
                    enemy->clearFlag(TSF_BONUS);
                }

                bullet->destroy();
                enemy->destroy();
                if(enemy->lives_count <= 0) m_enemy_to_kill--;
                if(player != nullptr) player->score += enemy->scoreForHit();
            }
        }
    }
}

// Verifica colisão das balas do inimigo com o jogador
void Game::checkCollisionEnemyBulletsWithPlayer(Enemy *enemy, Player *player)
{
    if(enemy->to_erase || player->to_erase) return;
    if(player->testFlag(TSF_DESTROYED)) return;
    SDL_Rect intersect_rect;

    for(auto bullet : enemy->bullets)
    {
        if(!bullet->to_erase && !bullet->collide)
        {
            intersect_rect = intersectRect(&bullet->collision_rect, &player->collision_rect);
            if(intersect_rect.w > 0 && intersect_rect.h > 0)
            {
                bullet->destroy();
                player->destroy();
            }
        }
    }
}

// Verifica colisão entre duas balas
void Game::checkCollisionTwoBullets(Bullet *bullet1, Bullet *bullet2)
{
    if(bullet1 == nullptr || bullet2 == nullptr) return;
    if(bullet1->to_erase || bullet2->to_erase) return;

    SDL_Rect intersect_rect = intersectRect(&bullet1->collision_rect, &bullet2->collision_rect);

    if(intersect_rect.w > 0 && intersect_rect.h > 0)
    {
        bullet1->destroy();
        bullet2->destroy();
    }
}

// Verifica colisão do jogador com bônus e aplica efeito
void Game::checkCollisionPlayerWithBonus(Player *player, Bonus *bonus)
{
    if(player->to_erase || bonus->to_erase) return;

    SDL_Rect intersect_rect = intersectRect(&player->collision_rect, &bonus->collision_rect);
    if(intersect_rect.w > 0 && intersect_rect.h > 0)
    {

        // sound
        SoundManager::getInstance().playSound("bonus");

        player->score += 300;

        if(bonus->type == ST_BONUS_GRENADE)
        {
            // Elimina todos os inimigos
            for(auto enemy : m_enemies)
            {
                if(!enemy->to_erase)
                {
                    player->score += 200;
                    while(enemy->lives_count > 0) enemy->destroy();
                    m_enemy_to_kill--;
                }
            }
        }
        else if(bonus->type == ST_BONUS_HELMET)
        {
            // Dá escudo ao jogador
            player->setFlag(TSF_SHIELD);
        }
        else if(bonus->type == ST_BONUS_CLOCK)
        {
            // Congela todos os inimigos
            for(auto enemy : m_enemies) if(!enemy->to_erase) enemy->setFlag(TSF_FROZEN);
        }
        else if(bonus->type == ST_BONUS_SHOVEL)
        {
            // Protege a águia com paredes de pedra
            m_protect_eagle = true;
            m_protect_eagle_time = 0;
            for(int i = 0; i < 3; i++)
            {
                if(m_level.at(m_level_rows_count - i - 1).at(11) != nullptr)
                    delete m_level.at(m_level_rows_count - i - 1).at(11);
                m_level.at(m_level_rows_count - i - 1).at(11) = new Object(11 * AppConfig::tile_rect.w, (m_level_rows_count - i - 1) * AppConfig::tile_rect.h, ST_STONE_WALL);

                if(m_level.at(m_level_rows_count - i - 1).at(14) != nullptr)
                    delete m_level.at(m_level_rows_count - i - 1).at(14);
                m_level.at(m_level_rows_count - i - 1).at(14) = new Object(14 * AppConfig::tile_rect.w, (m_level_rows_count - i - 1)  * AppConfig::tile_rect.h, ST_STONE_WALL);
            }
            for(int i = 12; i < 14; i++)
            {
                if(m_level.at(m_level_rows_count - 3).at(i) != nullptr)
                    delete m_level.at(m_level_rows_count - 3).at(i);
                m_level.at(m_level_rows_count - 3).at(i) = new Object(i * AppConfig::tile_rect.w, (m_level_rows_count - 3) * AppConfig::tile_rect.h, ST_STONE_WALL);
            }
        }
        else if(bonus->type == ST_BONUS_TANK)
        {
            // Dá uma vida extra ao jogador
            player->lives_count++;
        }
        else if(bonus->type == ST_BONUS_STAR)
        {
            // Aumenta o nível de estrela do jogador
            player->changeStarCountBy(1);
        }
        else if(bonus->type == ST_BONUS_GUN)
        {
            // Aumenta o nível de estrela do jogador em 3
            player->changeStarCountBy(3);
        }
        else if(bonus->type == ST_BONUS_BOAT)
        {
            // Permite atravessar água
            player->setFlag(TSF_BOAT);
        }
        bonus->to_erase = true;
    }
}

// Avança para o próximo nível
void Game::nextLevel()
{
    // sound
    SoundManager::getInstance().playSound("level_starting");

    m_current_level++;
    if(m_current_level > 35) m_current_level = 1;
    if(m_current_level < 0) m_current_level = 35;

    m_level_start_screen = true;
    m_level_start_time = 0;
    m_game_over = false;
    m_game_over_hold_time = 0;
    m_finished = false;
    m_enemy_to_kill = AppConfig::enemy_start_count;

    std::string level_path = AppConfig::levels_path + Engine::intToString(m_current_level);
    loadLevel(level_path);

    // Cria jogadores se necessário
    if(m_players.empty())
    {
        for(int i = 0; i < m_player_count; i++)
            m_players.push_back(new Player(i));
    }

    // Redistribui os controles conforme a quantidade de jogadores da partida
    Controllers::setPlayerCount(m_player_count);
}

int Game::enemyLimit() const
{
    return AppConfig::enemy_max_count_on_map;
}

Uint32 Game::enemySpawnDelay() const
{
    return AppConfig::enemy_redy_time;
}

// Gera um novo inimigo no mapa
void Game::generateEnemy()
{
    SDL_Point point = AppConfig::enemy_starting_point.at(m_enemy_respown_position);
    m_enemy_respown_position++;
    if(m_enemy_respown_position >= static_cast<int>(AppConfig::enemy_starting_point.size())) m_enemy_respown_position = 0;
    m_enemies.push_back(createEnemy(point, m_current_level));
}

Enemy* Game::createEnemy(SDL_Point point, int level)
{
    float p = static_cast<float>(rand()) / RAND_MAX;
    SpriteType type = static_cast<SpriteType>(p < (0.00735 * level + 0.09265) ? ST_TANK_D : rand() % (ST_TANK_C - ST_TANK_A + 1) + ST_TANK_A);
    Enemy* e = new Enemy(point.x, point.y, type);

    double a, b, c;
    if(level <= 17)
    {
        a = -0.040625 * level + 0.940625;
        b = -0.028125 * level + 0.978125;
        c = -0.014375 * level + 0.994375;
    }
    else
    {
        a = -0.012778 * level + 0.467222;
        b = -0.025000 * level + 0.925000;
        c = -0.036111 * level + 1.363889;
    }

    p = static_cast<float>(rand()) / RAND_MAX;
    if(p < a) e->lives_count = 1;
    else if(p < b) e->lives_count = 2;
    else if(p < c) e->lives_count = 3;
    else e->lives_count = 4;

    p = static_cast<float>(rand()) / RAND_MAX;
    if(p < 0.12) e->setFlag(TSF_BONUS);
    return e;
}

// Gera um bônus aleatório no mapa, evitando sobreposição com a águia
void Game::generateBonus()
{
    // Como no original, só um bônus fica no mapa: o novo substitui o anterior
    for(auto bonus : m_bonuses) delete bonus;
    m_bonuses.clear();

    Bonus* b = new Bonus(0, 0, randomBonusType());
    SDL_Rect intersect_rect;
    do
    {
        b->pos_x = rand() % (AppConfig::map_rect.x + AppConfig::map_rect.w - 1 *  AppConfig::tile_rect.w);
        b->pos_y = rand() % (AppConfig::map_rect.y + AppConfig::map_rect.h - 1 * AppConfig::tile_rect.h);
        b->update(0);
        intersect_rect = intersectRect(&b->collision_rect, &m_eagle->collision_rect);
    }while(intersect_rect.w > 0 && intersect_rect.h > 0);

    m_bonuses.push_back(b);
}

// Campanha: um dos 8 bônus originais, com a mesma chance
SpriteType Game::randomBonusType()
{
    return static_cast<SpriteType>(rand() % (ST_BONUS_BOAT - ST_BONUS_GRENADE + 1) + ST_BONUS_GRENADE);
}

// ======================== Poderes dos modos extras ========================

bool Game::reservedTile(int, int)
{
    return false;
}

bool Game::areaFree(int row, int column, int rows, int columns)
{
    const int t = AppConfig::tile_rect.w;
    if(row < 0 || column < 0 || row + rows > m_level_rows_count || column + columns > m_level_columns_count) return false;
    for(int r = row; r < row + rows; r++)
        for(int c = column; c < column + columns; c++)
            if(m_level.at(r).at(c) != nullptr || reservedTile(r, c)) return false;

    SDL_Rect area = {column * t, row * t, columns * t, rows * t};
    auto overlaps = [&](SDL_Rect r) {
        SDL_Rect i = intersectRect(&r, &area);
        return i.w > 0 && i.h > 0;
    };
    for(Eagle* base : bases())
        if(overlaps(base->collision_rect)) return false;
    // nada de parede ou torreta escondida debaixo do mato
    for(auto bush : m_bushes)
        if(overlaps(bush->collision_rect)) return false;
    for(auto player : m_players)
        if(!player->to_erase && (overlaps(player->dest_rect) || overlaps(player->collision_rect))) return false;
    for(auto enemy : m_enemies)
        if(!enemy->to_erase && (overlaps(enemy->dest_rect) || overlaps(enemy->collision_rect))) return false;
    for(auto turret : m_turrets)
        if(!turret->to_erase && overlaps(turret->dest_rect)) return false;
    return true;
}

void Game::frontCell(Tank* tank, int* row, int* column) const
{
    // Na direção do tanque, a borda vai para fora dele: o tanque anda de 8 em 8 px e para
    // muito no meio de um bloco (y = 200 = 12,5 blocos); arredondando, a área "da frente"
    // de quem olha para cima ou para a esquerda cobria o próprio tanque, e a barricada e a
    // torreta eram recusadas por falta de espaço. Na outra direção, o arredondamento centra
    const double t = AppConfig::tile_rect.w;
    int r = static_cast<int>(std::lround(tank->pos_y / t));
    int c = static_cast<int>(std::lround(tank->pos_x / t));
    switch(tank->direction)
    {
    case D_UP: r = static_cast<int>(std::floor(tank->pos_y / t)) - 2; break;
    case D_DOWN: r = static_cast<int>(std::ceil(tank->pos_y / t)) + 2; break;
    case D_LEFT: c = static_cast<int>(std::floor(tank->pos_x / t)) - 2; break;
    default: c = static_cast<int>(std::ceil(tank->pos_x / t)) + 2; break;
    }
    *row = r;
    *column = c;
}

bool Game::placeBarricade(Tank* tank)
{
    int row, column;
    frontCell(tank, &row, &column);
    if(!areaFree(row, column, 2, 2)) return false;
    const int t = AppConfig::tile_rect.w;
    for(int r = row; r < row + 2; r++)
        for(int c = column; c < column + 2; c++)
            m_level.at(r).at(c) = new Brick(c * t, r * t);
    SoundManager::getInstance().playSound("bonus");
    return true;
}

void Game::restoreTerrain(const std::vector<std::string>& grid, const std::vector<SDL_Point>& skip)
{
    const int t = AppConfig::tile_rect.w;
    std::vector<SDL_Rect> busy;
    for(Player* p : m_players) if(!p->to_erase) busy.push_back(p->collision_rect);
    for(Enemy* e : m_enemies) if(!e->to_erase) busy.push_back(e->collision_rect);
    for(Turret* turret : m_turrets) if(!turret->to_erase) busy.push_back(turret->collision_rect);
    for(Mine* mine : m_mines) if(!mine->to_erase) busy.push_back(mine->collision_rect);
    for(Bonus* bonus : m_bonuses) if(!bonus->to_erase) busy.push_back(bonus->collision_rect);

    for(int row = 0; row < m_level_rows_count && row < static_cast<int>(grid.size()); row++)
        for(int column = 0; column < m_level_columns_count && column < static_cast<int>(grid[row].size()); column++)
        {
            if(std::any_of(skip.begin(), skip.end(), [&](SDL_Point p) { return p.x == column && p.y == row; }))
                continue;
            char symbol = grid[row][column];
            if(symbol == '%')
            {
                // Arbusto não bloqueia ninguém: volta mesmo com alguém em cima
                bool there = std::any_of(m_bushes.begin(), m_bushes.end(), [&](Object* b) {
                    return !b->to_erase && b->pos_x == column * t && b->pos_y == row * t; });
                if(!there) m_bushes.push_back(new Object(column * t, row * t, ST_BUSH));
                continue;
            }
            if(symbol != '#' && symbol != '@') continue; // água e gelo não são destruídos

            Object*& cell = m_level.at(row).at(column);
            bool brick = (symbol == '#');
            // Pedra que está lá fica; tijolo é trocado por um inteiro (pode estar rachado)
            if(cell != nullptr && !cell->to_erase && (!brick || cell->type != ST_BRICK_WALL)) continue;
            SDL_Rect area = {column * t, row * t, t, t};
            if(std::any_of(busy.begin(), busy.end(), [&](const SDL_Rect& r) { return SDL_HasIntersection(&r, &area); }))
                continue;
            delete cell;
            cell = brick ? static_cast<Object*>(new Brick(column * t, row * t))
                         : new Object(column * t, row * t, ST_STONE_WALL);
        }
}

bool Game::placeTurret(Player* player)
{
    int row, column;
    frontCell(player, &row, &column);
    if(!areaFree(row, column, 2, 2)) return false;
    const int t = AppConfig::tile_rect.w;
    Turret* turret = new Turret(column * t, row * t, player->team, player->playerIndex(), player->color);
    turret->direction = player->direction;
    m_turrets.push_back(turret);
    SoundManager::getInstance().playSound("bonus");
    return true;
}

void Game::placeMine(Player* player)
{
    SDL_Point center = {static_cast<int>(player->pos_x) + player->dest_rect.w / 2, static_cast<int>(player->pos_y) + player->dest_rect.h / 2};
    m_mines.push_back(new Mine(center, player->team, player->playerIndex(), player->color));
    SoundManager::getInstance().playSound("bonus");
}

bool Game::recall(Player* player, const std::vector<SDL_Point>& points)
{
    const int t = AppConfig::tile_rect.w;
    for(SDL_Point p : points)
    {
        SDL_Rect area = {p.x, p.y, 2 * t, 2 * t};
        bool occupied = false;
        auto check = [&](Tank* other) {
            if(other == player || other->to_erase) return;
            SDL_Rect a = intersectRect(&other->dest_rect, &area), b = intersectRect(&other->collision_rect, &area);
            if((a.w > 0 && a.h > 0) || (b.w > 0 && b.h > 0)) occupied = true;
        };
        for(auto other : m_players) check(other);
        for(auto other : m_enemies) check(other);
        for(auto other : m_turrets) check(other);
        if(occupied) continue;
        player->teleport(p.x, p.y);
        return true;
    }
    return false;
}

void Game::killEnemy(Enemy* enemy, Player* by)
{
    if(enemy->to_erase || !enemy->testFlag(TSF_LIFE)) return;
    if(enemy->testFlag(TSF_BONUS))
    {
        generateBonus();
        enemy->clearFlag(TSF_BONUS);
    }
    if(by != nullptr) by->score += enemy->scoreForHit();
    while(enemy->lives_count > 0) enemy->destroy();
    m_enemy_to_kill--;
}

void Game::updateFriendlyPowers(Uint32 dt)
{
    if(m_mines.empty() && m_turrets.empty()) return;
    auto overlap = [](const SDL_Rect& a, const SDL_Rect& b, int min) {
        SDL_Rect r = intersectRect(const_cast<SDL_Rect*>(&a), const_cast<SDL_Rect*>(&b));
        return r.w >= min && r.h >= min;
    };
    auto playerByIndex = [&](int index) -> Player* {
        for(auto p : m_players) if(p->playerIndex() == index) return p;
        return nullptr;
    };

    // Torretas: colidem com os tanques, atiram nos inimigos e levam tiro deles
    for(auto turret : m_turrets)
    {
        if(turret->to_erase) continue;
        for(auto player : m_players) checkCollisionTwoTanks(player, turret, dt);
        for(auto enemy : m_enemies)
        {
            checkCollisionTwoTanks(enemy, turret, dt);
            checkCollisionPlayerBulletsWithEnemy(turret, enemy);
            for(auto b1 : turret->bullets)
                for(auto b2 : enemy->bullets)
                    checkCollisionTwoBullets(b1, b2);
            for(auto bullet : enemy->bullets)
                if(!bullet->to_erase && !bullet->collide && turret->testFlag(TSF_LIFE) &&
                   overlap(bullet->collision_rect, turret->collision_rect, 1))
                {
                    bullet->destroy();
                    turret->destroy();
                }
        }
        for(auto bullet : turret->bullets) checkCollisionBulletWithLevel(bullet);
    }

    // Minas: explodem o inimigo que passar por cima (de vez, como a granada) e somem com
    // qualquer tiro (dá para limpar o caminho atirando nelas)
    for(auto mine : m_mines)
    {
        if(mine->to_erase) continue;
        for(auto enemy : m_enemies)
            if(enemy->testFlag(TSF_LIFE) && overlap(mine->collision_rect, enemy->collision_rect, 6))
            {
                killEnemy(enemy, playerByIndex(mine->owner));
                mine->detonate();
                break;
            }
        if(mine->to_erase) continue;
        std::vector<Tank*> shooters(m_players.begin(), m_players.end());
        shooters.insert(shooters.end(), m_enemies.begin(), m_enemies.end());
        shooters.insert(shooters.end(), m_turrets.begin(), m_turrets.end());
        for(Tank* shooter : shooters)
            for(auto bullet : shooter->bullets)
                if(!mine->to_erase && !bullet->to_erase && !bullet->collide && overlap(bullet->collision_rect, mine->collision_rect, 1))
                {
                    bullet->destroy();
                    mine->detonate();
                }
    }

    for(auto mine : m_mines) mine->update(dt);
    for(auto turret : m_turrets) turret->update(dt);
    m_mines.erase(std::remove_if(m_mines.begin(), m_mines.end(), [](Mine* m){ if(m->to_erase) { delete m; return true; } return false; }), m_mines.end());
    m_turrets.erase(std::remove_if(m_turrets.begin(), m_turrets.end(), [](Turret* t){ if(t->to_erase) { delete t; return true; } return false; }), m_turrets.end());
}
