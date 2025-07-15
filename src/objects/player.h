#ifndef PLAYER_H
#define PLAYER_H

#include "tank.h"

/**
 * @brief Classe responsável pelo comportamento dos tanques controlados pelo jogador.
 * Herda de Tank e adiciona lógica de input, pontuação e evolução do jogador.
 */
class Player : public Tank
{
public:

    /**
     * @brief Estrutura que armazena os códigos das teclas responsáveis pelo controle do tanque do jogador.
     * Permite configurar quais teclas controlam cada direção e o disparo.
     */
    struct PlayerKeys
    {
        PlayerKeys(): up(SDL_SCANCODE_UNKNOWN), down(SDL_SCANCODE_UNKNOWN), left(SDL_SCANCODE_UNKNOWN), right(SDL_SCANCODE_UNKNOWN), fire(SDL_SCANCODE_UNKNOWN) {}
        PlayerKeys(SDL_Scancode u, SDL_Scancode d, SDL_Scancode l, SDL_Scancode r, SDL_Scancode f): up(u), down(d), left(l), right(r), fire(f) {}
        /**
         * Tecla para mover para cima.
         */
        SDL_Scancode up;
        /**
         * Tecla para mover para baixo.
         */
        SDL_Scancode down;
        /**
         * Tecla para mover para a esquerda.
         */
        SDL_Scancode left;
        /**
         * Tecla para mover para a direita.
         */
        SDL_Scancode right;
        /**
         * Tecla para disparar projétil.
         */
        SDL_Scancode fire;
    };

    /**
     * Construtor padrão.
     * Cria o jogador na primeira posição inicial definida em AppConfig.
     * @see AppConfig::player_starting_point
     */
    Player();

    /**
     * Construtor parametrizado.
     * Cria o tanque do jogador em uma posição e tipo específicos.
     * @param x - posição horizontal inicial
     * @param y - posição vertical inicial
     * @param type - tipo do sprite do jogador
     */
    Player(double x, double y, SpriteType type);

    /**
     * Atualiza o estado do jogador.
     * Responsável por atualizar a animação do tanque, verificar o estado das teclas pressionadas
     * e reagir aos comandos de movimento e disparo.
     * @param dt - tempo (em ms) desde a última chamada, usado para controlar a animação
     */
    void update(Uint32 dt);

    /**
     * Realiza o respawn do jogador.
     * Remove uma vida, limpa todas as flags e inicia a animação de renascimento do tanque.
     */
    void respawn();

    /**
     * Destrói o tanque do jogador.
     * Inicia a animação de explosão caso o tanque não possua escudo, barco ou três estrelas.
     */
    void destroy();

    /**
     * Cria um projétil se ainda não atingiu o limite máximo.
     * Aumenta a velocidade do projétil se o jogador tiver pelo menos uma estrela
     * e adiciona dano extra se o jogador tiver três estrelas.
     * @return ponteiro para o projétil criado, ou nullptr se não for possível criar
     */
    Bullet* fire();

    /**
     * Altera a quantidade de estrelas do jogador.
     * Se o número de estrelas for maior que zero, aumenta a velocidade padrão do tanque.
     * Para duas ou mais estrelas e para cada incremento positivo, aumenta o número máximo de projéteis.
     * @param c - variação na quantidade de estrelas (pode ser negativa)
     */
    void changeStarCountBy(int c);

    /**
     * Teclas de controle do jogador atual.
     * Permite customizar o input de cada jogador.
     */
    PlayerKeys player_keys;

    /**
     * Pontuação atual do jogador.
     */
    unsigned score;

private:
    /**
     * Quantidade atual de estrelas do jogador; varia de 0 a 3.
     * Estrelas aumentam habilidades do tanque.
     */
    int star_count;

    /**
     * Tempo decorrido desde o último disparo do jogador.
     * Usado para controlar o tempo de recarga.
     */
    Uint32 m_fire_time;
};

#endif // PLAYER_H
