#ifndef ENEMY_H
#define ENEMY_H

#include "tank.h"

/**
 * @brief Classe responsável pelo comportamento dos tanques inimigos.
 * Gerencia movimentação, lógica de tiro e interação com o alvo.
 */
class Enemy : public Tank
{
public:
    /**
     * Construtor padrão: cria um inimigo na primeira posição de spawn definida.
     * @see AppConfig::enemy_starting_point
     */
    Enemy();

    /**
     * Construtor parametrizado: cria um inimigo em uma posição e tipo específicos.
     * @param x - posição horizontal inicial do inimigo
     * @param y - posição vertical inicial do inimigo
     * @param type - tipo do sprite do tanque inimigo
     */
    Enemy(double x, double y, SpriteType type);

    /**
     * Desenha o tanque inimigo na tela.
     * Se a flag @a AppConfig::show_enemy_target estiver ativada, desenha uma linha até o alvo do inimigo.
     */
    void draw();

    /**
     * Atualiza a posição do tanque inimigo, decide direção e momento do próximo disparo
     * de acordo com o tipo do inimigo.
     * @param dt - tempo (em ms) desde a última atualização
     */
    void update(Uint32 dt);

    /**
     * Reduz o nível de armadura em 1. Se chegar a zero, o tanque é destruído (explode).
     */
    void destroy();

    /**
     * Retorna a quantidade de pontos que o jogador recebe ao destruir este inimigo.
     * @return pontos
     */
    unsigned scoreForHit();

    /**
     * Posição alvo para onde o tanque inimigo está se dirigindo.
     */
    SDL_Point target_position;

private:
    /**
     * Tempo desde a última mudança de direção (em ms).
     */
    Uint32 m_direction_time;

    /**
     * Tempo durante o qual o tanque mantém a direção atual antes de mudar (em ms).
     */
    Uint32 m_keep_direction_time;

    /**
     * Tempo desde a última tentativa de retomar o movimento (em ms).
     */
    Uint32 m_speed_time;

    /**
     * Tempo até a próxima tentativa de retomar o movimento (em ms); define quando a velocidade será diferente de zero.
     */
    Uint32 m_try_to_go_time;

    /**
     * Tempo desde a última tentativa de disparar um projétil (em ms).
     */
    Uint32 m_fire_time;

    /**
     * Tempo até a próxima tentativa de disparo (em ms).
     */
    Uint32 m_reload_time;
};

#endif // ENEMY_H
