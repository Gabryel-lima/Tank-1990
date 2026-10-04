#include "turret.h"
#include "../appconfig.h"
#include "../soundmanager.h"

Turret::Turret(double x, double y, int team, int owner, SDL_Color color)
    : Tank(x, y, ST_TURRET), owner(owner)
{
    this->team = team;
    this->color = color;
    m_time_left = AppConfig::power_turret_time;
    m_reload_left = 0;
    m_bullet_max_size = 1;
    direction = (team == 1 ? D_DOWN : D_UP);
    lives_count = 0;  // não renasce: destruída (ou com o tempo acabado), some
    respawn();        // aparece com a animação de criação, como um tanque
}

void Turret::update(Uint32 dt)
{
    if(to_erase) return;
    speed = 0;
    stop = true;
    Tank::update(dt);
    if(!testFlag(TSF_LIFE)) return;

    // Um quadro só, virado para a direção atual (as direções ficam lado a lado na textura)
    src_rect = moveRect(m_sprite->rect, direction, 0);

    m_reload_left = m_reload_left > dt ? m_reload_left - dt : 0;
    if(dt >= m_time_left) destroy(); // tempo acabou: explode, como um tanque
    else m_time_left -= dt;
}

void Turret::draw()
{
    // Pisca no último quarto do tempo, avisando que vai acabar
    if(testFlag(TSF_LIFE) && m_time_left < AppConfig::power_turret_time / 4 && (m_time_left / 150) % 2)
    {
        for(auto bullet : bullets) bullet->draw();
        return;
    }
    Tank::draw();
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
