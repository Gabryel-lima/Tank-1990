#ifndef BRICK_H
#define BRICK_H

#include "object.h"

/**
 * @brief Classe responsável por um bloco de parede (tijolo).
 */
class Brick : public Object {
public:
    /**
     * Cria um tijolo na posição (0, 0).
     */
    Brick();
    /**
     * Cria um tijolo em uma posição específica.
     * @param x - posição horizontal inicial
     * @param y - posição vertical inicial
     */
    Brick(double x, double y);
    /**
     * Atualização do tijolo (não faz nada, pois o tijolo é estático).
     * @param dt - tempo desde a última atualização
     */
    void update(Uint32 dt);
    /**
     * Altera o estado do tijolo após ser atingido por um projétil.
     * @param bullet_direction - direção do projétil
     */
    void bulletHit(Direction bullet_direction);
private:
    /**
     * Quantidade de vezes que o tijolo foi atingido.
     */
    int m_collision_count;
    /**
     * Código do estado atual do tijolo (define qual parte está destruída).
     */
    int m_state_code;
};

#endif // BRICK_H 