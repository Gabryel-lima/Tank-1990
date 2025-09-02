#include "object.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <algorithm>

// Construtor padrão: inicializa o objeto com valores padrão
Object::Object()
{
    pos_x = 0.0;
    pos_y = 0.0;
    type = ST_NONE;
    to_erase = false;
    m_sprite = nullptr;
    m_frame_display_time = 0;
    m_current_frame = 0;
    color = {255, 255, 255, 255}; // Cor padrão (branco - não altera a cor original)
}

// Construtor: inicializa o objeto em (x, y) com o tipo de sprite fornecido
Object::Object(double x, double y, SpriteType type)
{
    pos_x = x;
    pos_y = y;
    this->type = type;
    to_erase = false;
    m_sprite = Engine::getEngine().getSpriteConfig()->getSpriteData(type);
    m_frame_display_time = 0;
    m_current_frame = 0;
    color = {255, 255, 255, 255}; // Cor padrão (branco - não altera a cor original)

    // Inicializa o retângulo de origem do sprite
    src_rect.x = m_sprite->rect.x;
    src_rect.y = m_sprite->rect.y;
    src_rect.h = m_sprite->rect.h;
    src_rect.w = m_sprite->rect.w;

    // Inicializa o retângulo de destino (posição na tela)
    dest_rect.x = pos_x;
    dest_rect.y = pos_y;
    dest_rect.h = m_sprite->rect.h;
    dest_rect.w = m_sprite->rect.w;

    // Inicializa o retângulo de colisão
    collision_rect.x = pos_x;
    collision_rect.y = pos_y;
    collision_rect.h = m_sprite->rect.h;
    collision_rect.w = m_sprite->rect.w;
}

// Construtor: inicializa o objeto em (x, y) com um ponteiro para SpriteData
Object::Object(double x, double y, const SpriteData *sprite)
{
    pos_x = x;
    pos_y = y;
    this->type = ST_NONE; // Tipo padrão, pois SpriteData não contém informação de tipo
    to_erase = false;
    m_sprite = sprite;
    m_frame_display_time = 0;
    m_current_frame = 0;
    color = {255, 255, 255, 255}; // Cor padrão (branco - não altera a cor original)

    // Inicializa o retângulo de origem do sprite
    src_rect.x = m_sprite->rect.x;
    src_rect.y = m_sprite->rect.y;
    src_rect.h = m_sprite->rect.h;
    src_rect.w = m_sprite->rect.w;

    // Inicializa o retângulo de destino (posição na tela)
    dest_rect.x = pos_x;
    dest_rect.y = pos_y;
    dest_rect.h = m_sprite->rect.h;
    dest_rect.w = m_sprite->rect.w;

    // Inicializa o retângulo de colisão
    collision_rect.x = pos_x;
    collision_rect.y = pos_y;
    collision_rect.h = m_sprite->rect.h;
    collision_rect.w = m_sprite->rect.w;
}

// Destrutor padrão
Object::~Object()
{
}

// Desenha o objeto na tela, se houver sprite e não estiver marcado para remoção
void Object::draw()
{
    if(m_sprite == nullptr || to_erase) return;
    
    // Se a cor não é branca padrão, usa renderização com cor
    if(color.r != 255 || color.g != 255 || color.b != 255 || color.a != 255)
    {
        Engine::getEngine().getRenderer()->drawObjectWithColor(&src_rect, &dest_rect, color);
    }
    else
    {
        Engine::getEngine().getRenderer()->drawObject(&src_rect, &dest_rect);
    }
}

// Atualiza o estado do objeto, incluindo animação de frames e retângulos de colisão/destino
void Object::update(Uint32 dt)
{
    if(to_erase) return;

    // Atualiza posição e tamanho do retângulo de destino
    dest_rect.x = pos_x;
    dest_rect.y = pos_y;
    dest_rect.h = m_sprite->rect.h;
    dest_rect.w = m_sprite->rect.w;

    // Atualiza posição e tamanho do retângulo de colisão
    collision_rect.x = pos_x;
    collision_rect.y = pos_y;
    collision_rect.h = m_sprite->rect.h;
    collision_rect.w = m_sprite->rect.w;

    // Se o sprite possui múltiplos frames, processa a animação
    if(m_sprite->frames_count > 1)
    {
        m_frame_display_time += dt;
        if(m_frame_display_time > m_sprite->frame_duration)
        {
            m_frame_display_time = 0;
            m_current_frame++;
            if(m_current_frame >= m_sprite->frames_count)
            {
                // Se o sprite é animado em loop, reinicia o frame, senão permanece no último
                if(m_sprite->loop) m_current_frame = 0;
                else m_current_frame = m_sprite->frames_count - 1;
            }

            // Atualiza o retângulo de origem para o frame atual
            src_rect = moveRect(m_sprite->rect, 0, m_current_frame);
        }
    }
}

// Move o retângulo de origem do sprite para o frame desejado (usado em animações)
// x e y são deslocamentos em número de frames na horizontal e vertical
SDL_Rect Object::moveRect(const SDL_Rect &rect, int x, int y)
{
    SDL_Rect r;
    r.x = rect.x + x*rect.w;
    r.y = rect.y + y*rect.h;
    r.w = rect.w;
    r.h = rect.h;

    return r;
}

// Calcula a interseção entre dois retângulos SDL
SDL_Rect intersectRect(SDL_Rect *rect1, SDL_Rect *rect2)
{
    SDL_Rect intersect_rect;
    intersect_rect.x = std::max(rect1->x, rect2->x);
    intersect_rect.y = std::max(rect1->y, rect2->y);
    intersect_rect.w = std::min(rect1->x + rect1->w, rect2->x + rect2->w) - intersect_rect.x;
    intersect_rect.h = std::min(rect1->y + rect1->h, rect2->y + rect2->h) - intersect_rect.y;

    return intersect_rect;
}
