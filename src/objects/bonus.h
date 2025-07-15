#ifndef BONUS_H
#define BONUS_H

#include "object.h"

/**
 * @brief Classe responsável por exibir e gerenciar o bônus no jogo.
 * Herda de Object e adiciona lógica de tempo de exibição e piscar.
 */
class Bonus : public Object
{
public:
    /**
     * Construtor padrão: cria um bônus do tipo estrela na posição (0, 0).
     * Útil para inicialização rápida ou testes.
     */
    Bonus();

    /**
     * Construtor parametrizado: cria um bônus em uma posição e tipo específicos.
     * @param x - posição horizontal inicial do bônus
     * @param y - posição vertical inicial do bônus
     * @param type - tipo do bônus (ex: estrela, granada, capacete, etc)
     */
    Bonus(double x, double y, SpriteType type);

    /**
     * Desenha o bônus na tela, se estiver visível.
     * Chama o método draw da classe base Object.
     */
    void draw();

    /**
     * Atualiza a animação e o estado do bônus.
     * Controla o tempo de exibição, define quando o bônus deve ser removido
     * e aumenta a frequência do piscar quando está próximo de sumir.
     * @param dt - tempo (em ms) desde a última atualização
     */
    void update(Uint32 dt);

private:
    /**
     * Tempo (em ms) desde a criação do bônus.
     * Usado para controlar duração e frequência do piscar.
     */
    Uint32 m_bonus_show_time;

    /**
     * Indica se o bônus está visível no momento.
     * Usado para efeito de piscar antes de sumir.
     */
    bool m_show;
};

#endif // BONUS_H
