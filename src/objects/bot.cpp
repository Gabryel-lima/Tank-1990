#include "bot.h"
#include "../appconfig.h"

#include <cstdlib>

Bot::Bot(SDL_Point spawn, SpriteType type, int team, int lives, SDL_Color team_color, Role role)
    : Enemy(spawn.x, spawn.y, type), role(role)
{
    this->team = team;
    spawn_point = spawn;
    color = team_color;
    m_fire_time = 0;
    m_reload_time = 400;
    command.direction = (team == 1 ? D_DOWN : D_UP);
    lives_count = lives + 1; // respawn() gasta uma vida ao colocar o tanque no mapa
    respawn();
}

void Bot::update(Uint32 dt)
{
    if(to_erase) return;
    Tank::update(dt);

    // O Enemy escolhe a cor do sprite pela blindagem; o bot usa sempre a prateada,
    // que fica com a cor da equipe ao ser tingida
    if(testFlag(TSF_LIFE))
        src_rect = moveRect(m_sprite->rect, testFlag(TSF_ON_ICE) ? new_direction : direction, m_current_frame);
    else
        src_rect = moveRect(m_sprite->rect, 0, m_current_frame);

    m_fire_time += dt;
    if(testFlag(TSF_LIFE) && !testFlag(TSF_FROZEN))
    {
        // No gelo o tanque escorrega: a direção nova só vale depois do deslize
        if(command.direction != (testFlag(TSF_ON_ICE) ? new_direction : direction))
            setDirection(command.direction);
        if(command.move) speed = default_speed;
        else if(!testFlag(TSF_ON_ICE)) speed = 0.0;

        if(command.fire && m_fire_time > m_reload_time && fire() != nullptr)
        {
            m_fire_time = 0;
            m_reload_time = 250 + rand() % 300;
        }
    }
    stop = false;
}

void Bot::destroy()
{
    if(testFlag(TSF_SHIELD)) return;
    Tank::destroy();
}

void Bot::respawn()
{
    lives_count--;
    if(lives_count <= 0)
    {
        lives_count = 0;
        if(bullets.empty()) to_erase = true;
        return;
    }

    pos_x = spawn_point.x;
    pos_y = spawn_point.y;
    dest_rect.x = pos_x;
    dest_rect.y = pos_y;

    Tank::respawn();
    direction = (team == 1 ? D_DOWN : D_UP);
    setFlag(TSF_SHIELD);
    m_shield_time = AppConfig::tank_shield_time / 2; // escudo de renascimento: metade do capacete
}
