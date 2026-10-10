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
 * @li nos intervalos, cada jogador tem a sua loja ao lado da base, na cor dele, que vende
 *     poderes, estrelas, espaços de poder e a tropa de reforço, com as moedas da equipe ou
 *     de cada um (escolha no menu);
 * @li raramente, um inimigo rompe a parede de pedra em que bateu e passa
 *     (AppConfig::survival_wall_breach_chance);
 * @li acaba quando a águia cai ou todos os jogadores perdem as vidas: vale a onda alcançada.
 */
class Survival : public Game
{
    friend struct SurvivalTest; // tools/survival_test.cpp: monta cenários e confere o estado
public:
    /**
     * @param players - quantidade de jogadores (1 a 4)
     * @param map - mapa (índice em AppConfig::survival_maps), ou -1 para sortear
     * @param shared_coins - moedas da equipe (true) ou de cada jogador (false)
     */
    explicit Survival(int players, int map = -1, bool shared_coins = true);

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
    bool bonusSpot(SDL_Point* spot) override;

public:
    /**
     * Onde um bônus pode surgir: as posições de tanque (2x2 tiles) sem bloco nenhum (só chão,
     * gelo ou arbusto), fora da águia, das torretas e da loja, e aonde os tanques chegam
     * andando a partir do nascimento dos jogadores. Pública para o teste (tools/survival_test).
     */
    std::vector<SDL_Point> bonusSpots() const;

protected:
    void checkCollisionPlayerWithBonus(Player* player, Bonus* bonus) override;
    bool reservedTile(int row, int column) override;
    int enemyLimit() const override;
    Uint32 enemySpawnDelay() const override;
    void drawStatus() override;
    void drawOverlay() override;
    void drawPause() override { drawPauseBox(); }
    /** Tiro de torreta ou de aliado na águia (passou do alvo): some, sem ferir (G1). */
    void onBaseHit(Eagle* base, Bullet* bullet) override;

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

    /**
     * Poder novo que fica guardado até o botão: os que dependem de onde e quando são usados
     * (Powers::storable: mina, barricada, torreta, retorno, turbo). Os outros (reviver,
     * reparo, trégua, escudo de equipe) valem na hora, também na compra.
     */
    static bool storesPower(SpriteType type);

    /** A muralha da águia está inteira (sem bloco faltando nem tijolo rachado). */
    bool baseWallIntact() const;

    /**
     * Dá o poder novo ao jogador: o guardável vai para o espaço de poder; o outro vale na
     * hora (applyNow).
     * @return false se o que vale na hora não teve efeito (ver applyNow)
     */
    bool givePower(Player* player, SpriteType type);

    /**
     * Poder que vale na hora (o que não é storesPower). A trégua e o escudo de equipe pegos
     * ou comprados fora da onda ficam armados para o começo da próxima (no intervalo eles
     * correriam sem inimigos).
     * @return false se não há o que fazer agora (reparo com a muralha inteira, já armado)
     */
    bool applyNow(Player* player, SpriteType type);

    /** Por que o poder que vale na hora não faria nada agora, ou "" se faz. */
    std::string noEffect(SpriteType type) const;

    /** Escudo em todos os jogadores vivos, como o capacete da campanha. */
    void shieldTeam();

    // ===== Lojas (AppConfig::survival_shop): uma por jogador, abertas nos intervalos =====

