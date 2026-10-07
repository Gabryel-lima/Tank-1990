#ifndef TURRET_H
#define TURRET_H

#include "tank.h"

#include <functional>

/**
 * @brief Torreta dos modos extras: canhão parado que atira sozinho até acabar o tempo
 * ou a munição (AppConfig::power_turret_time e power_turret_ammo).
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

    /** Tiros que ainda restam. */
    int ammo() const { return m_ammo; }

    /**
     * Sobrevivência: fica até ser destruída (sem tempo nem munição contados, sem a névoa de
     * "acabando") e atira a cada @a reload ms.
     */
    void setPermanent(Uint32 reload);

    /**
     * Onde o tiro fere a torreta: o corpo, sem o cano nem o vão dos lados dele (o retângulo
     * de colisão é o do tanque, 28x28, e incluía o vazio ao lado do cano). Viva, segue a
     * direção do cano; sem vida, vazio.
     */
    SDL_Rect hitRect() const;

protected:
    /** Névoa branca quando o tempo ou a munição estão acabando. */
    void drawEffects() override;

private:
    Uint32 m_time_left;
    Uint32 m_reload_left;
    Uint32 m_reload;
    int m_ammo;
    bool m_permanent = false;
};

#endif // TURRET_H
