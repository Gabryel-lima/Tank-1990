#ifndef DUEL_H
#define DUEL_H

#include "game.h"
#include "navgrid.h"
#include "../objects/bot.h"

#include <map>

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
     * Mapa escolhido (índice em AppConfig::duel_maps), ou -1 para um mapa aleatório a cada rodada.
     */
    int map = 0;

    /**
     * Jogadores controlados pelo computador (usado pela simulação, tools/duel_sim).
     * No jogo normal, todos são humanos.
     */
    bool cpu[4] = {false, false, false, false};

    /**
     * Quantos humanos estão na equipe.
     */
    int humansInTeam(int team) const;
};

/**
 * @brief Registro do que aconteceu na partida, para medir o equilíbrio (tools/duel_sim).
 */
struct DuelStats
{
    /** Um bônus coletado. */
    struct Pickup
    {
        int round;        ///< rodada (começa em 1)
        int team;         ///< equipe de quem coletou
        SpriteType type;  ///< tipo do bônus
        bool neutral;     ///< cinza (qualquer equipe pega) ou da cor da equipe
        int side;         ///< onde estava: -1 no meio, 0 na metade de quem pegou, 1 na do adversário
        double lead;      ///< vantagem em vidas de quem coletou, proporcional (-1 a 1)
    };
    std::vector<Pickup> pickups;

    std::vector<int> round_winner;       ///< equipe vencedora de cada rodada (-1 = empate)
    std::vector<bool> round_by_base;     ///< rodada decidida pela base (senão, por eliminação)
    std::vector<Uint32> round_time;      ///< duração de cada rodada (ms)
    std::vector<int> round_map;          ///< mapa de cada rodada

    // IA, separada em [0] bots de reforço e [1] jogadores do computador
    double ai_alive_time[2] = {0, 0}; ///< tempo somado de tanques da IA em campo (ms)
    double ai_stuck_time[2] = {0, 0}; ///< parte desse tempo em que a IA queria andar e não saiu do lugar
    int ai_unstick[2] = {0, 0};       ///< vezes em que a IA desistiu do caminho por estar presa
    std::vector<double> bot_heat;     ///< tempo dos bots em cada bloco do mapa (linha * colunas + coluna)
    int heat_columns = 0;
};

/**
 * @brief Modo duelo: duas equipes, cada uma defendendo a sua base e atacando a outra.
 *
 * Reaproveita do Game o mapa, as colisões e a assistência de curva; muda as regras:
 * @li a equipe A nasce embaixo e a B em cima, cada uma com a sua águia;
 * @li só jogadores humanos (1v1, 2v2 ou divisões como 2v1 e 3v1);
 * @li vence a rodada quem destruir a base inimiga ou eliminar todos os jogadores inimigos;
 * @li vence a partida quem ganhar AppConfig::duel_rounds_to_win rodadas;
 * @li projéteis não ferem aliados; o tiro de um jogador destrói a própria base e os
 *     tijolos em volta dela (como no original), o do bot de reforço não;
 * @li a frente de cada águia é de pedra e as laterais de tijolo: a base cai pelos flancos,
 *     e o pátio à frente dela deixa o defensor contornar a águia para o lado atacado;
 * @li bônus surgem em pontos simétricos no meio do mapa; a equipe em desvantagem
 *     passa a recebê-los do seu lado do campo;
 * @li a cor é da equipe (companheiros têm a mesma cor); cada bônus surge na cor de uma
 *     equipe (só jogadores dela coletam, mais na metade adversária) ou cinza, como no jogo
 *     original (qualquer jogador coleta, no meio do mapa); o bônus de tanque traz um bot
 *     aliado (reforço) da cor da equipe;
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

    /** O que aconteceu na partida até agora (usado pela simulação). */
    const DuelStats& stats() const { return m_stats; }

protected:
    std::vector<Eagle*> bases() override;
    void onBaseHit(Eagle* base, Bullet* bullet) override;
    bool bulletCanDamage(Bullet* bullet, int row, int column) override;
    bool powerAppliesAt(Bullet* bullet, int row, int column) override;

