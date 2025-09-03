#include "player.h"
#include "../appconfig.h"
#include "../soundmanager.h"

#include <iostream>
#include <SDL2/SDL.h>


// Construtor padrão do jogador.
// Inicializa o jogador na posição inicial definida em AppConfig.
Player::Player(const PlayerKeys& keys, int idx)
    : Tank(AppConfig::player_starting_point.at(idx).x, AppConfig::player_starting_point.at(idx).y, static_cast<SpriteType>(ST_PLAYER_1 + idx)), player_keys(keys)
{
    speed = 0; // Velocidade inicial
    lives_count = 4; // Número inicial de vidas
    m_bullet_max_size = AppConfig::player_bullet_max_size; // Máximo de balas simultâneas
    score = 0; // Pontuação inicial
    star_count = 0; // Nível de power-up (estrelas)
    m_shield = new Object(pos_x, pos_y, ST_SHIELD); // Cria o escudo do jogador
    m_shield_time = 0; // Tempo de escudo inicial
    m_controller = nullptr;
    if(player_keys.type == Player::InputType::Controller || player_keys.type == Player::InputType::Hybrid)
    {
        // Verifica se há controles suficientes conectados
        int num_joysticks = SDL_NumJoysticks();
        if (idx < num_joysticks && SDL_IsGameController(idx)) {
            m_controller = SDL_GameControllerOpen(idx);
        }
    }
    
    // Ajusta o tipo de input baseado na disponibilidade de controles
    adjustInputType(idx);
    
    // Define a cor do jogador baseada no índice
    setPlayerColor(getPlayerColor(idx));
    
    respawn(); // Posiciona o jogador e reseta estados
}

// Construtor com índices separados.
// Permite separar o índice do jogador (cor/posição) do índice do controle físico.
Player::Player(const PlayerKeys& keys, int player_idx, int controller_idx)
    : Tank(AppConfig::player_starting_point.at(player_idx).x, AppConfig::player_starting_point.at(player_idx).y, static_cast<SpriteType>(ST_PLAYER_1 + player_idx)), player_keys(keys)
{
    speed = 0; // Velocidade inicial
    lives_count = 4; // Número inicial de vidas
    m_bullet_max_size = AppConfig::player_bullet_max_size; // Máximo de balas simultâneas
    score = 0; // Pontuação inicial
    star_count = 0; // Nível de power-up (estrelas)
    m_shield = new Object(pos_x, pos_y, ST_SHIELD); // Cria o escudo do jogador
    m_shield_time = 0; // Tempo de escudo inicial
    m_controller = nullptr;
    if(player_keys.type == Player::InputType::Controller || player_keys.type == Player::InputType::Hybrid)
    {
        // Verifica se há controles suficientes conectados
        int num_joysticks = SDL_NumJoysticks();
        if (controller_idx < num_joysticks && SDL_IsGameController(controller_idx)) {
            m_controller = SDL_GameControllerOpen(controller_idx);
        }
    }
    
    // Ajusta o tipo de input baseado na disponibilidade de controles
    adjustInputType(player_idx);
    
    // Define a cor do jogador baseada no índice do jogador (não do controle)
    setPlayerColor(getPlayerColor(player_idx));
    
    respawn(); // Posiciona o jogador e reseta estados
}

// Construtor parametrizado do jogador.
// Permite definir posição e tipo do sprite.
Player::Player(double x, double y, SpriteType type, int idx)
    : Tank(x, y, type)
{
   speed = 0;
   lives_count = 4;
   m_bullet_max_size = AppConfig::player_bullet_max_size;
   score = 0;
   star_count = 0;
   m_shield = new Object(x, y, ST_SHIELD);
   m_shield_time = 0;
   m_controller = nullptr;
   if(player_keys.type == Player::InputType::Controller || player_keys.type == Player::InputType::Hybrid)
   {
       // Verifica se há controles suficientes conectados
       int num_joysticks = SDL_NumJoysticks();
       if (idx < num_joysticks && SDL_IsGameController(idx)) {
           m_controller = SDL_GameControllerOpen(idx);
       }
   }
   
   // Define a cor do jogador baseada no índice
   setPlayerColor(getPlayerColor(idx));
   
   respawn();
}

// Destrutor do jogador.
// Fecha o controle do jogador se aberto e libera o escudo se alocado.
Player::~Player()
{
    if(m_controller)
    {
        SDL_GameControllerClose(m_controller);
        m_controller = nullptr;
    }
    if(m_shield)
    {
        delete m_shield;
        m_shield = nullptr;
    }
}

