#include "turret.h"
#include <algorithm>
#include "../appconfig.h"
#include "../soundmanager.h"

Turret::Turret(double x, double y, int team, int owner, SDL_Color color)
    : Tank(x, y, ST_TURRET), owner(owner)
{
    this->team = team;
    this->color = color;
    m_time_left = AppConfig::power_turret_time;
    m_reload_left = 0;
    m_reload = AppConfig::power_turret_reload;
    m_ammo = AppConfig::power_turret_ammo;
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
    if(testFlag(TSF_LIFE))
    {
        m_reload_left = m_reload_left > dt ? m_reload_left - dt : 0;
        if(m_permanent) {}
        else if(dt >= m_time_left) destroy(); // tempo acabou: explode, como um tanque
        else m_time_left -= dt;
    }

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

void Turret::setPermanent(Uint32 reload)
{
    m_permanent = true;
    m_reload = reload;
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

void Turret::drawEffects()
{
    if(!testFlag(TSF_LIFE) || m_permanent) return;
    // Acabando: no último quarto do tempo ou nos últimos 3 tiros; vale o que estiver
    // mais perto do fim
    double warn = AppConfig::power_turret_time / 4.0;
    double by_time = m_time_left < warn ? 1.0 - m_time_left / warn : 0.0;
    double by_ammo = m_ammo <= 3 ? (4 - m_ammo) / 4.0 : 0.0;
    double level = std::max(by_time, by_ammo);
    if(level > 0) drawHaze(std::max(level, 0.15));
}

void Turret::think(const std::function<bool(Direction)>& worth)
{
    if(!testFlag(TSF_LIFE) || testFlag(TSF_FROZEN)) return;
    for(int k = 0; k < 4; k++)
    {
        Direction d = static_cast<Direction>((direction + k) % 4);
        if(!worth(d)) continue;
        direction = d;
        if(m_reload_left == 0 && (m_permanent || m_ammo > 0) && fire() != nullptr)
        {
            SoundManager::getInstance().playSound("shoot");
            m_reload_left = m_reload;
            // Último tiro: some logo depois (o tempo de o projétil seguir e a névoa avisar)
            if(!m_permanent && --m_ammo == 0) m_time_left = std::min<Uint32>(m_time_left, 800);
        }
        return;
    }
}