private:
    enum Phase
    {
        PHASE_INTRO,      ///< "ROUND N" sobre o mapa, tudo parado
        PHASE_PLAY,       ///< rodada em andamento
        PHASE_ROUND_END,  ///< resultado da rodada
        PHASE_MATCH_END   ///< resultado da partida
    };

    /** Pontos de nascimento da equipe, em ordem de preferência (lados alternados). */
    std::vector<SDL_Point> spawnOrder(int team) const;

    /**
     * Ponto de nascimento de cada jogador: cada um numa coluna só dele, alternando os
     * lados da base, para que ninguém nasça na linha de tiro de um adversário.
     */
    std::vector<SDL_Point> assignSpawns();

    /** Monta o mapa e os tanques de uma nova rodada. */
    void startRound();

    /** Cria o jogador @a index da partida (equipe, cor, ponto de nascimento) com @a lives vidas. */
    Player* createPlayer(int index, int lives);

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

    // ======================== IA (duel_ai.cpp) ========================

    /** Memória da IA de um tanque entre um quadro e outro. */
    struct AIState
    {
        std::vector<int> field;     ///< distância de cada célula até o destino (NavGrid)
        SDL_Rect face = {0, 0, 0, 0}; ///< o que encarar ao chegar (tanque ou base), ou w = 0
        Uint32 replan = 0;          ///< tempo até recalcular o destino (ms)
        double last_x = -1, last_y = -1;
        bool wanted_move = false;   ///< no quadro anterior, a IA mandou andar
        Uint32 still_time = 0;      ///< tempo querendo andar sem sair do lugar (ms)
        Uint32 unstick_time = 0;    ///< tempo restante andando numa direção qualquer para destravar
        Direction unstick_dir = D_UP;
        Uint32 aim_time = 0;        ///< há quanto tempo segura a mira num alvo (ms)
        Uint32 aim_cooldown = 0;    ///< tempo até poder virar de novo para atirar de lado (ms)
        Tank* chasing = nullptr;    ///< inimigo que o atacante está caçando (histerese da decisão)
        Uint32 align_pause = 0;     ///< alinhamento ao chegar bloqueado: espera antes de tentar de novo (ms)
        Direction move_dir = D_UP;  ///< direção em que andou por último
        Uint32 move_time = 0;       ///< há quanto tempo anda nessa direção (ms)
    };

    /** Tanque controlado pela IA: bot de reforço ou jogador do computador. */
    bool isAI(Tank* tank) const;

    /** Papel fixo do tanque da IA (o jogador do computador ataca; o segundo da equipe defende). */
    Bot::Role baseRole(Tank* tank) const;

    /**
     * Papel do tanque da IA agora: o fixo, menos quando ninguém em campo ataca; aí o jogador
     * do computador que defendia sai para atacar (evita a rodada sem fim).
     */
    Bot::Role roleOf(Tank* tank) const;

    /** O que o tanque consegue atravessar (barco, tiro forte). */
    NavGrid::Abilities abilitiesOf(Tank* tank) const;

    /** Monta as grades de navegação das duas equipes a partir do mapa atual. */
    void buildNavGrids();

    /** Decide o comando de cada tanque da IA neste quadro. */
    void updateAI(Uint32 dt);

    /** Escolhe o destino do tanque e calcula o campo de distâncias até ele. */
    void planAI(Tank* tank, AIState& state);

    /** Comando do quadro: segue o caminho, destrava, vira para atirar. */
    TankCommand steerAI(Tank* tank, AIState& state, Uint32 dt);

    /**
     * Células de onde se acerta o alvo atirando em linha reta (na mesma linha ou coluna,
     * sem pedra no meio e com no máximo dois tijolos), até @a range blocos de distância.
     */
    std::vector<int> fireGoals(const SDL_Rect& target, int team, const NavGrid::Abilities& abilities, int range) const;

    /**
     * O tiro do tanque na direção @a d acerta o alvo: alinhado, à frente, a até @a range
     * blocos e sem pedra (nem muralha protegida) no caminho; tijolos, no máximo dois.
     */
    bool clearShot(Tank* shooter, Direction d, const SDL_Rect& target, int range) const;

    /** Atirar na direção @a d acerta algum inimigo, a base inimiga ou um projétil que vem vindo. */
    bool worthFiring(Tank* shooter, Direction d, int range);

    /**
     * O tiro na direção @a d passaria pela muralha ou pela águia da própria equipe
     * (a IA não atira assim: o tiro de jogador destrói a própria base).
     */
    bool firesAtOwnBase(Tank* tank, Direction d) const;

    /** Há tijolo (ou pedra, com tiro forte) colado na frente do tanque. */
    bool breakableAhead(Tank* tank, Direction d) const;

    /**
     * Projéteis do atirador contra um tanque de outra equipe.
     * Conta a eliminação para o jogador humano que atirou.
     */
    void checkBulletsAgainstTank(Tank* shooter, Tank* target);

    /** Destrói o tanque e conta a eliminação para o jogador, se de fato morreu. */
    void hitTank(Tank* target, Tank* shooter);

    /** Equipe bem atrás em vidas (proporcionalmente), ou -1 se estão parelhas. */
    int trailingTeam();

    /** Pontos simétricos de surgimento de bônus dentro da metade do mapa da equipe. */
    std::vector<SDL_Point> halfSpots(int team) const;

    /** Vantagem em vidas da equipe, proporcional ao total de cada uma (-1 a 1). */
    double teamLead(int team);

    /** Sorteia a equipe dona de um bônus colorido: 50/50, ou a que está atrás em vidas. */
    int chooseBonusTeam();

    /** Sorteia um bônus em um dos pontos simétricos do mapa. */
    void spawnBonus();

    /** Aplica o efeito do bônus para o jogador e a sua equipe (ou guarda o poder). */
    void applyBonus(Player* player, Bonus* bonus);

    /** Reviver: um companheiro que caiu volta com uma vida; sem ninguém caído, vida extra. */
    void revive(Player* player);

    /** Usa o poder guardado do jogador. @return false se não deu (sem espaço, ponto ocupado) */
    bool usePower(Player* player);

    /** Botão de poder, torretas e minas (a cada quadro). */
    void updatePowers(Uint32 dt);

    bool reservedTile(int row, int column) override;

    /** Monta a base da equipe: frente de pedra, laterais e cantos do material @a wall. */
    void setBaseWalls(int team, SpriteType wall);

    /** Controla a duração da pá (base reforçada com pedra) de cada equipe. */
    void updateFortify(Uint32 dt);

    /** O bloco (linha, coluna) fica na zona de uma das bases, onde o canhão não vale. */
    bool isInBaseZone(int row, int column) const;

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
    int m_map;                 ///< mapa da rodada atual (índice em AppConfig::duel_maps)
    std::vector<int> m_player_columns; ///< colunas (x) de nascimento usadas pelos jogadores
    std::vector<SDL_Point> m_spawns;   ///< ponto de nascimento de cada jogador na rodada

    NavGrid m_nav[2];                  ///< grade de navegação de cada equipe (muralha própria bloqueia)
    std::map<Tank*, AIState> m_ai;     ///< memória da IA de cada tanque
    DuelStats m_stats;
};

#endif // DUEL_H