    SDL_Point m_shop_pads[4] = {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}}; ///< canto do 2x2 da loja de cada jogador (pixels); x < 0: sem loja
    int m_shop_item[4] = {0, 0, 0, 0}; ///< item escolhido por cada jogador (índice em shopItems())
    bool m_shared_coins = true;      ///< moedas da equipe (true) ou de cada jogador
    bool m_truce_armed = false;      ///< trégua comprada no intervalo: começa com a próxima onda
    bool m_shield_armed = false;     ///< escudo de equipe comprado no intervalo: liga com a próxima onda
    int m_coins_spent[4] = {0, 0, 0, 0}; ///< moedas gastas por cada jogador (índice)

    /**
     * Um item da loja. type: o poder (Powers), ST_BONUS_STAR (um nível de tiro),
     * ST_BONUS_TANK (tropa de reforço) ou ST_NONE (mais um espaço de poder).
     */
    struct ShopItem
    {
        SpriteType type;
        int price; ///< preço da lista (a estrela e o espaço sobem a partir dele, ver price())
    };

    /** Itens da loja, de AppConfig::survival_shop_items, do mais barato ao mais caro. */
    static std::vector<ShopItem> shopItems();

    /** Preço do item para o jogador agora (a estrela e o espaço sobem a cada compra). */
    int price(const ShopItem& item, const Player* player) const;

    /** Por que o jogador não pode comprar o item agora (o texto da loja), ou "" se pode. */
    std::string cannotBuy(const ShopItem& item, const Player* player) const;

    /**
     * Moedas que o jogador pode gastar: compartilhadas, as da equipe (os pontos de todos,
     * em moedas, mais as iniciais, menos o que todos gastaram); individuais, as dele.
     */
    int coins(const Player* player) const;

    /** Moedas da equipe toda (a soma, nas individuais). */
    int teamCoins() const;

    /**
     * Quanto falta (ms) para os inimigos da próxima onda começarem a surgir: no intervalo e no
     * aviso "WAVE N" (0 durante a onda). A contagem na tela sai daqui, então ela termina
     * exatamente quando a onda começa.
     */
    Uint32 timeToWave() const;

    /** timeToWave em segundos inteiros, arredondado para cima (o número mostrado). */
    int countdown() const;

    /** Duração da fase de intervalo (ms): o intervalo menos o aviso da próxima onda. */
    Uint32 breakTime() const;

    /** Alguma loja aberta: no intervalo, antes do aviso da próxima onda, e com lugar no mapa. */
    bool shopOpen() const;

    /** Há loja (no mapa) para o jogador @a index. */
    bool hasShop(int index) const;

    /**
     * Escolhe o lugar da loja de cada jogador, ao lado da base (SurvivalLayout::shopSpots), sem
     * uma cobrir a outra: P1 e P3 do lado esquerdo, P2 e P4 do direito (o lado de onde nascem);
     * sem lugar ali, do outro lado.
     */
    void placeShop();

    /** O tanque está em cima da loja dele (até 8 px fora do lugar). */
    bool onShop(const Player* player) const;

    /**
     * Quem está em cima da própria loja: LB e RB escolhem o item, tiro compra (sem disparar);
     * fora dela, o botão de poder usa o poder guardado, como sempre.
     */
    void updateShop();

    /** Compra o item. @return false se não deu (cannotBuy) */
    bool buy(Player* player, const ShopItem& item);

    // ===== Tropa de reforço: aliados do computador (Game::m_allies) =====

    /** Ponto de nascimento livre para um aliado (os dos jogadores, ao lado da base), ou x < 0. */
    SDL_Point allySpawn() const;

    /** Chama um aliado na cor do jogador. @return false sem lugar ou no limite */
    bool callReinforcement(Player* player);

    /** Decide o comando de cada aliado (Game::hunt). */
    void steerAllies(Uint32 dt);

    // ===== Rompimento da pedra (AppConfig::survival_wall_breach_chance) =====

    std::vector<Enemy*> m_pushing_stone; ///< inimigos que empurravam pedra no quadro anterior
    /** Explosões dos blocos rompidos e o tempo de cada uma (ms). */
    std::vector<std::pair<Object*, Uint32>> m_breach_effects;

    /**
     * Blocos de pedra que o tanque empurra de frente agora (a fileira logo à frente, na
     * largura dele). Vazio se não empurra pedra ou se a fileira toca a muralha da águia.
     * @param r - retângulo de colisão do tanque
     * @param line - recebe a fileira (linha ou coluna, conforme a direção)
     */
    std::vector<SDL_Point> stoneAhead(SDL_Rect r, Direction direction, int* line) const;

    /** A cada batida nova de um inimigo na pedra, sorteia o rompimento. */
    void breachWalls(Uint32 dt);

    /** Anima e tira as explosões dos blocos rompidos. */
    void updateBreachEffects(Uint32 dt);

    /** Rompe a parede de pedra na frente do inimigo (até 4 fileiras), com a explosão em cada bloco. */
    void breach(Enemy* enemy);

    void drawFloor() override;
    /** Caixa de texto da loja do jogador @a index (o item escolhido, se ele está em cima). */
    void drawShop(int index);
    /**
     * Caixa de texto da loja em @a pad (sem a borda de 1 px), de @a width x @a height: ao lado
     * da loja, do lado de fora, e com a borda inteira dentro do mapa.
     */
    SDL_Rect shopBoxRect(SDL_Point pad, int width, int height) const;
    static std::string shopName(SpriteType type);

    /** Tiles da muralha da águia ({coluna, linha}): laterais e frente. */
    std::vector<SDL_Point> baseWallTiles() const;

    /** O mapa volta ao do arquivo (o que foi destruído), e a muralha da águia é refeita. */
    void regenerateMap();

    /** Um jogador que caiu volta com uma vida. @return false se ninguém caiu */
    bool reviveOne();

    /** Usa o poder guardado do jogador. @return false se não deu (sem espaço, ponto ocupado) */
    bool usePower(Player* player);


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
