#ifndef EAGLE_H
#define EAGLE_H

#include "object.h"

/**
 * @brief Classe responsável por representar a águia (base) que os jogadores devem proteger e os inimigos tentar destruir.
 * Herda de Object e implementa a lógica de animação e destruição da águia.
 */
class Eagle : public Object
{
public:
    /**
     * Construtor padrão: cria a águia na posição (0, 0).
     * Útil para inicialização rápida ou testes.
     */
    Eagle();

    /**
     * Construtor parametrizado: cria a águia em uma posição específica.
     * @param x - posição horizontal inicial da águia
     * @param y - posição vertical inicial da águia
     */
    Eagle(double x, double y);

    /**
     * Atualiza o estado e a animação da águia.
     * Se estiver em processo de destruição, avança os frames da animação.
     * @param dt - tempo (em ms) desde a última atualização
     */
    void update(Uint32 dt);

    /**
     * Inicia a animação de destruição da águia.
     * Altera o sprite e o estado para indicar que a base foi destruída.
     */
    void destroy();
};

#endif // EAGLE_H
