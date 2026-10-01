#ifndef DUEL_H
#define DUEL_H

#include "game.h"
#include "../objects/bot.h"

/**
 * @brief Configuração de uma partida do modo duelo, montada no menu "Extra Modes".
 */
struct DuelConfig
{
    /**
     * Quantidade de jogadores de cada equipe. Índice 0 = equipe A, 1 = equipe B.
     * O Duel recalcula a partir de @a human_team.
     */
    int team_size[2] = {1, 1};

    /**
     * Quantidade de jogadores (2 a 4). Não há bots ocupando vagas: bots só entram
     * pelo bônus de reforço.
     */
    int humans = 2;

    /**
     * Equipe de cada jogador humano (0 = A, 1 = B). Só os primeiros @a humans valem.
     */
    int human_team[4] = {0, 1, 0, 1};

    /**
     * Quantos humanos estão na equipe.
     */
    int humansInTeam(int team) const;
};

/**
 * @brief Modo duelo: duas equipes, cada uma defendendo a sua base e atacando a outra.
 *
 * Reaproveita do Game o mapa, as colisões e a assistência de curva; muda as regras:
 * @li a equipe A nasce embaixo e a B em cima, cada uma com a sua águia;
 * @li só jogadores humanos (1v1, 2v2 ou divisões como 2v1 e 3v1);
 * @li vence a rodada quem destruir a base inimiga ou eliminar todos os jogadores inimigos;
 * @li vence a partida quem ganhar AppConfig::duel_rounds_to_win rodadas;
 * @li projéteis não ferem aliados nem a própria base (nem os tijolos em volta dela);
 * @li bônus surgem em pontos simétricos no meio do mapa; a equipe em desvantagem
 *     passa a recebê-los do seu lado do campo;
 * @li a cor é da equipe (companheiros têm a mesma cor); o bônus de tanque é da equipe:
 *     surge na cor de uma equipe, só jogadores dela coletam, mais na metade adversária,
 *     e traz um bot aliado (reforço) da mesma cor;
 * @li com equipes de tamanhos diferentes, a menor tem mais vidas (e base de pedra, se a outra tiver o dobro).
 */
class Duel : public Game
{
public:
    explicit Duel(const DuelConfig& config);
    ~Duel();

    void draw() override;
    void update(Uint32 dt) override;

    /**
     * @li Enter / Start - pausa
     * @li Esc / Back - volta ao menu
     * @li no fim da partida, tiro / Enter / A - volta ao menu
     */
    void eventProcess(SDL_Event* ev) override;

    /**
     * Volta para a tela de configuração do duelo, para facilitar a revanche.
     */
    AppState* nextState() override;

    /**
     * Cor da equipe: paleta de 4 cores (amarelo, verde, azul, vermelho), uma por equipe.
     * Companheiros de equipe têm a mesma cor; cores diferentes só entre equipes diferentes.
     */
    static SDL_Color teamColor(int team);

protected:
    std::vector<Eagle*> bases() override;
    void onBaseHit(Eagle* base, Bullet* bullet) override;
    bool bulletCanDamage(Bullet* bullet, int row, int column) override;

private:
    enum Phase
    {
        PHASE_INTRO,      ///< "ROUND N" sobre o mapa, tudo parado
        PHASE_PLAY,       ///< rodada em andamento
        PHASE_ROUND_END,  ///< resultado da rodada
        PHASE_MATCH_END   ///< resultado da partida
    };

    /** Monta o mapa e os tanques de uma nova rodada. */
    void startRound();

    /** Remove tudo da rodada atual (mapa, tanques, bônus e as duas bases). */
    void clearRound();

    /**
     * Encerra a rodada.
     * @param winner - equipe vencedora, ou -1 para empate (rodada repetida)
     */
    void endRound(int winner);

    /** Todos os tanques da partida (jogadores e bots). */
    std::vector<Tank*> allTanks();

    /** Vidas restantes dos jogadores da equipe, contando o tanque em campo. */
    int teamLives(int team);

    /**
     * Bônus de reforço: cria um bot aliado na equipe do jogador, com uma vida.
     * @return false se a equipe já tem AppConfig::duel_max_allies reforços ou não há ponto livre
     */
    bool spawnAlly(Player* player);

    /** Equipe ainda tem algum jogador que não foi removido. */
    bool teamAlive(int team);

    /** Vidas de cada tanque da equipe: equipes menores recebem mais vidas. */
    int livesPerTank(int team) const;

    /** Material padrão da muralha da base (pedra para a equipe menor). */
    SpriteType defaultWall(int team) const;

    /** Base da equipe. */
    Eagle* baseOf(int team);

    /** Define os alvos dos bots de acordo com o papel de cada um. */
    void updateBotTargets();

    /**
     * Projéteis do atirador contra um tanque de outra equipe.
     * Conta a eliminação para o jogador humano que atirou.
     */
    void checkBulletsAgainstTank(Tank* shooter, Tank* target);

    /** Destrói o tanque e conta a eliminação para o jogador, se de fato morreu. */
    void hitTank(Tank* target, Tank* shooter);

    /** Bônus que surge para uma equipe específica (só jogadores daquela cor coletam). */
    static bool isTeamBonus(SpriteType type);

    /** Equipe bem atrás em vidas (proporcionalmente), ou -1 se estão parelhas. */
    int trailingTeam();

    /** Pontos simétricos de surgimento de bônus dentro da metade do mapa da equipe. */
    std::vector<SDL_Point> halfSpots(int team) const;

    /** Sorteia a equipe dona de um bônus exclusivo: 50/50, ou a que está atrás em vidas. */
    int chooseBonusTeam();

    /** Sorteia um bônus em um dos pontos simétricos do mapa. */
    void spawnBonus();

    /** Aplica o efeito do bônus para o jogador e a sua equipe. */
    void applyBonus(Player* player, Bonus* bonus);

    /** Coloca tijolos ou pedras em volta da base da equipe. */
    void setBaseWalls(int team, SpriteType wall);

    /** Controla a duração da pá (base reforçada com pedra) de cada equipe. */
    void updateFortify(Uint32 dt);

    /** O bloco (linha, coluna) faz parte da muralha da base da equipe. */
    bool isBaseWall(int team, int row, int column) const;

    DuelConfig m_config;
    Eagle* m_base_b;          ///< base da equipe B (a da equipe A é m_eagle)
    Phase m_phase;
    Uint32 m_phase_time;
    int m_round;
    int m_wins[2];
    int m_round_winner;
    Uint32 m_bonus_time;
    bool m_fortified[2];
    Uint32 m_fortify_time[2];
    SpriteType m_base_wall[2]; ///< material atual da muralha de cada base
    int m_kills[4];            ///< eliminações de cada jogador humano na partida
};

#endif // DUEL_H
