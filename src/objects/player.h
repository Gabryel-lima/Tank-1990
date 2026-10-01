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
     * @brief Teclas do teclado que controlam o tanque do jogador.
     * Jogadores sem teclado (3 e 4) usam o construtor padrão, com todas as
     * teclas em SDL_SCANCODE_UNKNOWN. O controle (gamepad) de cada jogador é
     * definido pela classe Controllers, não aqui.
     */
    struct PlayerKeys
    {
        SDL_Scancode up;
        SDL_Scancode down;
        SDL_Scancode left;
        SDL_Scancode right;
        SDL_Scancode fire;

        PlayerKeys(SDL_Scancode u, SDL_Scancode d, SDL_Scancode l, SDL_Scancode r, SDL_Scancode f)
            : up(u), down(d), left(l), right(r), fire(f) {}

        // Sem teclado: o jogador só joga com controle
        PlayerKeys()
            : up(SDL_SCANCODE_UNKNOWN), down(SDL_SCANCODE_UNKNOWN), left(SDL_SCANCODE_UNKNOWN),
              right(SDL_SCANCODE_UNKNOWN), fire(SDL_SCANCODE_UNKNOWN) {}

        bool hasKeyboard() const { return fire != SDL_SCANCODE_UNKNOWN; }
    };

    /**
     * Construtor padrão.
     * Cria o jogador na posição inicial definida em AppConfig.
     * @param keys - teclas do teclado do jogador
     * @param idx - índice do jogador (0 = Jogador 1); define cor, posição e controle
     */
    Player(const PlayerKeys& keys, int idx);

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
