#include "demo.h"
#include "game.h"
#include "duel.h"
#include "survival.h"
#include "../appconfig.h"

#include <cstdlib>

namespace
{
    const int CAMPAIGN_STAGES = 35;
    const Uint32 INTRO_SKIP = 3200; ///< ms: mais que a abertura de qualquer modo (a maior, 3 s)

    void cpuPlayers(const std::vector<Player*>& players)
    {
        for(Player* player : players) player->cpu = true;
    }

    /** Campanha: uma fase sorteada, de 1 a 4 jogadores (começa no "STAGE N" cinza, como o jogo). */
    class CampaignDemo : public Game
    {
    public:
        explicit CampaignDemo(std::vector<Player*> players)
            : Game(players, rand() % CAMPAIGN_STAGES) { cpuPlayers(m_players); m_demo = true; }
    };

    /** Duelo: 1 contra 1 ou 2 contra 2, num mapa sorteado; a IA do duelo joga pelos dois lados. */
    class DuelDemo : public Duel
    {
    public:
        explicit DuelDemo(const DuelConfig& config) : Duel(config) { m_demo = true; }
    };

    /** Sobrevivência: um mapa sorteado, de 1 a 4 jogadores. */
    class SurvivalDemo : public Survival
    {
    public:
        SurvivalDemo() : Survival(1 + rand() % 4, -1) { cpuPlayers(m_players); m_demo = true; }
    };

    std::vector<Player*> newPlayers(int count)
    {
        std::vector<Player*> players;
        for(int i = 0; i < count; i++)
        {
            Player* player = new Player(i);
            player->cpu = true;
            players.push_back(player);
        }
        return players;
    }
}

namespace Demo
{

const std::vector<Mode>& modes()
{
    static const std::vector<Mode> list = {
        {"campaign", [] { return static_cast<AppState*>(new CampaignDemo(newPlayers(1 + rand() % 2))); }},
        {"duel", [] {
            DuelConfig config;
            config.humans = (rand() % 2 == 0) ? 2 : 4;
            config.map = -1;
            for(int i = 0; i < 4; i++) { config.human_team[i] = i % 2; config.cpu[i] = true; }
            return static_cast<AppState*>(new DuelDemo(config));
        }},
        {"survival", [] { return static_cast<AppState*>(new SurvivalDemo()); }},
    };
    return list;
}

AppState* random()
{
    const std::vector<Mode>& list = modes();
    AppState* demo = list.at(rand() % list.size()).create();
    // Pula a abertura (o "STAGE N" cinza, o aviso da rodada ou da onda): o fundo do menu já
    // começa com os tanques andando
    for(Uint32 time = 0; time < INTRO_SKIP; time += 16) demo->update(16);
    return demo;
}

} // namespace Demo
