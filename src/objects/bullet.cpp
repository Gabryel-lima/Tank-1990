#include "bullet.h"
#include "../appconfig.h"

// Construtor padrão: inicializa o projétil na posição (0,0) com sprite padrão
Bullet::Bullet()
    : Object(0, 0, ST_BULLET)
{
    speed = 0.0;
    direction = D_UP;
    increased_damage = false;
    collide = false;
}

// Construtor parametrizado: inicializa o projétil na posição (x, y) com sprite padrão
Bullet::Bullet(double x, double y)
    : Object(x, y, ST_BULLET)
{
    speed = 0.0;
    direction = D_UP;
    increased_damage = false;
    collide = false;
}

// Atualiza o estado do projétil
void Bullet::update(Uint32 dt)
{
    // Se o projétil ainda não colidiu, move conforme a direção e velocidade
    if(!collide)
    {
        switch (direction)
        {
        case D_UP:
            pos_y -= speed * dt;
            break;
        case D_RIGHT:
            pos_x += speed * dt;
            break;
        case D_DOWN:
            pos_y += speed * dt;
            break;
        case D_LEFT:
            pos_x -= speed * dt;
            break;
        }

        // Atualiza o retângulo de origem do sprite conforme a direção
        src_rect = moveRect(m_sprite->rect, direction, 0);
        Object::update(dt);
    }
    else
    {
        // Se o projétil colidiu e possui animação de destruição (mais de 1 frame)
        if(m_sprite->frames_count > 1)
        {
            m_frame_display_time += dt;
            if(m_frame_display_time > m_sprite->frame_duration)
            {
                m_frame_display_time = 0;
                m_current_frame++;
                // Se chegou ao último frame da animação, marca para remoção
                if(m_current_frame >= m_sprite->frames_count)
                    to_erase = true;

                // Atualiza o frame da animação de destruição
                src_rect = moveRect(m_sprite->rect, 0, m_current_frame);
            }
        }
    }
}

// Inicia a animação de destruição do projétil
void Bullet::destroy()
{
    // Evita múltiplas chamadas para destruir o mesmo projétil
    if(collide) return; // impede múltiplas execuções

    collide = true;
    speed = 0;
    m_current_frame = 0;
    m_frame_display_time = 0;
    // Troca o sprite para o de destruição do projétil
    m_sprite = Engine::getEngine().getSpriteConfig()->getSpriteData(ST_DESTROY_BULLET);

    // Ajusta a posição do retângulo de destino para centralizar a animação de destruição
    switch(direction)
    {
    case D_UP:
        dest_rect.x = pos_x + (dest_rect.w - m_sprite->rect.w) / 2; // dest_rect.w, dest_rect.h - tamanho antigo do projétil
        dest_rect.y = pos_y - m_sprite->rect.h / 2;
        break;
    case D_RIGHT:
        dest_rect.x = pos_x + dest_rect.w - m_sprite->rect.w / 2;
        dest_rect.y = pos_y + (dest_rect.h - m_sprite->rect.h) / 2;
        break;
    case D_DOWN:
        dest_rect.x = pos_x + (dest_rect.w - m_sprite->rect.w) / 2;
        dest_rect.y = pos_y + dest_rect.h - m_sprite->rect.h / 2;
        break;
    case D_LEFT:
        dest_rect.x = pos_x - m_sprite->rect.w / 2;
        dest_rect.y = pos_y + (dest_rect.h - m_sprite->rect.h) / 2;
        break;
    }

    // Atualiza o tamanho do retângulo de destino para o tamanho da animação de destruição
    dest_rect.h = m_sprite->rect.h;
    dest_rect.w = m_sprite->rect.w;

    // Atualiza o retângulo de origem do sprite para o início da animação de destruição
    src_rect.x = m_sprite->rect.x;
    src_rect.y = m_sprite->rect.y;
    src_rect.h = m_sprite->rect.h;
    src_rect.w = m_sprite->rect.w;

}
