#ifndef TANK_H
#define TANK_H

#include "object.h"
#include "bullet.h"
#include "../type.h"

#include <vector>

typedef unsigned TankStateFlags;

/**
 * @brief
 * Classe responsável pela mecânica básica dos tanques: movimentação e disparo.
 */
class Tank : public Object
{
public:
    /**
     * Construtor padrão.
     * Cria o tanque na primeira posição inicial dos inimigos.
     * @see AppConfig::enemy_starting_point
     */
    Tank();

    /**
     * Construtor parametrizado.
     * Cria o tanque em uma posição e tipo específicos.
     * @param x - posição horizontal inicial
     * @param y - posição vertical inicial
     * @param type - tipo do tanque
     */
    Tank(double x, double y, SpriteType type);

    /**
     * Destrutor virtual.
     * Garante destruição correta dos recursos.
     */
    virtual ~Tank();

    /**
     * Desenha o tanque na tela.
     * Se necessário, desenha também o escudo e o barco.
     * Chama o desenho dos projéteis disparados.
     */
    void draw();

    /**
     * Atualiza o estado do tanque.
     * Atualiza posição, retângulos de destino e colisão, posição do escudo/barco, animação,
     * atualiza os projéteis e remove projéteis destruídos.
     * Controla o tempo de escudo e congelamento, desativando as flags quando necessário.
     * @param dt - tempo desde a última chamada (em ms), usado para animação
     */
    void update(Uint32 dt);

    /**
     * Cria um novo projétil se ainda não atingiu o limite máximo.
     * @return ponteiro para o projétil criado, ou nullptr se não for possível criar
     */
    virtual Bullet* fire();

    /**
     * Retorna o retângulo de colisão previsto para o próximo frame,
     * considerando a velocidade e direção atuais.
     * @param dt - tempo previsto para o próximo frame
     * @return próximo retângulo de colisão
     */
    SDL_Rect nextCollisionRect(Uint32 dt);

    /**
     * Define a próxima direção de movimento, considerando o efeito de deslize no gelo.
     * Ao mudar de direção, ajusta o tanque para múltiplos do tamanho do tile (AppConfig::tile_rect).
     * @param d - nova direção
     */
    void setDirection(Direction d);

    /**
     * Para o tanque devido a uma colisão.
     * @param intersect_rect - área de colisão
     */
    void collide(SDL_Rect &intersect_rect);

    /**
     * Limpa todas as flags e inicia a animação de respawn do tanque.
     */
    virtual void respawn();

    /**
     * Inicia a animação de destruição/explosão do tanque.
     */
    virtual void destroy();

    /**
     * Seta uma flag de estado do tanque.
     * @param flag - flag a ser ativada
     */
    void setFlag(TankStateFlag flag);

    /**
     * Limpa uma flag de estado do tanque.
     * @param flag - flag a ser desativada
     */
    void clearFlag(TankStateFlag flag);

    /**
     * Verifica se uma flag de estado está ativa.
     * @param flag - flag a ser verificada
     * @return true se a flag está ativa, false caso contrário
     */
    bool testFlag(TankStateFlag flag);

    /**
     * Velocidade padrão do tanque.
     * Pode variar conforme o tipo do tanque ou bônus coletados.
     */
    double default_speed;

    /**
     * Velocidade atual do tanque.
     */
    double speed;

    /**
     * Indica se o tanque está parado.
     */
    bool stop;

    /**
     * Direção atual do tanque.
     */
    Direction direction;

    /**
     * Vetor com os projéteis disparados pelo tanque.
     */
    std::vector<Bullet*> bullets;

    /**
     * Número de vidas do jogador ou nível de armadura do tanque inimigo.
     */
    int lives_count;

protected:
    /**
     * Flags de estado atuais do tanque (invulnerável, congelado, etc).
     */
    TankStateFlags m_flags;

    /**
     * Tempo desde o início do deslize (gelo).
     */
    Sint32 m_slip_time;

    /**
     * Direção visual do tanque durante o deslize, pode ser diferente da direção real de movimento.
     */
    Direction new_direction;

    /**
     * Quantidade máxima de projéteis que o tanque pode disparar simultaneamente.
     */
    unsigned m_bullet_max_size;

    /**
     * Ponteiro para o escudo do tanque. nullptr se não houver escudo.
     */
    Object* m_shield;

    /**
     * Ponteiro para o barco do tanque. nullptr se não houver barco.
     */
    Object* m_boat;

    /**
     * Tempo desde que o escudo foi adquirido.
     */
    Uint32 m_shield_time;

    /**
     * Tempo desde que o tanque foi congelado.
     */
    Uint32 m_frozen_time;
};

#endif // TANK_H
