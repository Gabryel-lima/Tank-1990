#ifndef PLAYER_H
#define PLAYER_H

#include "tank.h"

#include <vector>

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
        SDL_Scancode power; ///< usa o poder guardado (modos extras)
        const char* name; ///< nome mostrado na tela (ex.: "WASD")

        PlayerKeys(SDL_Scancode u, SDL_Scancode d, SDL_Scancode l, SDL_Scancode r, SDL_Scancode f, SDL_Scancode p, const char* n)
            : up(u), down(d), left(l), right(r), fire(f), power(p), name(n) {}
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
     * Só as balas que ainda voam: a que bateu libera o tiro na hora, sem esperar os 240 ms
     * da animação da explosão (que segurava o tiro seguinte, mais ainda atirando de perto).
     */
    unsigned bulletsInUse() const override;

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

    /**
     * Quantidade atual de estrelas (0 a 3). Com 3, o tiro quebra pedra.
     */
    int stars() const { return star_count; }

    /**
     * Tiro demolidor (duelo): o segundo estágio do tanque. Vem do canhão pego por quem já
     * tinha estrela (AppConfig::duel_demolisher_stars). O tiro derruba a pedra da base
     * inimiga, se disparado de dentro da zona dela. Perde-se ao morrer.
     */
    bool demolisher() const { return m_demolisher; }
    void setDemolisher(bool on) { m_demolisher = on && star_count >= 3; }

    /** Brilho do tiro demolidor: aceso nos primeiros 120 ms de cada segundo do relógio. */
    static bool demolisherGlint(Uint32 effect_time) { return effect_time % 1000 < 120; }
    static const Uint8 DEMOLISHER_GLINT_ALPHA = 170;

protected:
    /** Tiro demolidor: um brilho branco curto a cada segundo, à vista de todos. */
    void drawEffects() override;

public:

    /**
     * Com estrela, um tiro só tira uma estrela (o tanque volta um estágio) em vez de
     * destruí-lo; sem estrela, morre. No Battle City original qualquer tiro mata: esta é
     * uma regra do jogo. O duelo pode desligar (ver AppConfig::duel_star_armor).
     */
    bool star_armor = true;

    /**
     * Jogador controlado pelo computador (usado pela simulação do duelo, tools/duel_sim):
     * ignora teclado e controle e segue @a cpu_command.
     */
    bool cpu = false;

    /**
     * Comando da IA aplicado no próximo update, se @a cpu for verdadeiro.
     */
    TankCommand cpu_command;

    /**
     * Define o intervalo mínimo entre tiros (ms). O padrão é AppConfig::player_reload_time;
     * o modo duelo usa um intervalo maior para limitar a cadência.
     */
    void setReloadTime(Uint32 ms);

    // ======================== Poderes dos modos extras ========================

    /**
     * Poder guardado (mina, barricada, torreta, retorno ou turbo), ou ST_NONE. Enquanto
     * guarda um poder, o jogador não pega outros bônus (ver Powers). Perde-o ao morrer.
     */
    SpriteType held_power = ST_NONE;

    /**
     * Sobrevivência: os poderes guardados depois do held_power, na ordem em que serão usados
     * (cada um ocupa um espaço). O duelo não usa: lá o jogador guarda um poder só.
     */
    std::vector<SpriteType> power_stock;

    /** Espaços de poder: quantos poderes o jogador guarda ao mesmo tempo (sobrevivência). */
    int power_slots = 1;

    /** Poderes guardados: o held_power e o estoque. */
    int storedPowers() const;

    /** Guarda um poder: no held_power, se vazio; senão, no fim do estoque. */
    void storePower(SpriteType type);

    /** O held_power foi usado: o próximo do estoque toma o lugar dele. */
    void consumeHeldPower();

    /**
     * O botão de poder foi apertado desde a última chamada (Shift do layout de teclado, LB
     * do controle ou o comando da IA). Conta uma vez por aperto.
     */
    bool takePowerPress();

    /**
     * O botão de tiro foi apertado desde a última chamada. Conta uma vez por aperto; serve à
     * loja da sobrevivência, onde o tiro compra (ver shop_mode).
     */
    bool takeFirePress();

    /**
     * Passos na loja desde a última chamada: -1 por aperto do LB, +1 por aperto do RB ou do
     * botão de poder do teclado (que só avança).
     */
    int takeShopStep();

    /** Na loja (sobrevivência): o tiro não dispara, só conta o aperto (takeFirePress). */
    bool shop_mode = false;

    /** Turbo: velocidade multiplicada por AppConfig::power_turbo_factor por @a ms. */
    void boost(Uint32 ms);

    /** Turbo ativo. */
    bool boosted() const { return m_turbo_time > 0; }

    /**
     * Quanto falta para o turbo acabar, para a névoa de "acabando" (V1): 0 longe do fim (ou
     * sem turbo), subindo até 1 no último quarto do tempo.
     */
    double turboEnding() const;

    /**
     * Poder com duração em uso pelo jogador (hoje, o turbo): o painel mostra o ícone dele no
     * espaço de poder enquanto dura, com a névoa de "acabando" (W8, V1). Poder novo com
     * duração que fica no jogador responde aqui, e o painel funciona sem código novo.
     * @param ending - recebe de 0 (longe do fim) a 1 (no fim), como turboEnding
     * @return o tipo do poder, ou ST_NONE se nenhum está valendo
     */
    SpriteType activePower(double* ending) const;

    /**
     * Retorno: reaparece em (x, y) com a animação de nascimento (sem gastar vida, mantendo
     * estrelas e barco; durante a animação o tanque não leva tiro).
     */
    void teleport(double x, double y);

private:
    bool m_power_down = false;     ///< botão de poder segurado no quadro anterior
    bool m_power_pressed = false;  ///< aperto ainda não consumido por takePowerPress
    bool m_fire_down = false;      ///< botão de tiro segurado no quadro anterior
    bool m_fire_pressed = false;   ///< aperto ainda não consumido por takeFirePress
    bool m_shop_left_down = false;  ///< LB segurado no quadro anterior
    bool m_shop_right_down = false; ///< RB (ou poder do teclado) segurado no quadro anterior
    int m_shop_step = 0;            ///< passos na loja ainda não consumidos por takeShopStep
    Uint32 m_turbo_time = 0;       ///< tempo restante de turbo (ms)
    bool m_demolisher = false;     ///< tiro demolidor (ver demolisher())

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

    /**
     * Intervalo mínimo entre tiros (ms).
     */
    Uint32 m_reload_time;
};

#endif // PLAYER_H
