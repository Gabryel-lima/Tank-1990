#include "eagle.h"

// Construtor padrão: inicializa a águia na posição (0,0) com o sprite padrão de águia
Eagle::Eagle()
    : Object(0, 0, ST_EAGLE)
{
}

// Construtor parametrizado: inicializa a águia na posição (x, y) com o sprite padrão de águia
Eagle::Eagle(double x, double y)
    : Object(x, y, ST_EAGLE)
{
}

// Atualiza o estado da águia
void Eagle::update(Uint32 dt)
{
    // Se a águia está no estado de destruição, processa a animação de destruição
    if(type == ST_DESTROY_EAGLE)
    {
        m_frame_display_time += dt; // Acumula o tempo desde o último frame
        if(m_frame_display_time > m_sprite->frame_duration)
        {
            m_frame_display_time = 0;
            m_current_frame++;
            // Se chegou ao último frame da animação de destruição
            if(m_current_frame >= m_sprite->frames_count)
            {
                // Troca o sprite para a bandeira (flag) e redefine o tipo
                m_sprite = Engine::getEngine().getSpriteConfig()->getSpriteData(ST_FLAG);
                m_current_frame = 0;
                type = ST_FLAG;
                update(0); // Atualiza imediatamente para o novo estado
            }
            // Atualiza o retângulo de origem do sprite para o frame atual
            src_rect = moveRect(m_sprite->rect, 0, m_current_frame);
        }
    }
    else
        // Se não está destruída, utiliza a atualização padrão do objeto
        Object::update(dt);
}

// Inicia a animação de destruição da águia
void Eagle::destroy()
{
    // Só executa se a águia ainda não estiver destruída
    if(type != ST_EAGLE) return;

    // Troca o sprite para o de destruição e atualiza o tipo
    m_sprite = Engine::getEngine().getSpriteConfig()->getSpriteData(ST_DESTROY_EAGLE);
    type = ST_DESTROY_EAGLE;
    src_rect = moveRect(m_sprite->rect, 0, 0);

    // Centraliza o sprite de destruição na posição da águia
    dest_rect.x = pos_x + (dest_rect.w - m_sprite->rect.w)/2;
    dest_rect.y = pos_y + (dest_rect.h - m_sprite->rect.h)/2;
    dest_rect.h = m_sprite->rect.h;
    dest_rect.w = m_sprite->rect.w;
}
