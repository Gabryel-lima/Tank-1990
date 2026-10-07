#include "mine.h"
#include <algorithm>
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
    if(m_permanent) return;
    if(dt >= m_time_left) to_erase = true;
    else m_time_left -= dt;
}

void Mine::draw()
{
    Object::draw();
    double warn = AppConfig::power_mine_time / 4.0;
    if(!m_permanent && m_time_left < warn) drawHaze(std::max(1.0 - m_time_left / warn, 0.15));
}

void Mine::detonate()
{
    if(to_erase) return;
    SoundManager::getInstance().playSound("shell_exp");
    to_erase = true;
}
