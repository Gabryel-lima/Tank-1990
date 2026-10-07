#ifndef TURRET_H
#define TURRET_H

#include "tank.h"

#include <functional>

/**
 * @brief Torreta dos modos extras: canhão parado que atira sozinho até ser destruída (sem
 * tempo nem munição), a cada AppConfig::power_turret_reload ms.
 *
 * É um tanque que não anda (colide, leva tiro e explode como os outros). A cada quadro o
 * modo de jogo chama think() com a regra de quando vale atirar numa direção (inimigo
 * alinhado, sem a própria base no caminho...), e a torreta vira e atira.
 */
class Turret : public Tank
{
public:
    /**
     * @param x, y - canto superior esquerdo (alinhado à grade), em pixels
     * @param team - equipe dona (no duelo; na sobrevivência, -1)
     * @param owner - índice do jogador que a colocou, ou -1
     * @param color - cor de quem colocou (o sprite é cinza e recebe essa cor)
     */
    Turret(double x, double y, int team, int owner, SDL_Color color);

    void update(Uint32 dt) override;

    /** Explode como um tanque (a explosão grande), já no primeiro quadro. */
    void destroy() override;

    /**
     * Escolhe a direção (a atual primeiro) em que @a worth diz que vale atirar, vira para ela
     * e atira quando a recarga permitir.
     */
    void think(const std::function<bool(Direction)>& worth);

    int owner;

    /**
     * Onde o tiro fere a torreta: o corpo, sem o cano nem o vão dos lados dele (o retângulo
     * de colisão é o do tanque, 28x28, e incluía o vazio ao lado do cano). Viva, segue a
     * direção do cano; sem vida, vazio.
     */
    SDL_Rect hitRect() const;

private:
    Uint32 m_reload_left;
};

#endif // TURRET_H
