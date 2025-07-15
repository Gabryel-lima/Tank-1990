#ifndef BONUS_H
#define BONUS_H

#include "object.h"

/**
 * @brief Classe responsável por exibir e controlar o bônus no jogo.
 */
class Bonus : public Object {
public:
    /**
     * Cria um bônus padrão (estrela) na posição (0, 0).
     */
    Bonus();
    /**
     * Cria um bônus em uma posição e tipo específicos.
     * @param x - posição horizontal inicial
     * @param y - posição vertical inicial
     * @param type - tipo do bônus
     */
    Bonus(double x, double y, SpriteType type);
    /**
     * Desenha o bônus (considerando efeito de piscar).
     */
    void draw();
    /**
     * Atualiza a animação do bônus e controla o tempo de exibição.
     * @param dt - tempo desde a última atualização
     */
    void update(Uint32 dt);
private:
    /**
     * Tempo desde a criação do bônus.
     */
    Uint32 m_bonus_show_time;
    /**
     * Indica se o bônus está visível (para efeito de piscar).
     */
    bool m_show;
};

#endif // BONUS_H 