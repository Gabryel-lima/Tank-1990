#include "player.h" // Ok
#include "../appconfig.h" // Ok
#include <SDL2/SDL.h>
#include <iostream>

// Construtor padrão: inicializa o jogador na posição inicial padrão (jogador 1)
Player::Player()
    : Tank(AppConfig::player_starting_point.at(0).x, AppConfig::player_starting_point.at(0).y, ST_PLAYER_1) {
    speed = 0; // velocidade inicial
    lives_count = 11; // número inicial de vidas
    m_bullet_max_size = AppConfig::player_bullet_max_size; // máximo de balas permitidas
    score = 0; // pontuação inicial
    star_count = 0; // número de estrelas inicial
    m_shield = new Object(0, 0, ST_SHIELD); // cria o escudo
    m_shield_time = 0; // tempo de escudo inicial
    respawn(); // chama respawn para posicionar e preparar o jogador
}

// Construtor alternativo: inicializa o jogador em uma posição e tipo específicos
Player::Player(double x, double y, SpriteType type)
    : Tank(x, y, type) {
   speed = 0;
   lives_count = 11;
   m_bullet_max_size = AppConfig::player_bullet_max_size;
   score = 0;
   star_count = 0;
   m_shield = new Object(x, y, ST_SHIELD);
   m_shield_time = 0;
   respawn();
}

// Atualiza o estado do jogador, processa input e animação
void Player::update(Uint32 dt) {
    const Uint8 *key_state = SDL_GetKeyboardState(NULL); // obtém o estado do teclado

    Tank::update(dt); // atualiza lógica base do tanque

    // Processa input do jogador se não estiver no menu
    if(key_state != nullptr && !testFlag(TSF_MENU)) {
        // Movimentação
        if(key_state[player_keys.up]) {
            setDirection(D_UP);
            speed = default_speed;
        }
        else if(key_state[player_keys.down]) {
            setDirection(D_DOWN);
            speed = default_speed;
        }
        else if(key_state[player_keys.left]) {
            setDirection(D_LEFT);
            speed = default_speed;
        }
        else if(key_state[player_keys.right]) {
            setDirection(D_RIGHT);
            speed = default_speed;
        }
        else {
            // Se não está no gelo ou não está escorregando, para o tanque
            if(!testFlag(TSF_ON_ICE) || m_slip_time == 0)
                speed = 0.0;
        }

        // Disparo
        if(key_state[player_keys.fire] && m_fire_time > AppConfig::player_reload_time) {
            fire();
            m_fire_time = 0;
        }
    }

    m_fire_time += dt; // incrementa o tempo desde o último disparo

    // Atualiza o sprite de acordo com o estado de vida e estrelas
    if(testFlag(TSF_LIFE))
        src_rect = moveRect(m_sprite->rect, (testFlag(TSF_ON_ICE) ? new_direction : direction), m_current_frame + 2 * star_count);
    else
        src_rect = moveRect(m_sprite->rect, 0, m_current_frame + 2 * star_count);

    stop = false; // reseta flag de parada
}

// Remove uma vida, reposiciona o jogador e ativa o escudo temporário
void Player::respawn() {
    lives_count--; // perde uma vida
    if(lives_count <= 0) {
        // Se não há mais balas em jogo, marca para remoção
        if(bullets.size() == 0) to_erase = true;
        return;
    }

    // Define posição inicial conforme o tipo do jogador
    if(type == ST_PLAYER_1) {
        pos_x = AppConfig::player_starting_point.at(0).x;
        pos_y = AppConfig::player_starting_point.at(0).y;
    }
    else {
        pos_x = AppConfig::player_starting_point.at(1).x;
        pos_y = AppConfig::player_starting_point.at(1).y;
    }

    // Atualiza retângulo de destino do sprite
    dest_rect.x = pos_x;
    dest_rect.y = pos_y;
    dest_rect.h = m_sprite->rect.h;
    dest_rect.w = m_sprite->rect.w;

    setDirection(D_UP); // sempre renasce virado para cima
    Tank::respawn(); // chama lógica base de respawn
    setFlag(TSF_SHIELD); // ativa escudo temporário
    m_shield_time = AppConfig::tank_shield_time / 2; // tempo reduzido de escudo
}

// Lida com destruição do tanque do jogador
void Player::destroy() {
    if(testFlag(TSF_SHIELD)) return; // não destrói se tem escudo
    if(testFlag(TSF_BOAT)) {
        clearFlag(TSF_BOAT); // perde o barco, mas não morre
        return;
    }

    // Se tem 3 estrelas, perde só uma estrela; senão, perde 3 e morre
    if(star_count == 3)
        changeStarCountBy(-1);
    else {
        changeStarCountBy(-3);
        Tank::destroy();
    }
}

// Cria um projétil, ajustando velocidade e dano conforme estrelas
Bullet* Player::fire() {
    Bullet* b = Tank::fire();
    if(b != nullptr) {
        // Se tem pelo menos uma estrela, aumenta a velocidade do tiro
        if(star_count > 0) b->speed = AppConfig::bullet_default_speed * 1.3;
        // Se tem 3 estrelas, aumenta o dano do tiro
        if(star_count == 3) b->increased_damage = true;
    }
    return b;
}

// Altera o número de estrelas do jogador e ajusta atributos relacionados
void Player::changeStarCountBy(int c) {
    star_count += c;
    if(star_count > 3) star_count = 3;
    else if(star_count < 0) star_count = 0;

    // Se ganhou estrela e já tem pelo menos 2, aumenta o máximo de balas
    if(star_count >= 2 && c > 0) m_bullet_max_size++;
    else m_bullet_max_size = 2;

    // Se tem pelo menos uma estrela, aumenta a velocidade padrão
    if(star_count > 0) default_speed = AppConfig::tank_default_speed * 1.3;
    else default_speed = AppConfig::tank_default_speed;
}
