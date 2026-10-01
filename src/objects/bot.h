#ifndef BOT_H
#define BOT_H

#include "enemy.h"

/**
 * @brief Tanque controlado pela CPU no modo duelo.
 *
 * Reaproveita a inteligência do Enemy (perseguir target_position e atirar),
 * mas com as regras de um jogador: tem vidas (renascimentos) em vez de
 * blindagem, renasce no ponto da sua equipe com escudo temporário e é
 * desenhado com o sprite prateado tingido com a cor da equipe.
 */
class Bot : public Enemy
{
public:
    /**
     * Papel do bot na equipe.
     */
    enum Role
    {
        ROLE_ATTACK, ///< Vai para a base inimiga e caça quem estiver por perto
        ROLE_DEFEND  ///< Fica perto da própria base e caça invasores
    };

    /**
     * @param spawn - ponto de renascimento
     * @param type - sprite do tanque inimigo (define a forma de atirar do Enemy)
     * @param team - equipe (0 = A, 1 = B)
     * @param lives - quantidade de vidas
     * @param team_color - cor aplicada ao sprite
     * @param role - papel do bot na equipe
     */
    Bot(SDL_Point spawn, SpriteType type, int team, int lives, SDL_Color team_color, Role role);

    /**
     * Atualiza a IA do Enemy e corrige o sprite para a cor da equipe.
     */
    void update(Uint32 dt);

    /**
     * Destrói o tanque, exceto se estiver com escudo.
     */
    void destroy();

    /**
     * Gasta uma vida e renasce no ponto da equipe com escudo temporário.
     */
    void respawn();

    /**
     * Papel do bot na equipe.
     */
    Role role;
};

#endif // BOT_H
