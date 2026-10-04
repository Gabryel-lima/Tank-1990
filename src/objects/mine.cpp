#include "mine.h"
#include "../appconfig.h"
#include "../soundmanager.h"

Mine::Mine(SDL_Point center, int team, int owner, SDL_Color color)
    : Object(center.x - 8, center.y - 8, ST_MINE), team(team), owner(owner)
{
    this->color = color;
    m_time_left = AppConfig::power_mine_time;
    update(0);
}

void Mine::update(Uint32 dt)
{
    if(to_erase) return;
    Object::update(dt);
    if(dt >= m_time_left) to_erase = true;
    else m_time_left -= dt;
}

void Mine::detonate()
{
    if(to_erase) return;
    SoundManager::getInstance().playSound("shell_exp");
    to_erase = true;
}
