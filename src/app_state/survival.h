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
    void drawPause() override { drawPauseBox(); }

private:
    enum Phase
    {
        PHASE_WAVE_INTRO, ///< aviso "WAVE N": jogadores andam, inimigos ainda não surgem
        PHASE_PLAY,       ///< onda em andamento
        PHASE_BREAK,      ///< intervalo entre ondas: recompensas, loja aberta, contagem
        PHASE_RESULTS     ///< fim de jogo: onda alcançada e pontos de cada jogador
    };

    /** Começa a onda @a wave: aviso "WAVE N" e, depois dele, os inimigos. */
    void startWave(int wave);

    /**
     * Onda vencida: intervalo (AppConfig::survival_break_time) com as recompensas (mapa
     * regenerado, quem caiu volta, vida extra a cada N ondas) e a loja aberta ao lado da base.
     */
    void startBreak();

    /** Dificuldade da onda na escala das fases da campanha (1 a 35). */
    int difficulty() const;

    /** Refaz os tijolos em volta da águia (sem cobrir tanques, nem a pedra da pá). */
    void rebuildBaseWalls();

    /** Dá o poder ao jogador: guardável vai para o espaço de poder; os outros valem na hora. */
    void givePower(Player* player, SpriteType type);

    // ===== Loja (AppConfig::survival_shop): uma só, da equipe, aberta nos intervalos =====

    SDL_Point m_shop_pad = {-1, -1}; ///< canto do 2x2 da loja (pixels); x < 0: sem loja
    int m_shop_item = 0;             ///< item escolhido (índice em shopItems())
    int m_shop_user = -1;            ///< jogador em cima da loja (índice) ou -1
    int m_coins_spent = 0;

    /** Itens da loja (poder e preço), de AppConfig::survival_shop_items, do mais barato ao mais caro. */
    static std::vector<std::pair<SpriteType, int>> shopItems();

    /** Moedas da equipe: os pontos de todos os jogadores, em moedas, menos o que já foi gasto. */
    int coins() const;

    /** Duração da fase de intervalo (ms): o intervalo menos o aviso da próxima onda. */
    Uint32 breakTime() const;

    /** Loja aberta: no intervalo, antes do aviso da próxima onda, e com lugar no mapa. */
    bool shopOpen() const;

    /** Escolhe o lugar da loja: ao lado da base, de um lado sorteado (SurvivalLayout::shopSpots). */
    void placeShop();

    /** O tanque está em cima da loja (até 8 px fora do lugar). */
    bool onShop(const Player* player) const;

    /**
     * Quem está em cima da loja: LB e RB escolhem o item, tiro compra (sem disparar); os
     * outros usam o poder guardado com o botão de poder, como sempre.
     */
    void updateShop();

    /** Compra com as moedas da equipe. @return false sem moedas ou com o espaço ocupado */
    bool buy(Player* player, SpriteType type, int price);

    void drawFloor() override;
    void drawShop();
    static std::string shopName(SpriteType type);

    /** Tiles da muralha da águia ({coluna, linha}): laterais e frente. */
    std::vector<SDL_Point> baseWallTiles() const;

    /** O mapa volta ao do arquivo (o que foi destruído), e a muralha da águia é refeita. */
    void regenerateMap();

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
    std::vector<int> m_revived; ///< jogadores que voltaram nesta onda (índices), para o aviso
    Uint32 m_truce_time = 0;   ///< tempo restante da trégua (ms): sem inimigos novos
};

#endif // SURVIVAL_H
