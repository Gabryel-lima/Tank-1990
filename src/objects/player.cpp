#include "player.h"
#include "../appconfig.h"
#include "../soundmanager.h"
#include "../controllers.h"

#include <iostream>
#include <SDL2/SDL.h>


// Construtor padrão do jogador.
// Inicializa o jogador na posição inicial definida em AppConfig.
Player::Player(int idx)
    : Tank(AppConfig::player_starting_point.at(idx).x, AppConfig::player_starting_point.at(idx).y, static_cast<SpriteType>(ST_PLAYER_1 + idx))
{
    m_index = idx;
    speed = 0; // Velocidade inicial
    lives_count = 4; // Número inicial de vidas
    m_bullet_max_size = AppConfig::player_bullet_max_size; // Máximo de balas simultâneas
    score = 0; // Pontuação inicial
    star_count = 0; // Nível de power-up (estrelas)
    m_shield = new Object(pos_x, pos_y, ST_SHIELD); // Cria o escudo do jogador
    m_shield_time = 0; // Tempo de escudo inicial
    m_fire_time = 0; // Tempo desde o ultimo disparo
    m_reload_time = AppConfig::player_reload_time; // Intervalo mínimo entre tiros

    // Define a cor do jogador baseada no índice
    setPlayerColor(getPlayerColor(idx));

    respawn(); // Posiciona o jogador e reseta estados
}

// Destrutor do jogador.
// O escudo é liberado por ~Tank; o controle pertence à classe Controllers.
Player::~Player()
{
}

// Atualiza o estado do jogador a cada frame.
// Processa entrada do teclado e do controle, movimentação, tiro e animação.
void Player::update(Uint32 dt)
{
    Tank::update(dt); // Atualiza lógica base do tanque

    // Só processa input se não estiver no menu
    if(!testFlag(TSF_MENU))
    {
        bool up = false, down = false, left = false, right = false, shoot = false;

        // Jogador do computador: a IA do modo de jogo decide no lugar do teclado
        if(cpu)
        {
            up    = cpu_command.move && cpu_command.direction == D_UP;
            down  = cpu_command.move && cpu_command.direction == D_DOWN;
            left  = cpu_command.move && cpu_command.direction == D_LEFT;
            right = cpu_command.move && cpu_command.direction == D_RIGHT;
            shoot = cpu_command.fire;
            if(!cpu_command.move) setDirection(cpu_command.direction); // vira sem andar
        }

        // Teclado: o layout que a classe Controllers deu a este jogador (reserva de quem não tem controle)
        const Uint8 *key_state = cpu ? nullptr : SDL_GetKeyboardState(NULL);
        const PlayerKeys* keys = Controllers::keyboardFor(playerIndex());
        if(key_state != nullptr && keys != nullptr)
        {
            up    = key_state[keys->up];
            down  = key_state[keys->down];
            left  = key_state[keys->left];
            right = key_state[keys->right];
            shoot = key_state[keys->fire];
        }

        // Controle: D-pad ou analógico esquerdo; qualquer botão frontal atira
        SDL_GameController* pad = cpu ? nullptr : Controllers::forPlayer(playerIndex());
        if(pad != nullptr)
        {
            Sint16 axis_x = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
            Sint16 axis_y = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);
            up    = up    || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP)    || axis_y < -ANALOG_DEADZONE;
            down  = down  || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN)  || axis_y >  ANALOG_DEADZONE;
            left  = left  || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT)  || axis_x < -ANALOG_DEADZONE;
            right = right || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || axis_x >  ANALOG_DEADZONE;
            shoot = shoot || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A)
                          || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B)
                          || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_X)
                          || SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_Y);
        }

        // Movimentação: uma direção por vez, na ordem cima/baixo/esquerda/direita
        if(up)         setDirection(D_UP);
        else if(down)  setDirection(D_DOWN);
        else if(left)  setDirection(D_LEFT);
        else if(right) setDirection(D_RIGHT);

        if(up || down || left || right)
            speed = default_speed;
        else if(!testFlag(TSF_ON_ICE) || m_slip_time == 0)
            speed = 0.0; // Para o tanque, exceto se estiver escorregando no gelo

        // Disparo: respeita o tempo de recarga
        if(shoot && m_fire_time > m_reload_time)
        {
            fire();
            m_fire_time = 0;
        }
    }

    m_fire_time += dt; // Atualiza tempo desde o último tiro

    // Atualiza o frame do sprite conforme o estado de vida e power-up
    if(testFlag(TSF_LIFE))
        src_rect = moveRect(m_sprite->rect, (testFlag(TSF_ON_ICE) ? new_direction : direction), m_current_frame + 2 * star_count);
    else
        src_rect = moveRect(m_sprite->rect, 0, m_current_frame + 2 * star_count);

    stop = false; // Marca que o jogador não está parado (usado para animação)
}

