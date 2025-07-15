#include "enemy.h"
#include "../appconfig.h"
#include <stdlib.h>
#include <ctime>
#include <iostream>

// Construtor padrão do inimigo. Inicializa o inimigo na posição inicial padrão e define os parâmetros iniciais.
Enemy::Enemy()
    : Tank(AppConfig::enemy_starting_point.at(0).x, AppConfig::enemy_starting_point.at(0).y, ST_TANK_A)
{
    direction = D_DOWN; // Direção inicial para baixo
    m_direction_time = 0; // Tempo acumulado para troca de direção
    m_keep_direction_time = 100; // Tempo mínimo para manter a direção

    m_speed_time = 0; // Tempo acumulado para tentar se mover
    m_try_to_go_time = 100; // Intervalo para tentar se mover

    m_fire_time = 0; // Tempo acumulado para disparo
    m_reload_time = 100; // Tempo de recarga do disparo
    lives_count = 1; // Número de vidas do inimigo

    m_bullet_max_size = 1; // Quantidade máxima de balas simultâneas

    m_frozen_time = 0; // Tempo de congelamento

    // Ajusta a velocidade padrão dependendo do tipo do tanque
    if(type == ST_TANK_B)
        default_speed = AppConfig::tank_default_speed * 1.3;
    else
        default_speed = AppConfig::tank_default_speed;

    target_position = {-1, -1}; // Posição alvo inicial inválida

    respawn(); // Reposiciona o inimigo e reseta estados
}

// Construtor parametrizado do inimigo. Permite definir posição e tipo do sprite.
Enemy::Enemy(double x, double y, SpriteType type)
    : Tank(x, y, type)
{
    direction = D_DOWN;
    m_direction_time = 0;
    m_keep_direction_time = 100;

    m_speed_time = 0;
    m_try_to_go_time = 100;

    m_fire_time = 0;
    m_reload_time = 100;
    lives_count = 1;

    m_bullet_max_size = 1;

    m_frozen_time = 0;

    if(type == ST_TANK_B)
        default_speed = AppConfig::tank_default_speed * 1.3;
    else
        default_speed = AppConfig::tank_default_speed;

    target_position = {-1, -1};

    respawn();
}

// Desenha o inimigo na tela. Se ativado, mostra também a linha até o alvo do inimigo.
void Enemy::draw()
{
    if(to_erase) return; // Não desenha se marcado para remoção
    if(AppConfig::show_enemy_target)
    {
        SDL_Color c;
        // Define a cor da linha de acordo com o tipo do tanque
        if(type == ST_TANK_A) c = {250, 0, 0, 250};
        if(type == ST_TANK_B) c = {0, 0, 250, 255};
        if(type == ST_TANK_C) c = {0, 255, 0, 250};
        if(type == ST_TANK_D) c = {250, 0, 255, 250};
        // Desenha linha horizontal até o alvo
        SDL_Rect r = {min(target_position.x, dest_rect.x + dest_rect.w / 2), dest_rect.y + dest_rect.h / 2, abs(target_position.x - (dest_rect.x + dest_rect.w / 2)), 1};
        Engine::getEngine().getRenderer()->drawRect(&r, c,  true);
        // Desenha linha vertical até o alvo
        r = {target_position.x, min(target_position.y, dest_rect.y + dest_rect.h / 2), 1, abs(target_position.y - (dest_rect.y + dest_rect.h / 2))};
        Engine::getEngine().getRenderer()->drawRect(&r, c, true);
    }
    Tank::draw(); // Chama o desenho padrão do tanque
}

