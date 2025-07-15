#ifndef TYPE_H
#define TYPE_H

/**
 * @brief Enumeração dos tipos de sprites usados no jogo.
 */
enum SpriteType {
    ST_TANK_A,
    ST_TANK_B,
    ST_TANK_C,
    ST_TANK_D,
    ST_PLAYER_1,
    ST_PLAYER_2,
    ST_BRICK_WALL,
    ST_STONE_WALL,
    ST_WATER,
    ST_BUSH,
    ST_ICE,
    ST_BONUS_GRENADE,
    ST_BONUS_HELMET,
    ST_BONUS_CLOCK,
    ST_BONUS_SHOVEL,
    ST_BONUS_TANK,
    ST_BONUS_STAR,
    ST_BONUS_GUN,
    ST_BONUS_BOAT,
    ST_SHIELD,
    ST_CREATE,
    ST_DESTROY_TANK,
    ST_DESTROY_BULLET,
    ST_BOAT_P1,
    ST_BOAT_P2,
    ST_EAGLE,
    ST_DESTROY_EAGLE,
    ST_FLAG,
    ST_BULLET,
    ST_LEFT_ENEMY,
    ST_STAGE_STATUS,
    ST_TANKS_LOGO,
    ST_NONE
};

/**
 * @brief Flags de estado dos tanques (bitmask).
 */
enum TankStateFlag {
    TSF_SHIELD = 1 << 1,      // Escudo ativado
    TSF_FROZEN = 1 << 2,      // Congelado (após bônus relógio)
    TSF_DESTROYED = 1 << 3,   // Destruído
    TSF_BOAT = 1 << 4,        // Pode atravessar água
    TSF_BONUS = 1 << 5,       // Ao destruir, gera bônus
    TSF_ON_ICE = 1 << 6,      // Está sobre o gelo
    TSF_CREATE = 1 << 7,      // Em processo de criação
    TSF_LIFE = 1 << 8,        // Possui vida extra
    TSF_MENU = 1 << 9         // Animação acelerada no menu
};

/**
 * @brief Direções possíveis para tanques e projéteis.
 */
enum Direction {
    D_UP = 0,
    D_RIGHT = 1,
    D_DOWN = 2,
    D_LEFT = 3
};

#endif // TYPE_H 