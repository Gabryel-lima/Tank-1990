#ifndef MINE_H
#define MINE_H

#include "object.h"

/**
 * @brief Mina dos modos extras: fica no chão e explode o tanque inimigo que passar por cima.
 *
 * Quem decide quem é inimigo, o que a explosão faz e o que acontece com um tiro na mina é o
 * modo de jogo (Game/Duel). A mina só cuida de piscar: fica até explodir (sem tempo).
 */
class Mine : public Object
{
public:
    /**
     * @param center - centro da mina (o centro do tanque que a soltou), em pixels
     * @param team - equipe de quem soltou (o duelo usa; na sobrevivência, -1)
     * @param owner - índice do jogador que soltou (para os pontos), ou -1
     * @param color - cor de quem soltou (a mina é cinza e recebe essa cor)
     */
    Mine(SDL_Point center, int team, int owner, SDL_Color color);

    void update(Uint32 dt) override;

    /** Explode (encostou num inimigo ou levou um tiro): some do mapa. */
    void detonate();

    int team;
    int owner;
};

#endif // MINE_H