// Reposiciona o jogador após perder uma vida ou ao iniciar.
// Reseta posição, direção, escudo e chama respawn da classe base.
void Player::respawn()
{
    lives_count--; // Diminui o número de vidas
    if(lives_count <= 0)
    {
        // Se não há balas em jogo, marca para remoção
        if(bullets.size() == 0) to_erase = true;
        return;
    }

    // Usa o ponto de respawn próprio (duelo) ou o ponto padrão do jogador
    int idx = playerIndex();
    if(spawn_point.x >= 0) {
        pos_x = spawn_point.x;
        pos_y = spawn_point.y;
    }
    else if (idx >= 0 && idx < static_cast<int>(AppConfig::player_starting_point.size())) {
        pos_x = AppConfig::player_starting_point.at(idx).x;
        pos_y = AppConfig::player_starting_point.at(idx).y;
    }

    // Atualiza retângulo de destino do sprite
    dest_rect.x = pos_x;
    dest_rect.y = pos_y;
    dest_rect.h = m_sprite->rect.h;
    dest_rect.w = m_sprite->rect.w;

    // Renasce apontando para cima; no duelo, a equipe de cima (B) renasce apontando para baixo
    setDirection(team == 1 ? D_DOWN : D_UP);
    // A IA começa parada, olhando para onde o tanque nasceu (Tank::respawn chama update)
    cpu_command = TankCommand();
    cpu_command.direction = direction;
    Tank::respawn(); // Chama respawn da classe base
    setFlag(TSF_SHIELD); // Ativa escudo temporário
    m_shield_time = AppConfig::tank_shield_time / 2; // Tempo reduzido de escudo
    
    // Atualiza a posição e cor do escudo
    if(m_shield != nullptr)
    {
        m_shield->pos_x = pos_x;
        m_shield->pos_y = pos_y;
        m_shield->color = color; // Aplica a mesma cor do jogador ao escudo
    }
}

// Lógica de destruição do jogador.
// Considera escudo, barco e nível de estrela antes de destruir de fato.
void Player::destroy()
{
    // sound
    SoundManager::getInstance().playSound("player_exp");

    if(testFlag(TSF_SHIELD)) return; // Não destrói se estiver com escudo
    if(testFlag(TSF_BOAT))
    {
        clearFlag(TSF_BOAT); // Perde o barco, mas não morre
        return;
    }

    // Se está no nível máximo de estrela, perde só uma estrela (a menos que o modo
    // de jogo desligue essa "armadura", como o duelo)
    if(star_count == 3 && star_armor)
        changeStarCountBy(-1);
    else
    {
        // Perde três estrelas e chama destruição da classe base
        changeStarCountBy(-3);
        Tank::destroy();
    }
}

// Dispara uma bala, ajustando propriedades conforme o nível de estrela.
// Retorna ponteiro para a bala criada.
Bullet* Player::fire()
{
    Bullet* b = Tank::fire();
    if(b != nullptr)
    {
        // sound
        SoundManager::getInstance().playSound("shoot");
        // Se tem pelo menos uma estrela, aumenta a velocidade do tiro
        if(star_count > 0) b->speed = AppConfig::bullet_default_speed * 1.3;
        // Se está no nível máximo, o tiro causa mais dano
        if(star_count == 3) b->increased_damage = true;
    }
    return b;
}

// Altera o número de estrelas (power-up) do jogador.
// Ajusta velocidade, quantidade de balas e limita o valor.
void Player::changeStarCountBy(int c)
{
    star_count += c;
    if(star_count > 3) star_count = 3;
    else if(star_count < 0) star_count = 0;

    // Se ganhou estrela e chegou a 2 ou mais, aumenta o limite de balas
    if(star_count >= 2 && c > 0) m_bullet_max_size++;
    else m_bullet_max_size = 2;

    // Se tem pelo menos uma estrela, aumenta a velocidade padrão
    if(star_count > 0) default_speed = AppConfig::tank_default_speed * 1.3;
    else default_speed = AppConfig::tank_default_speed;
}

void Player::setReloadTime(Uint32 ms)
{
    m_reload_time = ms;
}

void Player::addLife() {
    lives_count++;
    SoundManager::getInstance().playSound("life");
}

void Player::shieldHit() {
    SoundManager::getInstance().playSound("shieldhit");
}

int Player::playerIndex() const
{
    return m_index;
}

void Player::setPlayerColor(SDL_Color player_color)
{
    color = player_color;
    // Também aplica a cor ao escudo, se existir
    if(m_shield != nullptr)
    {
        m_shield->color = player_color;
    }
}

SDL_Color Player::getPlayerColor(int player_index)
{
    // Define cores específicas para cada jogador
    switch(player_index)
    {
        case 0: // Player 1 - Amarelo dourado
            return {255, 215, 0, 255};
        case 1: // Player 2 - Verde
            return {0, 255, 0, 255};
        case 2: // Player 3 - Azul
            return {0, 100, 255, 255};
        case 3: // Player 4 - Vermelho
            return {255, 50, 50, 255};
        default: // Cor padrão (branco)
            return {255, 255, 255, 255};
    }
}

void Player::setFlag(TankStateFlag flag)
{
    // Chama o método da classe base
    Tank::setFlag(flag);
    
    // Se o escudo foi ativado, aplica a cor do jogador
    if(flag == TSF_SHIELD && m_shield != nullptr)
    {
        m_shield->color = color;
    }
}
