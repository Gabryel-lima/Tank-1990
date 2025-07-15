#ifndef BRICK_H
#define BRICK_H

#include "object.h"

/**
 * @brief Classe responsável por representar um único bloco de parede de tijolo.
 * Herda de Object e implementa a lógica de dano e colisão do tijolo.
 */
class Brick : public Object
{
public:
    /**
     * Construtor padrão: cria um tijolo na posição (0, 0).
     * Útil para inicialização rápida ou testes.
     */
    Brick();

    /**
     * Construtor parametrizado: cria um tijolo em uma posição específica.
     * @param x - posição horizontal inicial do tijolo
     * @param y - posição vertical inicial do tijolo
     */
    Brick(double x, double y);

    /**
     * Atualiza o estado do tijolo.
     * Esta função foi implementada para não alterar o collision_rect,
     * pois o tijolo é estático e não precisa de atualização por frame.
     * @param dt - tempo (em ms) desde a última atualização
     */
    void update(Uint32 dt);

    /**
     * Altera o estado do tijolo ao ser atingido por um projétil.
     * Modifica o collision_rect para refletir o dano sofrido.
     * @param bullet_direction - direção do projétil que atingiu o tijolo
     */
    void bulletHit(Direction bullet_direction);

private:
    /**
     * Número de vezes que o tijolo foi atingido por projéteis.
     * Usado para determinar o estado atual do bloco e sua destruição.
     */
    int m_collision_count;

    /**
     * Código de estado do tijolo.
     * Representa um dos dez possíveis estados de dano/forma do bloco.
     * Utilizado para controlar a renderização e colisão parcial.
     */
    int m_state_code;
};

#endif // BRICK_H
