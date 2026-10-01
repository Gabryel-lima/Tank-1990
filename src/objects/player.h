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
     * Zona morta do analógico esquerdo (~25% do curso de -32768 a +32767).
     */
    static const Sint16 ANALOG_DEADZONE = 8192;

    virtual ~Player();

    /**
     * @brief Um layout de teclado (ver AppConfig::keyboard_layouts).
     * Qual jogador usa qual layout é decidido pela classe Controllers:
     * o teclado é a reserva de quem não tem controle.
     */
    struct PlayerKeys
    {
        SDL_Scancode up;
        SDL_Scancode down;
        SDL_Scancode left;
        SDL_Scancode right;
        SDL_Scancode fire;
        const char* name; ///< nome mostrado na tela (ex.: "WASD")

        PlayerKeys(SDL_Scancode u, SDL_Scancode d, SDL_Scancode l, SDL_Scancode r, SDL_Scancode f, const char* n)
            : up(u), down(d), left(l), right(r), fire(f), name(n) {}
    };

    /**
     * Construtor padrão.
     * Cria o jogador na posição inicial definida em AppConfig.
     * O teclado e o controle do jogador são decididos pela classe Controllers.
     * @param idx - índice do jogador (0 = Jogador 1); define cor e posição
     */
    explicit Player(int idx);

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

    /**
     * Índice do jogador (0 = Jogador 1). Define teclas, controle e posição inicial.
     */
    int playerIndex() const;

    /**
     * Define a cor do tanque do jogador.
     * @param player_color - cor a ser aplicada ao tanque
     */
    void setPlayerColor(SDL_Color player_color);

    /**
     * Cores predefinidas para os jogadores.
     */
    static SDL_Color getPlayerColor(int player_index);

    /**
     * Seta uma flag de estado do tanque (sobrescreve Tank::setFlag).
     * Garante que o escudo tenha a mesma cor do jogador.
     * @param flag - flag a ser ativada
     */
    void setFlag(TankStateFlag flag);

private:
    /**
     * Índice do jogador (0 = Jogador 1). Guardado à parte porque no duelo
     * o sprite (type) indica a equipe, não o jogador.
     */
    int m_index;

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
