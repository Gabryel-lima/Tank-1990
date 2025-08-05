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
    virtual ~Player();
    /**
     * @brief Estrutura que armazena os códigos das teclas responsáveis pelo controle do tanque do jogador.
     * Permite configurar quais teclas controlam cada direção e o disparo.
     */
    enum class InputType {
        Keyboard,
        Controller,
        Hybrid  // Suporte para teclado + controle simultaneamente
    };
    
    struct PlayerKeys
    {
        InputType type;

        // Para teclado
        SDL_Scancode up;
        SDL_Scancode down;
        SDL_Scancode left;
        SDL_Scancode right;
        SDL_Scancode fire;

        // Para controle
        int axis_up;
        int axis_down;
        int axis_left;
        int axis_right;
        int button_fire;

        // Construtor para teclado
        PlayerKeys(SDL_Scancode u, SDL_Scancode d, SDL_Scancode l, SDL_Scancode r, SDL_Scancode f)
            : type(InputType::Keyboard), up(u), down(d), left(l), right(r), fire(f),
              axis_up(-1), axis_down(-1), axis_left(-1), axis_right(-1), button_fire(-1) {}

        // Construtor para controle
        PlayerKeys(int a_up, int a_down, int a_left, int a_right, int b_fire)
            : type(InputType::Controller), up(SDL_SCANCODE_UNKNOWN), down(SDL_SCANCODE_UNKNOWN),
              left(SDL_SCANCODE_UNKNOWN), right(SDL_SCANCODE_UNKNOWN), fire(SDL_SCANCODE_UNKNOWN),
              axis_up(a_up), axis_down(a_down), axis_left(a_left), axis_right(a_right), button_fire(b_fire) {}

        // Construtor para input híbrido (teclado + controle)
        PlayerKeys(SDL_Scancode k_up, SDL_Scancode k_down, SDL_Scancode k_left, SDL_Scancode k_right, SDL_Scancode k_fire,
                   int a_up, int a_down, int a_left, int a_right, int b_fire)
            : type(InputType::Hybrid), up(k_up), down(k_down), left(k_left), right(k_right), fire(k_fire),
              axis_up(a_up), axis_down(a_down), axis_left(a_left), axis_right(a_right), button_fire(b_fire) {}

        // Construtor padrão
        PlayerKeys()
            : type(InputType::Keyboard), up(SDL_SCANCODE_UNKNOWN), down(SDL_SCANCODE_UNKNOWN),
              left(SDL_SCANCODE_UNKNOWN), right(SDL_SCANCODE_UNKNOWN), fire(SDL_SCANCODE_UNKNOWN),
              axis_up(-1), axis_down(-1), axis_left(-1), axis_right(-1), button_fire(-1) {}
    };

    /**
     * Construtor padrão.
     * Cria o jogador na posição inicial definida em AppConfig.
     * @param keys - configuração de teclas/controle
     * @param idx - índice do controle
     */
    Player(const PlayerKeys& keys, int idx);

    /**
     * Construtor parametrizado.
     * Cria o tanque do jogador em uma posição e tipo específicos.
     * @param x - posição horizontal inicial
     * @param y - posição vertical inicial
     * @param type - tipo do sprite do jogador
     * @param idx - índice do controle
     */
    Player(double x, double y, SpriteType type, int idx);

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

    /**
     * Adiciona uma vida ao jogador.
     * Incrementa o contador de vidas do jogador.
     */
    void addLife();

    /**
     * Notifica que o escudo do jogador foi atingido.
     * Pode ser usado para efeitos visuais ou lógicos ao receber impacto com escudo ativo.
     */
    void shieldHit();

private:
    /**
     * Ponteiro para o controle do jogador (caso use controle).
     */
    SDL_GameController* m_controller = nullptr;
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
