#ifndef BULLET_H
#define BULLET_H

#include "object.h"

/**
 * @brief Classe responsável pelos projéteis disparados pelos tanques.
 * Herda de Object e implementa a lógica de movimento, colisão e dano do projétil.
 */
class Bullet : public Object
{
public:
    /**
     * Construtor padrão: cria um projétil na posição (0, 0).
     * Útil para inicialização rápida ou testes.
     */
    Bullet();

    /**
     * Construtor parametrizado: cria um projétil em uma posição específica.
     * @param x - posição horizontal inicial do projétil
     * @param y - posição vertical inicial do projétil
     */
    Bullet(double x, double y);

    /**
     * Atualiza a posição do projétil.
     * Move o projétil de acordo com sua velocidade e direção.
     * @param dt - tempo (em ms) desde a última atualização
     */
    void update(Uint32 dt);

    /**
     * Ativa a animação de explosão do projétil.
     * Marca o projétil para remoção após a animação.
     */
    void destroy();
    
    /**
     * Velocidade de deslocamento do projétil (pixels/ms).
     */
    double speed;

    /**
     * Indica se o projétil colidiu com algum objeto.
     * Quando verdadeiro, o projétil para de se mover e inicia a animação de explosão.
     */
    bool collide;

    /**
     * Indica se o projétil possui dano aumentado.
     * Projéteis com dano aumentado podem destruir paredes de pedra e arbustos.
     */
    bool increased_damage;

    /**
     * Direção de movimento do projétil.
     */
    Direction direction;
};

#endif // BULLET_H
