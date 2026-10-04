#ifndef SURVIVAL_H
#define SURVIVAL_H

#include "game.h"

/**
 * @brief Modo sobrevivência: de 1 a 4 jogadores, juntos, contra ondas de inimigos sem fim.
 *
 * Reaproveita do Game a campanha inteira (mapa, colisões, inimigos, bônus, águia e o
 * "GAME OVER"); muda só o ritmo:
 * @li o mapa vem de survival_levels/ (escolhido no menu ou sorteado) e vai se desgastando
 *     de onda em onda;
 * @li cada onda tem mais inimigos, mais deles no mapa ao mesmo tempo e mais blindados
 *     (a dificuldade segue a escala das fases da campanha);
 * @li entre as ondas, a muralha da águia é refeita e há uma pausa com o aviso "WAVE N";
 * @li a cada AppConfig::survival_life_every_waves ondas, todos ganham uma vida e quem
 *     já tinha caído volta ao jogo;
 * @li cada jogador tem a sua cor (P1 amarelo, P2 verde, P3 azul, P4 vermelho);
 * @li acaba quando a águia cai ou todos os jogadores perdem as vidas: vale a onda alcançada.
 */
class Survival : public Game
{
public:
    /**
     * @param players - quantidade de jogadores (1 a 4)
     * @param map - mapa (índice em AppConfig::survival_maps), ou -1 para sortear
     */
    explicit Survival(int players, int map = -1);

    /** Libera também os jogadores que caíram (na campanha eles vão para a tela de pontos). */
    ~Survival();

    void update(Uint32 dt) override;

    /**
     * @li Enter / Start - pausa
     * @li Esc / Back - volta ao menu
     * @li no fim, tiro / Enter / A - volta ao menu
     */
    void eventProcess(SDL_Event* ev) override;

    /** Volta para a tela do modo sobrevivência no menu (para jogar de novo). */
    AppState* nextState() override;

    /** Onda atual (começa em 1). */
    int wave() const { return m_wave; }

    /** Inimigos da onda @a wave. */
    static int waveEnemies(int wave);

protected:
    void generateEnemy() override;
    SpriteType randomBonusType() override;
    void checkCollisionPlayerWithBonus(Player* player, Bonus* bonus) override;
    bool reservedTile(int row, int column) override;
    int enemyLimit() const override;
    Uint32 enemySpawnDelay() const override;
    void drawStatus() override;
    void drawOverlay() override;

private:
    enum Phase
    {
        PHASE_WAVE_INTRO, ///< aviso "WAVE N": jogadores andam, inimigos ainda não surgem
        PHASE_PLAY,       ///< onda em andamento
        PHASE_RESULTS     ///< fim de jogo: onda alcançada e pontos de cada jogador
    };

    /** Começa a onda @a wave: aviso, muralha refeita, vida extra a cada N ondas. */
    void startWave(int wave);

    /** Dificuldade da onda na escala das fases da campanha (1 a 35). */
    int difficulty() const;

    /** Refaz os tijolos em volta da águia (sem cobrir tanques, nem a pedra da pá). */
    void rebuildBaseWalls();

    /** Vida extra para todos; quem tinha caído volta ao mapa. */
    void rewardLives();

    /** Um jogador que caiu volta com uma vida. @return false se ninguém caiu */
    bool reviveOne();

    /** Usa o poder guardado do jogador. @return false se não deu (sem espaço, ponto ocupado) */
    bool usePower(Player* player);

    /**
     * A torreta acerta um inimigo atirando na direção @a d: inimigo alinhado, a até
     * AppConfig::power_turret_range tiles, sem pedra no caminho e sem a águia ou a muralha
     * dela no meio (o tiro destruiria a própria base).
     */
    bool turretShot(Turret* turret, Direction d);

    Phase m_phase;
    Uint32 m_phase_time;
    int m_wave;
    int m_map;                 ///< mapa em jogo (índice em AppConfig::survival_maps)
    int m_destroyed;           ///< inimigos destruídos na partida
    bool m_life_reward;        ///< o aviso desta onda inclui a vida extra
    Uint32 m_truce_time = 0;   ///< tempo restante da trégua (ms): sem inimigos novos
};

#endif // SURVIVAL_H
