#include "mine.h"
#include "../soundmanager.h"

Mine::Mine(SDL_Point center, int team, int owner, SDL_Color color)
    : Object(center.x - 8, center.y - 8, ST_MINE), team(team), owner(owner)
{
    this->color = color;
    update(0);
}

void Mine::update(Uint32 dt)
{
    if(to_erase) return;
    Object::update(dt);
}

void Mine::detonate()
{
    if(to_erase) return;
    SoundManager::getInstance().playSound("shell_exp");
    to_erase = true;
}
