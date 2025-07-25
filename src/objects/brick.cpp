#include "brick.h"
#include "../soundmanager.h"
#include <iostream>

// Construtor padrão: inicializa o tijolo na posição (0,0) com sprite de parede de tijolo
Brick::Brick()
    : Object(0, 0, ST_BRICK_WALL)
{
    m_collision_count = 0; // contador de colisões começa em 0
    m_state_code = 0;      // código de estado inicial
}

// Construtor parametrizado: inicializa o tijolo na posição (x, y)
Brick::Brick(double x, double y)
    : Object(x, y, ST_BRICK_WALL)
{
    m_collision_count = 0; // contador de colisões começa em 0
    m_state_code = 0;      // código de estado inicial
}

void Brick::update(Uint32 dt)
{
    // Não faz nada (não altera o collision_rect)
}

// Função chamada quando o tijolo é atingido por um tiro
void Brick::bulletHit(Direction bullet_direction)
{
    // sound
    SoundManager::getInstance().playSound("brick_hit");
    if (type == ST_STONE_WALL) {
        SoundManager::getInstance().playSound("steelhit");
    }

    int bd = bullet_direction;
    m_collision_count++; // incrementa o número de colisões

    // Primeira colisão: define o estado de acordo com a direção do tiro
    if(m_collision_count == 1)
    {
        m_state_code = bd + 1;
    }
    // Segunda colisão: calcula novo estado ou marca para remoção
    else if(m_collision_count == 2)
    {
        int sum_square = (m_state_code - 1) * (m_state_code - 1) + bd * bd;
        if(sum_square % 2 == 1)
        {
            // Calcula novo código de estado para casos de colisão em cantos
            m_state_code = ((double)sum_square + 19.0) / 4.0;
        }
        else
        {
            // Estado final: tijolo destruído
            m_state_code = 9;
            to_erase = true;
        }
    }
    // Terceira colisão ou mais: destrói o tijolo
    else
    {
        m_state_code = 9;
        to_erase = true;
    }

    // Atualiza o retângulo de colisão de acordo com o estado do tijolo
    switch(m_state_code)
    {
    case 1: // Metade superior
        collision_rect.x = pos_x;
        collision_rect.y = pos_y;
        collision_rect.h = m_sprite->rect.h / 2;
        collision_rect.w = m_sprite->rect.w;
        break;
    case 2: // Metade direita
        collision_rect.x = pos_x + m_sprite->rect.w / 2;
        collision_rect.y = pos_y;
        collision_rect.h = m_sprite->rect.h;
        collision_rect.w = m_sprite->rect.w / 2;
        break;
    case 3: // Metade inferior
        collision_rect.x = pos_x;
        collision_rect.y = pos_y +  m_sprite->rect.h / 2;
        collision_rect.h = m_sprite->rect.h / 2;
        collision_rect.w = m_sprite->rect.w;
        break;
    case 4: // Metade esquerda
        collision_rect.x = pos_x;
        collision_rect.y = pos_y;
        collision_rect.h = m_sprite->rect.h;
        collision_rect.w = m_sprite->rect.w / 2;
        break;
    case 5: // Quarto superior direito
        collision_rect.x = pos_x + m_sprite->rect.w / 2;
        collision_rect.y = pos_y;
        collision_rect.h = m_sprite->rect.h / 2;
        collision_rect.w = m_sprite->rect.w / 2;
        break;
    case 6: // Quarto inferior direito
        collision_rect.x = pos_x + m_sprite->rect.w / 2;
        collision_rect.y = pos_y + m_sprite->rect.h / 2;
        collision_rect.h = m_sprite->rect.h / 2;
        collision_rect.w = m_sprite->rect.w / 2;
        break;
    case 7: // Quarto superior esquerdo
        collision_rect.x = pos_x;
        collision_rect.y = pos_y;
        collision_rect.h = m_sprite->rect.h / 2;
        collision_rect.w = m_sprite->rect.w / 2;
        break;
    case 8: // Quarto inferior esquerdo
        collision_rect.x = pos_x;
        collision_rect.y = pos_y + m_sprite->rect.h / 2;
        collision_rect.h = m_sprite->rect.h / 2;
        collision_rect.w = m_sprite->rect.w / 2;
        break;
    case 9: // Tijolo destruído: retângulo de colisão vazio
        collision_rect.x = 0;
        collision_rect.y = 0;
        collision_rect.h = 0;
        collision_rect.w = 0;
        break;
    }

    // Atualiza o retângulo fonte do sprite de acordo com o estado
    src_rect = moveRect(m_sprite->rect, 0, m_state_code);
}
