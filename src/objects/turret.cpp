#include "turret.h"
#include "../appconfig.h"
#include "../soundmanager.h"

Turret::Turret(double x, double y, int team, int owner, SDL_Color color)
    : Tank(x, y, ST_TURRET), owner(owner)
{
    this->team = team;
    this->color = color;
    m_reload_left = 0;
    m_bullet_max_size = 1;
    direction = (team == 1 ? D_DOWN : D_UP);
    lives_count = 0;  // não renasce: destruída, some
    respawn();        // aparece com a animação de criação, como um tanque
}

void Turret::update(Uint32 dt)
{
    if(to_erase) return;
    speed = 0;
    stop = true;
    Tank::update(dt);
    if(testFlag(TSF_LIFE)) m_reload_left = m_reload_left > dt ? m_reload_left - dt : 0;

    // Viva: um quadro só, virado para a direção atual (as direções ficam lado a lado na
    // textura). Surgindo ou explodindo: o quadro da animação, como os tanques. Sem isso, a
    // explosão desenhava o último quadro da torreta esticado no retângulo de 64x64 dela
    if(testFlag(TSF_LIFE)) src_rect = moveRect(m_sprite->rect, direction, 0);
    else src_rect = moveRect(m_sprite->rect, 0, m_current_frame);
}

void Turret::destroy()
{
    Tank::destroy();
    // Atingida entre um update e o desenho: sem isso, aquele quadro ainda saía com a torreta
    // esticada no retângulo da explosão
    if(testFlag(TSF_DESTROYED)) src_rect = moveRect(m_sprite->rect, 0, m_current_frame);
}

SDL_Rect Turret::hitRect() const
{
    if(!testFlag(TSF_LIFE)) return {0, 0, 0, 0};
    // O corpo no sprite de 32x32 (ver resources/png/texture.png): 24 px de largura por 20 de
    // profundidade, do lado oposto ao cano (o cano ocupa os 8 px da frente)
    const int x = dest_rect.x, y = dest_rect.y;
    switch(direction)
    {
    case D_UP: return {x + 4, y + 10, 24, 20};
    case D_RIGHT: return {x + 2, y + 4, 20, 24};
    case D_DOWN: return {x + 4, y + 2, 24, 20};
    default: return {x + 10, y + 4, 20, 24};
    }
}

void Turret::think(const std::function<bool(Direction)>& worth)
{
    if(!testFlag(TSF_LIFE) || testFlag(TSF_FROZEN)) return;
    for(int k = 0; k < 4; k++)
    {
        Direction d = static_cast<Direction>((direction + k) % 4);
        if(!worth(d)) continue;
        direction = d;
        if(m_reload_left == 0 && fire() != nullptr)
        {
            SoundManager::getInstance().playSound("shoot");
            m_reload_left = AppConfig::power_turret_reload;
        }
        return;
    }
}