// Atualiza o estado do inimigo, incluindo movimentação, direção, disparo e animação.
void Enemy::update(Uint32 dt)
{
    if(to_erase) return; // Não atualiza se marcado para remoção
    Tank::update(dt); // Atualiza lógica base do tanque

    // Atualiza o sprite de acordo com flags e vidas
    if(testFlag(TSF_LIFE))
    {
        if(testFlag(TSF_BONUS))
            // Se está em bônus, usa linha de sprites especial
            src_rect = moveRect(m_sprite->rect, (testFlag(TSF_ON_ICE) ? new_direction : direction) - 4, m_current_frame);
        else
            // Linha de sprites depende da quantidade de vidas
            src_rect = moveRect(m_sprite->rect, (testFlag(TSF_ON_ICE) ? new_direction : direction) + (lives_count -1) * 4, m_current_frame);
    }
    else
        // Sprite padrão se não tem vida
        src_rect = moveRect(m_sprite->rect, 0, m_current_frame);

    // Se está congelado, não faz nada além de atualizar sprite
    if(testFlag(TSF_FROZEN)) return;

    // Acumula tempo para cada ação
    m_direction_time += dt;
    m_speed_time += dt;
    m_fire_time += dt;

    // Troca de direção após tempo aleatório
    if(m_direction_time > m_keep_direction_time)
    {
        m_direction_time = 0;
        m_keep_direction_time = rand() % 800 + 100; // Novo tempo aleatório para manter direção

        float p = static_cast<float>(rand()) / RAND_MAX;

        // Decide se vai perseguir o alvo ou escolher direção aleatória
        if(p < (type == ST_TANK_A ? 0.8f : 0.5f) && target_position.x > 0 && target_position.y > 0)
        {
            int dx = target_position.x - (dest_rect.x + dest_rect.w / 2);
            int dy = target_position.y - (dest_rect.y + dest_rect.h / 2);

            p = static_cast<float>(rand()) / RAND_MAX;

            // Prioriza o eixo de maior distância para perseguir o alvo
            if(abs(dx) > abs(dy))
                setDirection(p < 0.7 ? (dx < 0 ? D_LEFT : D_RIGHT) : (dy < 0 ? D_UP : D_DOWN));
            else
                setDirection(p < 0.7 ? (dy < 0 ? D_UP : D_DOWN) : (dx < 0 ? D_LEFT : D_RIGHT));
        }
        else
            setDirection(static_cast<Direction>(rand() % 4)); // Direção aleatória
    }

    // Tenta se mover após tempo aleatório
    if(m_speed_time > m_try_to_go_time)
    {
        m_speed_time = 0;
        m_try_to_go_time = rand() % 300; // Novo tempo aleatório para tentar se mover
        speed = default_speed; // Restaura velocidade padrão
    }

    // Lógica de disparo automática do inimigo
    if(m_fire_time > m_reload_time)
    {
        m_fire_time = 0;
        if(type == ST_TANK_D)
        {
            m_reload_time = rand() % 400; // Recarga mais rápida para TANK_D
            int dx = target_position.x - (dest_rect.x + dest_rect.w / 2);
            int dy = target_position.y - (dest_rect.y + dest_rect.h / 2);

            // Se está parado, dispara imediatamente
            if(stop) fire();
            else
                // Só dispara se estiver alinhado com o alvo na direção atual
                switch (direction)
                {
                case D_UP:
                    if(dy < 0 && abs(dx) < dest_rect.w) fire();
                    break;
                case D_RIGHT:
                    if(dx > 0 && abs(dy) < dest_rect.h) fire();
                    break;
                case D_DOWN:
                    if(dy > 0 && abs(dx) < dest_rect.w) fire();
                    break;
                case D_LEFT:
                    if(dx < 0 && abs(dy) < dest_rect.h) fire();
                    break;
                }
        }
        else if(type == ST_TANK_C)
        {
            m_reload_time = rand() % 800; // Recarga intermediária para TANK_C
            fire();
        }
        else
        {
            m_reload_time = rand() % 1000; // Recarga mais lenta para outros tipos
            fire();
        }
    }

    stop = false; // Reseta flag de parada para próxima atualização
}

// Diminui a quantidade de vidas do inimigo e destrói se chegar a zero.
void Enemy::destroy()
{
    lives_count--;
    // clearFlag(TSF_BONUS); // possível queda única de bônus (comentado)
    if(lives_count <= 0)
    {
        lives_count = 0;
        Tank::destroy(); // Chama destruição padrão do tanque
    }
}

// Retorna a pontuação ao acertar o inimigo. 50 pontos se ainda tem vidas, 100 se foi destruído.
unsigned Enemy::scoreForHit()
{
    if(lives_count > 0) return 50;
    return 100;
}