// Atualiza o estado do jogador a cada frame.
// Processa entrada do teclado, movimentação, tiro e animação.
void Player::update(Uint32 dt)
{
    Tank::update(dt); // Atualiza lógica base do tanque

    // Só processa input se não estiver no menu
    if(!testFlag(TSF_MENU))
    {
        // Ajusta dinamicamente o input baseado na disponibilidade de controles
        // Se for o player 2, verifica se o player 1 está usando controle
        int player_index = static_cast<int>(type) - static_cast<int>(ST_PLAYER_1);
        if (player_index == 1) {
            adjustInputType(player_index);
        }
        
        if(player_keys.type == Player::InputType::Keyboard)
        {
            const Uint8 *key_state = SDL_GetKeyboardState(NULL); // Estado atual do teclado
            if(key_state != nullptr)
            {
                // Movimentação: verifica teclas direcionais
                if(key_state[player_keys.up])
                {
                    setDirection(D_UP);
                    speed = default_speed;
                }
                else if(key_state[player_keys.down])
                {
                    setDirection(D_DOWN);
                    speed = default_speed;
                }
                else if(key_state[player_keys.left])
                {
                    setDirection(D_LEFT);
                    speed = default_speed;
                }
                else if(key_state[player_keys.right])
                {
                    setDirection(D_RIGHT);
                    speed = default_speed;
                }
                else
                {
                    // Se não está no gelo ou não está escorregando, para o tanque
                    if(!testFlag(TSF_ON_ICE) || m_slip_time == 0)
                        speed = 0.0;
                }

                // Disparo: verifica tecla de tiro e tempo de recarga
                if(key_state[player_keys.fire] && m_fire_time > AppConfig::player_reload_time)
                {
                    fire();
                    m_fire_time = 0;
                }
            }
        }
        else if (player_keys.type == Player::InputType::Controller && m_controller)
        {
            bool moved = false;
            
            // --- Movimento usando analógicos ou D-pad ---
            // Verifica se são analógicos (valores negativos indicam analógicos)
            if (player_keys.axis_up < 0 || player_keys.axis_down < 0 || 
                player_keys.axis_left < 0 || player_keys.axis_right < 0)
            {
                // Processa analógicos
                // Zona morta otimizada para controle puro
                const Sint16 DEADZONE = ANALOG_DEADZONE_CONTROLLER;
                
                // Lê os valores dos analógicos
                Sint16 axis_y = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTY);
                Sint16 axis_x = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTX);
                
                // Processa movimento vertical (eixo Y)
                if (axis_y < -DEADZONE) {
                    setDirection(D_UP);
                    speed = default_speed;
                    moved = true;
                }
                else if (axis_y > DEADZONE) {
                    setDirection(D_DOWN);
                    speed = default_speed;
                    moved = true;
                }
                // Processa movimento horizontal (eixo X)
                else if (axis_x < -DEADZONE) {
                    setDirection(D_LEFT);
                    speed = default_speed;
                    moved = true;
                }
                else if (axis_x > DEADZONE) {
                    setDirection(D_RIGHT);
                    speed = default_speed;
                    moved = true;
                }
            }
            else
            {
                // Processa D-pad (botões)
                if (SDL_GameControllerGetButton(m_controller, SDL_GameControllerButton(player_keys.axis_up))) {
                    setDirection(D_UP);
                    speed = default_speed;
                    moved = true;
                }
                else if (SDL_GameControllerGetButton(m_controller, SDL_GameControllerButton(player_keys.axis_down))) {
                    setDirection(D_DOWN);
                    speed = default_speed;
                    moved = true;
                }
                else if (SDL_GameControllerGetButton(m_controller, SDL_GameControllerButton(player_keys.axis_left))) {
                    setDirection(D_LEFT);
                    speed = default_speed;
                    moved = true;
                }
                else if (SDL_GameControllerGetButton(m_controller, SDL_GameControllerButton(player_keys.axis_right))) {
                    setDirection(D_RIGHT);
                    speed = default_speed;
                    moved = true;
                }
            }
            
            // Se não moveu, para o tanque (exceto se está escorregando no gelo)
            if (!moved) {
                if(!testFlag(TSF_ON_ICE) || m_slip_time == 0)
                    speed = 0.0;
            }
            
            // --- Disparo ---
            if (player_keys.button_fire >= 0 &&
                SDL_GameControllerGetButton(
                    m_controller,
                    SDL_GameControllerButton(player_keys.button_fire)
                ) &&
                m_fire_time > AppConfig::player_reload_time)
            {
                fire();
                m_fire_time = 0;
            }
        }
        else if (player_keys.type == Player::InputType::Hybrid)
        {
            // Processa input híbrido: teclado + controle
            const Uint8 *key_state = SDL_GetKeyboardState(NULL);
            bool moved = false;
            bool keyboard_used = false;
            
            // --- Processa teclado primeiro (prioridade) ---
            if(key_state != nullptr)
            {
                if(key_state[player_keys.up])
                {
                    setDirection(D_UP);
                    speed = default_speed;
                    moved = true;
                    keyboard_used = true;
                }
                else if(key_state[player_keys.down])
                {
                    setDirection(D_DOWN);
                    speed = default_speed;
                    moved = true;
                    keyboard_used = true;
                }
                else if(key_state[player_keys.left])
                {
                    setDirection(D_LEFT);
                    speed = default_speed;
                    moved = true;
                    keyboard_used = true;
                }
                else if(key_state[player_keys.right])
                {
                    setDirection(D_RIGHT);
                    speed = default_speed;
                    moved = true;
                    keyboard_used = true;
                }

                // Disparo do teclado
                if(key_state[player_keys.fire] && m_fire_time > AppConfig::player_reload_time)
                {
                    fire();
                    m_fire_time = 0;
                }
            }
            
            // --- Processa controle apenas se o teclado não foi usado ---
            if(!keyboard_used && m_controller)
            {
                // --- Movimento usando analógicos ou D-pad ---
                // Verifica se são analógicos (valores negativos indicam analógicos)
                if (player_keys.axis_up < 0 || player_keys.axis_down < 0 || 
                    player_keys.axis_left < 0 || player_keys.axis_right < 0)
                {
                    // Processa analógicos
                    // Zona morta para input híbrido
                    const Sint16 DEADZONE = ANALOG_DEADZONE_HYBRID;
                    
                    // Lê os valores dos analógicos
                    Sint16 axis_y = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTY);
                    Sint16 axis_x = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTX);
                    
                    // Processa movimento vertical (eixo Y)
                    if (axis_y < -DEADZONE) {
                        setDirection(D_UP);
                        speed = default_speed;
                        moved = true;
                    }
                    else if (axis_y > DEADZONE) {
                        setDirection(D_DOWN);
                        speed = default_speed;
                        moved = true;
                    }
                    // Processa movimento horizontal (eixo X)
                    else if (axis_x < -DEADZONE) {
                        setDirection(D_LEFT);
                        speed = default_speed;
                        moved = true;
                    }
                    else if (axis_x > DEADZONE) {
                        setDirection(D_RIGHT);
                        speed = default_speed;
                        moved = true;
                    }
                }
                else
                {
                    // Processa D-pad (botões)
                    if (SDL_GameControllerGetButton(m_controller, SDL_GameControllerButton(player_keys.axis_up))) {
                        setDirection(D_UP);
                        speed = default_speed;
                        moved = true;
                    }
                    else if (SDL_GameControllerGetButton(m_controller, SDL_GameControllerButton(player_keys.axis_down))) {
                        setDirection(D_DOWN);
                        speed = default_speed;
                        moved = true;
                    }
                    else if (SDL_GameControllerGetButton(m_controller, SDL_GameControllerButton(player_keys.axis_left))) {
                        setDirection(D_LEFT);
                        speed = default_speed;
                        moved = true;
                    }
                    else if (SDL_GameControllerGetButton(m_controller, SDL_GameControllerButton(player_keys.axis_right))) {
                        setDirection(D_RIGHT);
                        speed = default_speed;
                        moved = true;
                    }
                }
                
                // Disparo do controle (sempre processado)
                if (player_keys.button_fire >= 0 &&
                    SDL_GameControllerGetButton(
                        m_controller,
                        SDL_GameControllerButton(player_keys.button_fire)
                    ) &&
                    m_fire_time > AppConfig::player_reload_time)
                {
                    fire();
                    m_fire_time = 0;
                }
            }
            
            // Se não moveu com nenhum input, para o tanque
            if (!moved) {
                if(!testFlag(TSF_ON_ICE) || m_slip_time == 0)
                    speed = 0.0;
            }
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

    // Usa o índice do tipo do jogador para buscar a posição correta
    int idx = static_cast<int>(type) - static_cast<int>(ST_PLAYER_1);
    if (idx >= 0 && idx < static_cast<int>(AppConfig::player_starting_point.size())) {
        pos_x = AppConfig::player_starting_point.at(idx).x;
        pos_y = AppConfig::player_starting_point.at(idx).y;
    }

    // Atualiza retângulo de destino do sprite
    dest_rect.x = pos_x;
    dest_rect.y = pos_y;
    dest_rect.h = m_sprite->rect.h;
    dest_rect.w = m_sprite->rect.w;

    setDirection(D_UP); // Sempre renasce apontando para cima
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

    // Se está no nível máximo de estrela, perde só uma estrela
    if(star_count == 3)
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

void Player::addLife() {
    lives_count++;
    SoundManager::getInstance().playSound("life");
}

void Player::shieldHit() {
    SoundManager::getInstance().playSound("shieldhit");
}

bool Player::isControllerActive(int controller_index) {
    int num_joysticks = SDL_NumJoysticks();
    if (controller_index < num_joysticks && SDL_IsGameController(controller_index)) {
        SDL_GameController* controller = SDL_GameControllerOpen(controller_index);
        if (controller) {
            bool is_attached = SDL_GameControllerGetAttached(controller);
            SDL_GameControllerClose(controller);
            return is_attached;
        }
    }
    return false;
}

void Player::adjustInputType(int player_index) {
    // Função simplificada - apenas verifica disponibilidade sem logs
    int num_joysticks = SDL_NumJoysticks();
    int available_controllers = 0;
    for (int i = 0; i < num_joysticks; ++i) {
        if (SDL_IsGameController(i)) {
            available_controllers++;
        }
    }
    // Função mantida para compatibilidade, mas sem logs ou alterações automáticas
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
