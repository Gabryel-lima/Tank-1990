#include "player.h"
#include "../appconfig.h"
#include "../soundmanager.h"
#include "../controllers.h"

#include <algorithm>
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
        bool up = false, down = false, left = false, right = false, shoot = false, power = false;
        bool shop_left = false, shop_right = false; // navegação na loja (sobrevivência)

        // Jogador do computador: a IA do modo de jogo decide no lugar do teclado
        if(cpu)
        {
            up    = cpu_command.move && cpu_command.direction == D_UP;
            down  = cpu_command.move && cpu_command.direction == D_DOWN;
            left  = cpu_command.move && cpu_command.direction == D_LEFT;
            right = cpu_command.move && cpu_command.direction == D_RIGHT;
            shoot = cpu_command.fire;
            power = cpu_command.use_power;
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
            power = key_state[keys->power];
            shop_right = power; // no teclado, o botão de poder avança na loja
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
            // LB usa o poder guardado (modos extras); na loja, LB e RB escolhem o item
            bool lb = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
            bool rb = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
            power = power || lb;
            shop_left = lb;
            shop_right = shop_right || rb;
        }

        // Movimentação: uma direção por vez, na ordem cima/baixo/esquerda/direita
        if(up)         setDirection(D_UP);
        else if(down)  setDirection(D_DOWN);
        else if(left)  setDirection(D_LEFT);
        else if(right) setDirection(D_RIGHT);

        // Botão de poder: conta só o momento em que é apertado
        if(power && !m_power_down) m_power_pressed = true;
        m_power_down = power;
        if(shop_left && !m_shop_left_down) m_shop_step--;
        if(shop_right && !m_shop_right_down) m_shop_step++;
        m_shop_left_down = shop_left;
        m_shop_right_down = shop_right;

        if(up || down || left || right)
            speed = default_speed * (m_turbo_time > 0 ? AppConfig::power_turbo_factor : 1.0);
        else if(!testFlag(TSF_ON_ICE) || m_slip_time == 0)
            speed = 0.0; // Para o tanque, exceto se estiver escorregando no gelo

        // Disparo: respeita o tempo de recarga. O relógio só volta a zero quando sai um tiro:
        // antes, a tentativa que falhava (bala ainda na tela) também o zerava, e o tiro seguinte
        // esperava uma recarga inteira a mais, um atraso que variava conforme o encaixe
        if(shoot && !m_fire_down) m_fire_pressed = true;
        m_fire_down = shoot;
        if(!shop_mode && shoot && m_fire_time > m_reload_time && fire() != nullptr)
            m_fire_time = 0;
    }

    m_fire_time += dt; // Atualiza tempo desde o último tiro
    m_turbo_time = m_turbo_time > dt ? m_turbo_time - dt : 0;

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

    m_turbo_time = 0; // o turbo acaba com a morte (o poder guardado, não)

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

    // Com estrela, o tiro só rebaixa o tanque um estágio (pesado → médio → leve → básico)
    // em vez de destruí-lo; o modo de jogo pode desligar essa "armadura" (o duelo desliga)
    if(star_count > 0 && star_armor)
        changeStarCountBy(-1);
    else
    {
        // Perde três estrelas e os poderes guardados (guardar tem risco) e chama a
        // destruição da classe base
        changeStarCountBy(-3);
        held_power = ST_NONE;
        power_stock.clear();
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
        b->from_player = true;
        // Se tem pelo menos uma estrela, aumenta a velocidade do tiro
        if(star_count > 0) b->speed = AppConfig::bullet_default_speed * 1.3;
        // Se está no nível máximo, o tiro quebra pedra
        if(star_count == 3) b->increased_damage = true;
        b->demolisher = m_demolisher;
    }
    return b;
}

unsigned Player::bulletsInUse() const
{
    return static_cast<unsigned>(std::count_if(bullets.begin(), bullets.end(), [](const Bullet* b) { return !b->collide; }));
}

void Player::drawEffects()
{
    // Brilho rápido (120 ms a cada 1 s): diferente da névoa contínua de quem está acabando
    if(m_demolisher && testFlag(TSF_LIFE) && demolisherGlint(m_effect_time))
        Engine::getEngine().getRenderer()->drawWhite(&src_rect, &dest_rect, DEMOLISHER_GLINT_ALPHA);
    // Turbo acabando: a névoa branca de tudo que está acabando (V1)
    if(testFlag(TSF_LIFE)) drawHaze(turboEnding());
}

SpriteType Player::activePower(double* ending) const
{
    if(m_turbo_time > 0)
    {
        if(ending != nullptr) *ending = turboEnding();
        return ST_BONUS_TURBO;
    }
    if(ending != nullptr) *ending = 0;
    return ST_NONE;
}

double Player::turboEnding() const
{
    double quarter = AppConfig::power_turbo_time / 4.0;
    if(m_turbo_time == 0 || quarter <= 0 || m_turbo_time >= quarter) return 0;
    return 1.0 - m_turbo_time / quarter;
}

// Altera o número de estrelas (power-up) do jogador.
// Ajusta velocidade, quantidade de balas e limita o valor.
//   0 estrelas: 1 tiro por vez
//   1 estrela:  tiro e tanque mais rápidos
//   2 estrelas: 2 tiros por vez
//   3 estrelas: 3 tiros por vez, e o tiro quebra pedra
void Player::changeStarCountBy(int c)
{
    star_count += c;
    if(star_count > 3) star_count = 3;
    else if(star_count < 0) star_count = 0;
    if(star_count < 3) m_demolisher = false;

    // O limite de balas sai só do estágio atual, subindo ou descendo: antes ele era somado
    // e zerado para 2, e o mesmo estágio dava limites diferentes conforme o caminho (quem
    // morria renascia com 2 balas, o começo da partida tinha 1)
    m_bullet_max_size = std::max(AppConfig::player_bullet_max_size, static_cast<unsigned>(star_count));

    // Se tem pelo menos uma estrela, aumenta a velocidade padrão
    if(star_count > 0) default_speed = AppConfig::tank_default_speed * 1.3;
    else default_speed = AppConfig::tank_default_speed;
}

void Player::setReloadTime(Uint32 ms)
{
    m_reload_time = ms;
}

int Player::takeShopStep()
{
    int step = m_shop_step;
    m_shop_step = 0;
    return step;
}

bool Player::takeFirePress()
{
    bool pressed = m_fire_pressed;
    m_fire_pressed = false;
    return pressed;
}

bool Player::takePowerPress()
{
    bool pressed = m_power_pressed;
    m_power_pressed = false;
    return pressed;
}

void Player::boost(Uint32 ms)
{
    m_turbo_time = ms;
}

void Player::teleport(double x, double y)
{
    bool boat = testFlag(TSF_BOAT);
    pos_x = x;
    pos_y = y;
    setDirection(team == 1 ? D_DOWN : D_UP);
    cpu_command = TankCommand();
    cpu_command.direction = direction;
    Tank::respawn();               // animação de nascimento, sem gastar vida
    if(boat) setFlag(TSF_BOAT);    // o barco continua
    SoundManager::getInstance().playSound("bonus");
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

int Player::storedPowers() const
{
    return (held_power != ST_NONE ? 1 : 0) + static_cast<int>(power_stock.size());
}

void Player::storePower(SpriteType type)
{
    if(held_power == ST_NONE) held_power = type;
    else power_stock.push_back(type);
}

void Player::consumeHeldPower()
{
    if(power_stock.empty()) held_power = ST_NONE;
    else
    {
        held_power = power_stock.front();
        power_stock.erase(power_stock.begin());
    }
}
