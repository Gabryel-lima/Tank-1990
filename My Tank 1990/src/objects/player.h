#ifndef PLAYER_H
#define PLAYER_H

#include "tank.h"

/**
 * @brief Classe responsável pelos tanques dos jogadores.
 */
class Player : public Tank {
public:
    /**
     * @brief Estrutura que armazena as teclas responsáveis pelo controle do tanque do jogador.
     */
    struct PlayerKeys {
        PlayerKeys(): up(SDL_SCANCODE_UNKNOWN), down(SDL_SCANCODE_UNKNOWN), left(SDL_SCANCODE_UNKNOWN), right(SDL_SCANCODE_UNKNOWN), fire(SDL_SCANCODE_UNKNOWN) {}
        PlayerKeys(SDL_Scancode u, SDL_Scancode d, SDL_Scancode l, SDL_Scancode r, SDL_Scancode f): up(u), down(d), left(l), right(r), fire(f) {}
        /**
         * Tecla responsável por mover para cima.
         */
        SDL_Scancode up;
        /**
         * Tecla responsável por mover para baixo.
         */
        SDL_Scancode down;
        /**
         * Tecla responsável por mover para a esquerda.
         */
        SDL_Scancode left;
        /**
         * Tecla responsável por mover para a direita.
         */
        SDL_Scancode right;
        /**
         * Tecla responsável por disparar o projétil.
         */
        SDL_Scancode fire;
    };

    /**
     * Cria o jogador na primeira das posições iniciais dos jogadores.
     * @see AppConfig::player_starting_point
     */
    Player();
    /**
     * Cria o tanque do jogador
     * @param x - posição inicial horizontal
     * @param y - posição inicial vertical
     * @param type - tipo do jogador
     */
    Player(double x, double y, SpriteType type);


    /**
     * Função responsável por atualizar a animação do tanque do jogador e por verificar o estado das teclas pressionadas, reagindo àquelas que controlam o tanque do jogador.
     * @param dt - tempo desde a última chamada da função, utilizado para atualizar a animação
     */
    void update(Uint32 dt);
    /**
     * Função responsável por remover uma vida, limpar todas as flags e iniciar a animação de renascimento do tanque.
     */
    void respawn();
    /**
     * Função responsável por iniciar a animação de explosão do tanque caso ele não tenha escudo, barco ou três estrelas.
     */
    void destroy();
    /**
     * Função responsável por criar um projétil caso ainda não tenha sido criado o número máximo permitido,
     * atribuir maior velocidade ao projétil se o jogador tiver pelo menos uma estrela e aumentar o dano se o jogador tiver três estrelas.
     * @return ponteiro para o projétil criado, ou nullptr caso não tenha sido criado
     */
    Bullet* fire();

    /**
     * Função que altera o número de estrelas atualmente possuídas. Com pelo menos uma estrela, a velocidade padrão do tanque é aumentada,
     * e para mais de uma estrela, bem como para cada valor positivo de @a c, o número máximo de projéteis é aumentado.
     * @param c - alteração no número de estrelas, pode ser negativo
     */
    void changeStarCountBy(int c);

    /**
     * Teclas de controle do jogador atual.
     */
    PlayerKeys player_keys;
    /**
     * Pontuação atual do jogador.
     */
    unsigned score;

private:
    /**
     * Número atual de estrelas; pode variar no intervalo [0, 3].
     */
    int star_count;
    /**
     * Tempo decorrido desde o último disparo de projétil.
     */
    Uint32 m_fire_time;
};

#endif // PLAYER_H
