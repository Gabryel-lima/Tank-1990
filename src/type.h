#ifndef TYPE_H
#define TYPE_H

/**
 * @brief Enum que representa os diferentes tipos de sprites do jogo.
 * Cada valor corresponde a um sprite específico utilizado na renderização dos objetos.
 */
enum SpriteType
{
    ST_TANK_A,         // Tanque inimigo tipo A
    ST_TANK_B,         // Tanque inimigo tipo B
    ST_TANK_C,         // Tanque inimigo tipo C
    ST_TANK_D,         // Tanque inimigo tipo D

    ST_PLAYER_1,       // Sprite do jogador 1
    ST_PLAYER_2,       // Sprite do jogador 2
    ST_PLAYER_3,       // Sprite do jogador 3

    ST_BRICK_WALL,     // Parede de tijolos (destrutível)
    ST_STONE_WALL,     // Parede de pedra (indestrutível)
    ST_WATER,          // Água (obstáculo)
    ST_BUSH,           // Arbusto (esconde tanques)
    ST_ICE,            // Gelo (faz o tanque deslizar)

    // Sprites de bônus
    ST_BONUS_GRENADE,  // Bônus: granada (destrói todos os inimigos)
    ST_BONUS_HELMET,   // Bônus: capacete (escudo temporário)
    ST_BONUS_CLOCK,    // Bônus: relógio (congela inimigos)
    ST_BONUS_SHOVEL,   // Bônus: pá (protege a base)
    ST_BONUS_TANK,     // Bônus: vida extra
    ST_BONUS_STAR,     // Bônus: estrela (melhora o tanque)
    ST_BONUS_GUN,      // Bônus: arma (poder de fogo extra)
    ST_BONUS_BOAT,     // Bônus: barco (permite atravessar água)

    ST_SHIELD,         // Sprite de escudo protetor
    ST_CREATE,         // Animação de criação de tanque
    ST_DESTROY_TANK,   // Animação de destruição de tanque
    ST_DESTROY_BULLET, // Animação de destruição de projétil
    ST_BOAT_P1,        // Sprite de barco para o jogador 1
    ST_BOAT_P2,        // Sprite de barco para o jogador 2

    ST_EAGLE,          // Base (águia) do jogador
    ST_DESTROY_EAGLE,  // Animação de destruição da base
    ST_FLAG,           // Bandeira (indicador de fase)

    ST_BULLET,         // Sprite de projétil

    ST_LEFT_ENEMY,     // Indicador de inimigos restantes
    ST_STAGE_STATUS,   // Status da fase

    ST_TANKS_LOGO,     // Logo do jogo

    ST_NONE            // Nenhum sprite (valor padrão)
};

/**
 * @brief Enum de flags de estado do tanque.
 * Cada flag representa uma condição ou efeito especial aplicado ao tanque.
 * Pode ser usado como máscara de bits.
 */
enum TankStateFlag
{
    TSF_SHIELD      = 1 << 1, // Após pegar o capacete (escudo temporário)
    TSF_FROZEN      = 1 << 2, // Após o inimigo pegar o relógio (tanque congelado)
    TSF_DESTROYED   = 1 << 3, // Após ser atingido por bomba ou projétil (tanque destruído)
    TSF_BOAT        = 1 << 4, // Após pegar o barco (pode atravessar água)
    TSF_BONUS       = 1 << 5, // Ao destruir este tanque, aparece um bônus no mapa
    TSF_ON_ICE      = 1 << 6, // Se o tanque está sobre o gelo, desliza
    TSF_CREATE      = 1 << 7, // Em processo de criação (spawn)
    TSF_LIFE        = 1 << 8, // Flag de vida extra
    TSF_MENU        = 1 << 9  // Animação acelerada no menu
};

/**
 * @brief Enum para as direções possíveis de movimento.
 */
enum Direction
{
    D_UP = 0,     // Cima
    D_RIGHT = 1,  // Direita
    D_DOWN = 2,   // Baixo
    D_LEFT = 3    // Esquerda
};

#endif // TYPE_H
